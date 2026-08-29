/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "GeneralImage.h"

#include "Direct2DHelper.h"
#include "../shared/Logging.h"
#include "../shared/PathUtils.h"
#include "../Resource.h"

#include <algorithm>
#include <cstring>
#include <d2d1effects.h>
#include <thread>
#include <utility>

namespace
{
    bool DecodeImageBytes(const std::vector<BYTE>& bytes, DecodedImageData& out)
    {
        if (bytes.empty()) return false;
        const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        // Only call CoUninitialize when we genuinely initialized COM (S_OK).
        // RPC_E_CHANGED_MODE means COM was already initialized in a different
        // apartment model — calling CoUninitialize there would corrupt COM state.
        const bool uninitialize = (comHr == S_OK);
        Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
        if (SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<IWICStream> stream;
            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
            hr = factory->CreateStream(stream.GetAddressOf());
            if (SUCCEEDED(hr)) hr = stream->InitializeFromMemory(const_cast<BYTE*>(bytes.data()), static_cast<DWORD>(bytes.size()));
            if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
            if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, frame.GetAddressOf());
            if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(converter.GetAddressOf());
            if (SUCCEEDED(hr)) hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
            if (SUCCEEDED(hr)) hr = converter->GetSize(&out.width, &out.height);
            out.stride = out.width * 4;
            if (SUCCEEDED(hr) && out.height > 0 && out.stride / 4 == out.width)
            {
                const size_t byteCount = static_cast<size_t>(out.stride) * out.height;
                out.pixels.resize(byteCount);
                hr = converter->CopyPixels(nullptr, out.stride, static_cast<UINT>(byteCount), out.pixels.data());
            }
        }
        if (uninitialize) CoUninitialize();
        return SUCCEEDED(hr) && out.IsValid();
    }
}

bool GeneralImage::DecodeFromBytes(const std::vector<BYTE>& bytes, DecodedImageData& out)
{
    return DecodeImageBytes(bytes, out);
}

GeneralImage::GeneralImage()
{
    m_ColorMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
}

GeneralImage::~GeneralImage()
{
    ShutdownAsyncDownloads();
}

void GeneralImage::ResetBitmapCache()
{
    m_D2DBitmap.Reset();
}

void GeneralImage::ReloadWICBitmap()
{
    m_pWICBitmap.Reset();
    if (m_LoadedPath.empty())
        return;

    const bool ok = Direct2D::LoadWICBitmapFromFile(m_LoadedPath, m_pWICBitmap.ReleaseAndGetAddressOf(), m_UseExifOrientation);
    if (!ok)
    {
        Logging::Log(LogLevel::Error, L"[novadesk] failed to preload WIC image: %s", m_LoadedPath.c_str());
    }
}

void GeneralImage::SetFallbackPath(const std::wstring &path)
{
    if (m_FallbackPath == path)
        return;
    m_FallbackPath = path;
    if (m_IsFallbackShowing)
    {
        LoadFallbackFromResource();
    }
}

void GeneralImage::LoadFallbackFromResource()
{
    std::lock_guard<std::recursive_mutex> lock(m_ImageStateMutex);
    m_pWICBitmap.Reset();
    ResetBitmapCache();

    if (!m_FallbackPath.empty())
    {
        const bool ok = Direct2D::LoadWICBitmapFromFile(m_FallbackPath, m_pWICBitmap.ReleaseAndGetAddressOf(), m_UseExifOrientation);
        if (ok)
        {
            m_IsFallbackShowing = true;
            return;
        }
        else
        {
            Logging::Log(LogLevel::Error, L"[novadesk] failed to load custom fallback image: %s", m_FallbackPath.c_str());
        }
    }

    const bool ok = Direct2D::LoadWICBitmapFromResource(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCEW(IDR_FALLBACK_IMAGE),
        RT_RCDATA,
        m_pWICBitmap.ReleaseAndGetAddressOf());
    if (!ok)
    {
        Logging::Log(LogLevel::Error, L"[novadesk] failed to load fallback image from resource");
    }
    m_IsFallbackShowing = true;
}

