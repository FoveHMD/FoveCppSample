# FOVE C++ Examples

This repository contains C++ programs that demonstrate how to use the FOVE SDK.

The Data Example is a platform-independant example that shows how to:
- Properly connect to the service
- Read out data and check for error

The DirectX11 Example is Windows-specific and demonstrates the following:
- DirectX setup
- Orientation tracking
- Position tracking
- Stereo rendering
- Submitting frames to the FOVE Compositor
- Eye tracking (objects are highlighted when gazed upon)
- Displaying the rendered scene in a window in addition to the HMD

The OpenGL example is similar to the DirectX11 example, except it uses OpenGL for rendering Previous versions used the WGL_NV_DX_interop2 extension to render to a DirectX11 surface (needed for submission to the FOVE compositor), but this is now internally handled by the FOVE API.

## How to build

This project uses [CMake](https://cmake.org/) to generate a Makefile or project file for a variety of IDEs.

You can use a standard CMake build process:
1. Make a build folder (usually just a folder called 'build' at the top level).
2. Use CMake command line or gui to generate project files
3. Build and run



Visual Studio:
1. Ensure CMake (https://cmake.org/) and Visual Studio 2015 or later are installed.
2. Run CMake to generate a 64-bit Visual Studio project (build folder can go anywhere).
3. Open the generated Visual Studio project, and build and run from there.



Qt Creator + MSVC tool chain:
1. Ensure that you have at least CMake 3.7, Qt Creator 3.8, and the MSVC tool chain (2015 or later) installed.
2. Setup a cmake kit in the Qt Creator options, and ensure that it points to the right cmake/compiler/debugger/generator (use the same generators as suggested above).
3. Open the CMakeList.txt file with Qt Creator, ensure that you've selected the kit you made, and build and run from there.

If you run into any difficulties, hit us up on the forums. http://forum.getfove.com/

If you have improvements you would like to submit to the sample project, send us a pull request.
