@echo off

pushd ..
Vendor\Binaries\Premake\Windows\premake5.exe --modules=app --file=Build.lua vs2026
popd
pause