void GeneralImage::SetPath(const std::wstring &path)
{
    m_ImagePath = path;
    m_LoadedPath = path;
    m_IsFallbackShowing = false;
    m_DownloadedBuffer.clear();
    m_DecodedImage = {};
    ResetBitmapCache();

    if (PathUtils::IsURL(path))
    {
        // Show the embedded fallback image immediately while the real one downloads
        LoadFallbackFromResource();
        if (m_OwnerHWND)
        {
            StartAsyncDownload(path);
        }
    }
    else
    {
        ReloadWICBitmap();
    }
}

void GeneralImage::EnsureBitmap(ID2D1DeviceContext *context)
{
    if (!context)
        return;

    if (m_pLastTarget != (ID2D1RenderTarget *)context)
    {
        m_D2DBitmap.Reset();
        m_pLastTarget = (ID2D1RenderTarget *)context;
    }

    if (!m_D2DBitmap)
    {
        std::lock_guard<std::recursive_mutex> lock(m_ImageStateMutex);
        bool ok = false;
        if (m_DecodedImage.IsValid())
        {
            const HRESULT hr = context->CreateBitmap(
                D2D1::SizeU(m_DecodedImage.width, m_DecodedImage.height),
                m_DecodedImage.pixels.data(), m_DecodedImage.stride,
                D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
                m_D2DBitmap.ReleaseAndGetAddressOf());
            ok = SUCCEEDED(hr);
            if (ok)
            {
                // Convert the raw pixels into a device-independent WIC bitmap
                // so the image can be recreated after a D2D device loss, then
                // release the raw pixel buffer to avoid keeping two CPU copies.
                IWICImagingFactory* wicFactory = Direct2D::GetWICFactory();
                if (wicFactory)
                {
                    Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
                    HRESULT wicHr = wicFactory->CreateBitmapFromMemory(
                        m_DecodedImage.width, m_DecodedImage.height,
                        GUID_WICPixelFormat32bppPBGRA,
                        m_DecodedImage.stride,
                        static_cast<UINT>(m_DecodedImage.pixels.size()),
                        m_DecodedImage.pixels.data(),
                        wicBitmap.GetAddressOf());
                    if (SUCCEEDED(wicHr))
                        m_pWICBitmap = std::move(wicBitmap);
                }
                m_DecodedImage = {};
            }
        }
        else if (m_pWICBitmap)
        {
            // The source was decoded when it was loaded/downloaded. Reuse it
            // across target recreation instead of decoding the same bytes again.
            const HRESULT hr = context->CreateBitmapFromWicBitmap(
                m_pWICBitmap.Get(),
                m_D2DBitmap.ReleaseAndGetAddressOf());
            ok = SUCCEEDED(hr);
            if (!ok && m_IsFallbackShowing)
            {
                // Reload the fallback source if it became invalid with a lost target.
                LoadFallbackFromResource();
                if (m_pWICBitmap)
                {
                    const HRESULT reloadHr = context->CreateBitmapFromWicBitmap(
                        m_pWICBitmap.Get(),
                        m_D2DBitmap.ReleaseAndGetAddressOf());
                    ok = SUCCEEDED(reloadHr);
                }
            }
        }
        else if (!m_DownloadedBuffer.empty())
        {
            // This is only reached if the initial asynchronous decode failed or
            // the WIC source was explicitly released.
            ok = Direct2D::LoadWICBitmapFromMemory(
                m_DownloadedBuffer.data(),
                static_cast<DWORD>(m_DownloadedBuffer.size()),
                m_pWICBitmap.ReleaseAndGetAddressOf());
            if (ok)
            {
                const HRESULT hr = context->CreateBitmapFromWicBitmap(
                    m_pWICBitmap.Get(),
                    m_D2DBitmap.ReleaseAndGetAddressOf());
                ok = SUCCEEDED(hr);
            }
        }
        else if (!m_LoadedPath.empty() && !PathUtils::IsURL(m_LoadedPath))
        {
            // Normal local file
            ok = Direct2D::LoadBitmapFromFile(
                context,
                m_LoadedPath,
                m_D2DBitmap.ReleaseAndGetAddressOf(),
                m_pWICBitmap.ReleaseAndGetAddressOf(),
                m_UseExifOrientation);
        }
        if (!ok)
        {
            Logging::Log(LogLevel::Error, L"[novadesk] failed to load image bitmap: %s", m_LoadedPath.c_str());
        }
    }
}

