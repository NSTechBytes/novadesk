/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "DiskMonitor.h"
#include <winioctl.h>

DiskMonitor::DiskMonitor(const std::wstring& driveLetter)
    : m_DriveLetter(driveLetter)
    , m_LastReadBytes(0)
    , m_LastWriteBytes(0)
    , m_LastUpdateTime(0)
    , m_FirstUpdate(true)
{
    // Ensure drive letter ends with backslash
    if (!m_DriveLetter.empty() && m_DriveLetter.back() != L'\\')
    {
        m_DriveLetter += L'\\';
    }
}

DiskMonitor::~DiskMonitor()
{
}

DiskMonitor::Stats DiskMonitor::GetStats()
{
    Stats stats = { 0, 0, 0, 0, 0, 0 };

    // Get disk space information
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceEx(m_DriveLetter.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
    {
        stats.totalSpace = totalBytes.QuadPart;
        stats.freeSpace = totalFreeBytes.QuadPart;
        stats.usedSpace = totalBytes.QuadPart - totalFreeBytes.QuadPart;
        
        if (totalBytes.QuadPart > 0)
        {
            stats.percentUsed = (int)((stats.usedSpace * 100) / totalBytes.QuadPart);
        }
    }

    // Get disk I/O statistics (read/write speeds)
    // Note: This is a simplified version - for accurate I/O tracking,
    // you'd need to use Performance Counters or WMI
    DISK_PERFORMANCE diskPerf = { 0 };
    std::wstring devicePath = L"\\\\.\\" + m_DriveLetter.substr(0, 2);
    
    HANDLE hDevice = CreateFile(
        devicePath.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hDevice != INVALID_HANDLE_VALUE)
    {
        DWORD bytesReturned;
        if (DeviceIoControl(
            hDevice,
            IOCTL_DISK_PERFORMANCE,
            NULL,
            0,
            &diskPerf,
            sizeof(diskPerf),
            &bytesReturned,
            NULL))
        {
            unsigned __int64 totalRead = diskPerf.BytesRead.QuadPart;
            unsigned __int64 totalWrite = diskPerf.BytesWritten.QuadPart;

            ULONGLONG currentTime = GetTickCount64();

            if (!m_FirstUpdate && m_LastUpdateTime > 0)
            {
                ULONGLONG timeDiff = currentTime - m_LastUpdateTime;
                if (timeDiff > 0)
                {
                    double seconds = timeDiff / 1000.0;
                    stats.readBytesPerSec = (totalRead - m_LastReadBytes) / seconds;
                    stats.writeBytesPerSec = (totalWrite - m_LastWriteBytes) / seconds;
                }
            }

            m_LastReadBytes = totalRead;
            m_LastWriteBytes = totalWrite;
            m_LastUpdateTime = currentTime;
            m_FirstUpdate = false;
        }

        CloseHandle(hDevice);
    }

    return stats;
}
