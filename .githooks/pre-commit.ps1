# Find clang-format bundled with Microsoft C/C++ extension or from PATH
$clang = Get-Command "clang-format" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1

if (-not $clang) {
    $clang = Get-ChildItem "$env:USERPROFILE\.vscode\extensions\ms-vscode.cpptools-*\LLVM\bin\clang-format.exe" -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}

if (-not $clang) {
    Write-Host "Notice: clang-format.exe not found. Skipping auto-formatting." -ForegroundColor Yellow
    exit 0
}

Write-Host "Using clang-format: $clang"

# Find C/C++ source files (excluding third_party and build artifacts)
$files = Get-ChildItem .\src -Recurse -Include *.cpp,*.c,*.h,*.hpp -ErrorAction SilentlyContinue |
    Where-Object {
        $_.FullName -notmatch '\\(build|out|dist|node_modules|\.git|third_party|external|vendor)\\'
    }

foreach ($file in $files) {
    & $clang -i --sort-includes=false $file.FullName
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Formatting failed on: $($file.FullName)" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Formatting completed successfully." -ForegroundColor Green
exit 0