void GeneralImage::SetOwnerHWND(HWND hWnd)
{
    if (m_OwnerHWND == hWnd)
        return;
    m_OwnerHWND = hWnd;
    // If SetPath was called with a URL before we had an HWND, kick off the download now
    if (m_OwnerHWND && PathUtils::IsURL(m_ImagePath) && m_IsFallbackShowing)
    {
        StartAsyncDownload(m_ImagePath);
    }
}

void GeneralImage::StartAsyncDownload(const std::wstring& url)
{
    HWND hWnd = m_OwnerHWND;
    if (!hWnd) return;

    std::lock_guard<std::mutex> lock(m_AsyncDownloadMutex);
    if (m_AsyncDownloadsShutdown)
        return;

    m_AsyncDownloadThreads.emplace_back([this, hWnd, url]() {
        AsyncImageResult* result = new AsyncImageResult();
        if (Direct2D::DownloadImageFromURL(url, result->encodedBytes) && !result->encodedBytes.empty())
        {
            // Don't decode here — only the background-image path needs
            // pre-decoded pixels, and that is done on-demand in the handler.
            // Element images decode from the encoded bytes via OnImageDownloaded.
            // This avoids having both encodedBytes and decodedImage.pixels in
            // memory simultaneously, halving peak memory for large images.
            if (IsAsyncDownloadShutdown())
            {
                delete result;
                return;
            }

            std::wstring* pUrl = new std::wstring(url);
            if (!PostMessageW(hWnd, WM_USER + 500, (WPARAM)pUrl, (LPARAM)result))
            {
                delete pUrl;
                delete result;
            }
        }
        else
        {
            delete result;
        }
    });
}

bool GeneralImage::IsAsyncDownloadShutdown()
{
    std::lock_guard<std::mutex> lock(m_AsyncDownloadMutex);
    return m_AsyncDownloadsShutdown;
}

void GeneralImage::ShutdownAsyncDownloads()
{
    std::vector<std::thread> threads;
    {
        std::lock_guard<std::mutex> lock(m_AsyncDownloadMutex);
        m_AsyncDownloadsShutdown = true;
        threads.swap(m_AsyncDownloadThreads);
    }

    for (std::thread &thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
}

void GeneralImage::OnImageDownloaded(const std::wstring& url, const std::vector<BYTE>& buffer)
{
    if (m_ImagePath != url || buffer.empty())
        return;

    std::lock_guard<std::recursive_mutex> lock(m_ImageStateMutex);

    // Store the buffer so EnsureBitmap can create the D2D resource on the render thread
    m_DownloadedBuffer = buffer;
    m_IsFallbackShowing = false;
    m_LoadedPath = url;   // keep tracking the original URL for future reference

    // Pre-decode into WIC so GetAutoWidth/GetAutoHeight work immediately
    m_pWICBitmap.Reset();
    Direct2D::LoadWICBitmapFromMemory(
        m_DownloadedBuffer.data(),
        static_cast<DWORD>(m_DownloadedBuffer.size()),
        m_pWICBitmap.ReleaseAndGetAddressOf());

    // The decoded WIC bitmap is retained for redraws and device recreation, so
    // successful downloads no longer need to keep both compressed and decoded
    // image representations in memory.
    if (m_pWICBitmap)
        m_DownloadedBuffer.clear();

    ResetBitmapCache();
}

void GeneralImage::OnImageDecoded(const std::wstring& url, DecodedImageData&& image)
{
    if (m_ImagePath != url || !image.IsValid()) return;

    std::lock_guard<std::recursive_mutex> lock(m_ImageStateMutex);
    m_DownloadedBuffer.clear();
    m_pWICBitmap.Reset();
    m_DecodedImage = std::move(image);
    m_IsFallbackShowing = false;
    m_LoadedPath = url;
    ResetBitmapCache();
}

void GeneralImage::SetImageTint(COLORREF color, BYTE alpha)
{
    m_ImageTint = color;
    m_ImageTintAlpha = alpha;
    m_HasImageTint = (alpha > 0);
}

void GeneralImage::SetColorMatrix(const float *matrix)
{
    if (matrix)
    {
        memcpy(m_ColorMatrix.data(), matrix, sizeof(float) * 20);
        m_HasColorMatrix = true;
    }
    else
    {
        m_HasColorMatrix = false;
    }
}

void GeneralImage::SetUseExifOrientation(bool enabled)
{
    if (m_UseExifOrientation == enabled)
        return;

    m_UseExifOrientation = enabled;
    ResetBitmapCache();
    ReloadWICBitmap();
}

void GeneralImage::SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin)
{
    if (w <= 0.0f || h <= 0.0f)
    {
        m_HasImageCrop = false;
        return;
    }

    m_HasImageCrop = true;
    m_ImageCropX = x;
    m_ImageCropY = y;
    m_ImageCropW = w;
    m_ImageCropH = h;
    m_ImageCropOrigin = origin;
}

