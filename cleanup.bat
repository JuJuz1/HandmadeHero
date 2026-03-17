@echo off
echo Cleaning up build files
echo.

rem move to recycling bin
powershell -NoProfile -Command "$shell = New-Object -ComObject Shell.Application; $exts = '*.obj','*.ilk','*.exe','*.pdb','*.dll','*.map','*.lib','*.exp','*.hmi','*.out'; Get-ChildItem -Path . -Recurse -Include $exts -File | ForEach-Object { Write-Host 'Moving:' $_.FullName; $shell.Namespace(0xA).MoveHere($_.FullName) }"
