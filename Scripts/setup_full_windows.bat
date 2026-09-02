@echo off

pushd ..
Vendor\Binaries\Premake\Windows\premake5.exe --modules=core,app,tests --file=Build.lua vs2026
popd
pause