bool GeneralImage::ResolveImageCropRect(float imageWidth, float imageHeight, D2D1_RECT_F &rect) const
{
    if (!m_HasImageCrop || imageWidth <= 0.0f || imageHeight <= 0.0f || m_ImageCropW <= 0.0f || m_ImageCropH <= 0.0f)
    {
        return false;
    }

    float startX = m_ImageCropX;
    float startY = m_ImageCropY;

    switch (m_ImageCropOrigin)
    {
    case IMAGE_CROP_ORIGIN_TOP_RIGHT:
        startX = imageWidth + m_ImageCropX;
        break;
    case IMAGE_CROP_ORIGIN_BOTTOM_RIGHT:
        startX = imageWidth + m_ImageCropX;
        startY = imageHeight + m_ImageCropY;
        break;
    case IMAGE_CROP_ORIGIN_BOTTOM_LEFT:
        startY = imageHeight + m_ImageCropY;
        break;
    case IMAGE_CROP_ORIGIN_CENTER:
        startX = (imageWidth * 0.5f) + m_ImageCropX;
        startY = (imageHeight * 0.5f) + m_ImageCropY;
        break;
    default:
        break;
    }

    float left = startX;
    float top = startY;
    float right = startX + m_ImageCropW;
    float bottom = startY + m_ImageCropH;

    left = (std::max)(0.0f, left);
    top = (std::max)(0.0f, top);
    right = (std::min)(imageWidth, right);
    bottom = (std::min)(imageHeight, bottom);

    if (right <= left || bottom <= top)
    {
        return false;
    }

    rect = D2D1::RectF(left, top, right, bottom);
    return true;
}

void GeneralImage::ApplyFlipToPixel(float &pixelX, float &pixelY, const D2D1_RECT_F &srcRect) const
{
    switch (m_ImageFlip)
    {
    case IMAGE_FLIP_HORIZONTAL:
        pixelX = srcRect.right - (pixelX - srcRect.left) - 0.001f;
        break;
    case IMAGE_FLIP_VERTICAL:
        pixelY = srcRect.bottom - (pixelY - srcRect.top) - 0.001f;
        break;
    case IMAGE_FLIP_BOTH:
        pixelX = srcRect.right - (pixelX - srcRect.left) - 0.001f;
        pixelY = srcRect.bottom - (pixelY - srcRect.top) - 0.001f;
        break;
    default:
        break;
    }
}

bool GeneralImage::BuildFlipTransform(const D2D1_RECT_F &dstRect, D2D1_MATRIX_3X2_F &outTransform) const
{
    if (m_ImageFlip == IMAGE_FLIP_NONE)
        return false;

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (m_ImageFlip == IMAGE_FLIP_HORIZONTAL || m_ImageFlip == IMAGE_FLIP_BOTH)
        scaleX = -1.0f;
    if (m_ImageFlip == IMAGE_FLIP_VERTICAL || m_ImageFlip == IMAGE_FLIP_BOTH)
        scaleY = -1.0f;

    const float centerX = (dstRect.left + dstRect.right) * 0.5f;
    const float centerY = (dstRect.top + dstRect.bottom) * 0.5f;
    outTransform =
        D2D1::Matrix3x2F::Translation(-centerX, -centerY) *
        D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
        D2D1::Matrix3x2F::Translation(centerX, centerY);
    return true;
}

