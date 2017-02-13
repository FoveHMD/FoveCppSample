# FOVE C++ Example

This repository contains a C++ program that gives an example use of the FOVE SDK. It currently does the following:
- DirectX setup
- Orientation tracking
- Position tracking
- Submitting left and right eye frames to the FOVE Compositor

More features are planned in the future, including Eye Tracking.

## How to build

To build and run, follow these steps:

1. Ensure CMake (https://cmake.org/) and Visual Studio 2015 or later are installed.
2. Ensure that the folder containing FoveClient.dll is added to the PATH environment variable.
3. Run CMake to generate a 64 bit Visual Studio project.
4. Open the generated Visual Studio project.
5. Ensure that the selected target is x64 (and the linker options do not contain /machine:X86).
6. Build and run.

If you run into any difficulties, hit us up on the forums. http://forum.getfove.com/

If you have improvements you would like to submit to the sample project, send us a pull request.