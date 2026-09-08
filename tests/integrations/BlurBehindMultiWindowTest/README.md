# BlurBehind Multi-Window Integration Test (v3.0.0)

This integration test validates the enhanced **BlurBehind v3.0.0** native addon across 4 simultaneous `widgetWindow` instances, verifying all composition capabilities.

## Test Matrix Across Windows

| Window | Focus Area | Features Exercised |
|---|---|---|
| **Window 1** | Classic Blur & Toggle | `apply(hwnd, 'blur', 'round')`, `setCorner('roundsmall')`, `toggle(hwnd)` (off → on restore), `disable(hwnd)` |
| **Window 2** | Acrylic & Config Object | `apply(hwnd, { type: 'acrylic', corner: 'round', stroke: 'hidden' })`, `setStroke(hwnd, '#00FF7F')`, `setStroke(hwnd, 'visible')` |
| **Window 3** | Config Object & Shadows | `apply(h3, { type: 'blur', corner: 'roundsmall' })`, `setShadow(h3, 'top\|left')`, `apply(h3, { type: 'acrylic', shadow: 'all' })` |
| **Window 4** | Shadows & Composition Flags | `setShadow(hwnd, 'bottom\|right')`, `setShadow(hwnd, 'all')`, `setEffect(hwnd, 'luminance')`, `setEffect(hwnd, 'fullscreen')`, `setEffect(hwnd, 'both')` |

## Capabilities & Feature Detection
- Validates `BlurBehind.name === "BlurBehind"`
- Validates `BlurBehind.version === "3.0.0"`
- Validates `BlurBehind.supports` boolean dictionary (`blur`, `acrylic`, `corner`, `stroke`, `shadow`)
- Validates `BlurBehind.isSupported(feature)` queries

## How to Run

From PowerShell in the repository root:
```powershell
.\Run.ps1 -Script "D:\Novadesk-Project\novadesk\tests\integrations\BlurBehindMultiWindowTest\index.js"
```

Or from within the test folder:
```powershell
cd D:\Novadesk-Project\novadesk\tests\integrations\BlurBehindMultiWindowTest
nwm run
```