bool GeneralImage::BuildProcessedImage(ID2D1DeviceContext *context, Microsoft::WRL::ComPtr<ID2D1Image> &outImage) const
{
    if (!context || !m_D2DBitmap)
        return false;

    outImage = m_D2DBitmap.Get();

    if (!(m_Grayscale || m_HasImageTint || m_HasColorMatrix || m_ImageAlpha < 255))
    {
        return true;
    }

    Microsoft::WRL::ComPtr<ID2D1Image> current = m_D2DBitmap.Get();

    if (m_Grayscale)
    {
        Microsoft::WRL::ComPtr<ID2D1Effect> grayEffect;
        if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, grayEffect.GetAddressOf())))
        {
            D2D1_MATRIX_5X4_F grayMatrix = D2D1::Matrix5x4F(
                0.299f, 0.299f, 0.299f, 0.0f,
                0.587f, 0.587f, 0.587f, 0.0f,
                0.114f, 0.114f, 0.114f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f, 0.0f);
            grayEffect->SetInput(0, current.Get());
            grayEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, grayMatrix);
            grayEffect->GetOutput(&current);
        }
    }

    Microsoft::WRL::ComPtr<ID2D1Effect> colorEffect;
    if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, colorEffect.GetAddressOf())))
    {
        D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        if (m_HasColorMatrix)
        {
            memcpy(&matrix, m_ColorMatrix.data(), sizeof(float) * 20);
        }
        else if (m_HasImageTint)
        {
            matrix.m[0][0] = GetRValue(m_ImageTint) / 255.0f;
            matrix.m[1][1] = GetGValue(m_ImageTint) / 255.0f;
            matrix.m[2][2] = GetBValue(m_ImageTint) / 255.0f;
            matrix.m[3][3] = m_ImageTintAlpha / 255.0f;
        }
        // Apply global image alpha unconditionally, regardless of color matrix
        matrix.m[3][3] *= (m_ImageAlpha / 255.0f);

        colorEffect->SetInput(0, current.Get());
        colorEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
        colorEffect->GetOutput(&current);
    }

    outImage = current;
    return true;
}

int GeneralImage::GetAutoWidth() const
{
    if (m_pWICBitmap)
    {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect((float)w, (float)h, cropRect))
            return (int)(cropRect.right - cropRect.left);
        return (int)w;
    }
    if (m_D2DBitmap)
    {
        D2D1_SIZE_F size = m_D2DBitmap->GetSize();
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect(size.width, size.height, cropRect))
            return (int)(cropRect.right - cropRect.left);
        return (int)size.width;
    }
    return 0;
}

int GeneralImage::GetAutoHeight() const
{
    if (m_pWICBitmap)
    {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect((float)w, (float)h, cropRect))
            return (int)(cropRect.bottom - cropRect.top);
        return (int)h;
    }
    if (m_D2DBitmap)
    {
        D2D1_SIZE_F size = m_D2DBitmap->GetSize();
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect(size.width, size.height, cropRect))
            return (int)(cropRect.bottom - cropRect.top);
        return (int)size.height;
    }
    return 0;
}

BYTE GeneralImage::GetPixelAlpha(int x, int y) const
{
    if (!m_pWICBitmap)
        return 0;

    UINT width = 0, height = 0;
    m_pWICBitmap->GetSize(&width, &height);
    if (x < 0 || x >= (int)width || y < 0 || y >= (int)height)
        return 0;

    Microsoft::WRL::ComPtr<IWICBitmapLock> pLock;
    WICRect rcLock = { x, y, 1, 1 };
    if (SUCCEEDED(m_pWICBitmap->Lock(&rcLock, WICBitmapLockRead, pLock.GetAddressOf())))
    {
        UINT cbBufferSize = 0;
        BYTE* pv = nullptr;
        if (SUCCEEDED(pLock->GetDataPointer(&cbBufferSize, &pv)) && cbBufferSize >= 4)
        {
            // 32bppPBGRA format used in Direct2DHelper
            return pv[3];
        }
    }
    return 0;
}
