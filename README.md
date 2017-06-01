# FOVE C++ Examples

This repository contains C++ programs that demonstrates how to use the FOVE SDK.

The data example shows how to:
- Properly connect to the service
- Read out data and check for error

The DirectX11 example demonstrates the following:
- DirectX setup
- Orientation tracking
- Position tracking
- Stereo rendering
- Submitting frames to the FOVE Compositor
- Eye tracking (objects are highlighted when gazed upon)

## How to build

To build and run, follow these steps:

Visual Studio:
1. Ensure CMake (https://cmake.org/) and Visual Studio 2015 or later are installed.
2. Run CMake to generate a 64-bit Visual Studio project (make sure to select a 64 bit generator such as "Visual Studio 14 2015 Win64").
3. Open the generated Visual Studio project, and build and run from there.

Qt Creator + MSVC tool chain:
1. Ensure that you have at least CMake 3.7, Qt Creator 3.8, and the MSVC tool chain (2015 or later) installed.
2. Setup a cmake kit in the Qt Creator options, and ensure that it points to the right cmake/compiler/debugger/generator (use the same generators as suggested above).
3. Open the CMakeList.txt file with Qt Creator, ensure that you've selected the kit you made, and build and run from there.

If you run into any difficulties, hit us up on the forums. http://forum.getfove.com/

If you have improvements you would like to submit to the sample project, send us a pull request.
