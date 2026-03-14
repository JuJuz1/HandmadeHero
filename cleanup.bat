@echo off
echo Cleaning up Microsoft Visual C++ (MSVC)
echo compiler toolset generated files
echo.

rem object files (.obj)
powershell -NoProfile -Command "$shell = New-Object -ComObject Shell.Application; $exts = '*.obj','*.ilk','*.exe','*.pdb','*.dll','*.map','*.lib','*.exp'; Get-ChildItem -Path . -Recurse -Include $exts -File | ForEach-Object { Write-Host 'Moving:' $_.FullName; $shell.Namespace(0xA).MoveHere($_.FullName) }"
