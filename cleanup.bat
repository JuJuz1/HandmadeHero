@echo off
echo Cleaning up Microsoft Visual C++ (MSVC)
echo compiler toolset generated files
echo -----------------------------------------------

rem object files (.obj)
echo Deleting .obj files...
rem move to recycle bin
powershell -Command "$shell = New-Object -ComObject Shell.Application; Get-ChildItem -Path . -Filter '*.obj' -Recurse | ForEach-Object {Write-Host 'Moving: ' $_.FullName; $shell.Namespace(0xA).MoveHere($_.FullName)}"
echo Done with .obj files.
echo -----------------------------------------------

rem linker files (.ilk)
echo Deleting .ilk files...
powershell -Command "$shell = New-Object -ComObject Shell.Application; Get-ChildItem -Path . -Filter '*.ilk' -Recurse | ForEach-Object {Write-Host 'Moving: ' $_.FullName; $shell.Namespace(0xA).MoveHere($_.FullName)}"
echo Done with .ilk files.
echo -----------------------------------------------

rem executables (.exe)
echo Deleting .exe files...
powershell -Command "$shell = New-Object -ComObject Shell.Application; Get-ChildItem -Path . -Filter '*.exe' -Recurse | ForEach-Object {Write-Host 'Moving: ' $_.FullName; $shell.Namespace(0xA).MoveHere($_.FullName)}"
echo Done with .exe files.
echo -----------------------------------------------

rem program database files (.pdb)
echo Deleting .pdb files...
powershell -Command "$shell = New-Object -ComObject Shell.Application; Get-ChildItem -Path . -Filter '*.pdb' -Recurse | ForEach-Object {Write-Host 'Moving: ' $_.FullName; $shell.Namespace(0xA).MoveHere($_.FullName)}"
echo Done with .pdb files.

echo -----------------------------------------------
echo Cleanup completed successfully!
