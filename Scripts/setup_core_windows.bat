@echo off

pushd ..
Vendor\Binaries\Premake\Windows\premake5.exe --modules=core --file=Build.lua vs2026
popd
pause