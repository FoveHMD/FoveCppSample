# FOVE C++ Examples

This repository contains C++17 (or newer) programs that demonstrate how to use the FOVE SDK. They can also serve as base code upon which to build your own eye-tracking VR app with FOVE.

The **Data Example** is a platform-independent example that shows how to:
- Connect to the FOVE Service
- Read out data from the headset, and check for error

The **DirectX11 Example** is Windows-specific and demonstrates the following:
- DirectX setup
- Orientation tracking
- Position tracking
- Stereo rendering
- Submitting frames to the FOVE Compositor
- Eye tracking (objects are highlighted when gazed upon)
- Displaying the rendered scene in a window in addition to the HMD

The **OpenGL Example** is similar to the DirectX11 Example, except it uses OpenGL for rendering. It is Windows only for now. Previous versions used the WGL_NV_DX_interop2 extension to render to a DirectX11 surface (needed for submission to the FOVE compositor), but this is now internally handled by the FOVE API.

The **Vulkan Example** is also similar to the DirectX11 Example, but Linux-only and using Vulkan. To keep things simple, the compiled shaders are in included (alongside the source) in the repo so compiling shaders is not needed. OpenGL and DirectX11 by contrast include a means to compile shaders at runtime, so only the Vulkan Example has this.

> Note: All of these examples are meant to be as short and simple as possible to be understandable. They do not always show the best approach. For example, in the graphical examples we render to the HMD and the PC monitor in the same thread .This is not recommended in production since they will likely have different frame rates.

## How to build

This project uses [CMake](https://cmake.org/) to generate a Makefile or project file for a variety of IDEs.

You can use a standard CMake build process:
1. Make a build folder (usually just a folder called 'build' at the top level).
2. Use CMake command line or gui to generate project files
3. Build and run

For example, if you want to use Visual Studio;
1. Ensure CMake and Visual Studio 2015 or later are installed.
2. Run CMake to generate a 64-bit Visual Studio project (build folder can go anywhere).
3. Open the generated Visual Studio project, and build and run from there.

If you want to compile directly without CMake, you can just pass the needed cpp files and search paths and libraries. The data example is the simplest:

```bash
bash$ c++ DataExample.cpp Util.cpp -I "FOVE SDK"* -L "FOVE SDK"* -lFoveClient -o DataExample
bash$ LD_LIBRARY_PATH=$(cd "FOVE SDK"* && pwd) ./DataExample
```

```cmd
x64 Native Tools Command Prompt for VS> CL.exe /EHsc /I"FOVE SDK X.X.X" DataExample.cpp Util.cpp "FOVE SDK X.X.X/FoveClient.lib"
x64 Native Tools Command Prompt for VS> DataExample.exe
```

## Contact

You can get in touch with us from [our website](https://fove-inc.com/contact/).

If you have fixes or improvements you would like to submit, don't hesitate to send us a pull request on GitHub.
