#!/bin/bash

# Generates GCC makefiles. The build needs GCC 14 or newer (std::ranges::to); where the distro's
# default g++ is older (Ubuntu 24.04 ships 13), build with: make CC=gcc-14 CXX=g++-14
pushd ..
Vendor/Binaries/Premake/Linux/premake5 --cc=gcc --file=Build.lua gmake
popd
