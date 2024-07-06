# CMake project for FOVE C++ Examples
cmake_minimum_required(VERSION 3.14)
project(FoveCppExamples)

# Enable C++17
# FOVE SDK supports C++11 and later, we use C++17 for some code readability improvements
set(CMAKE_CXX_STANDARD 17)

if (NOT WIN32)
	find_package(Threads REQUIRED)
endif()

# Find the Fove SDK
if("${CppExampleBuildInTree}")

	# This allows building within the the FOVE SDK source checkout (for FOVE developers)
	message(STATUS "Building FOVE CPP Examples in tree")
	add_definitions(-DFOVE_CPP_SAMPLE_BUILD_IN_TREE)

else()

	# Setup a FOVE_SDK_PATH variable to allow using a FOVE SDK from a custom location
	set(FOVE_SDK_PATH "" CACHE PATH "Directory of extracted FOVE SDK")

	# Find the sdk path (falling back to the built-in one if FOVE_SDK_PATH is not set)
	set(sdkFolder "${CMAKE_CURRENT_LIST_DIR}/FOVE SDK 1.3.1") # Path to builtin sdk folder
	if(NOT "${FOVE_SDK_PATH}" STREQUAL "")
		# Custom error message that suggests using the built-in release if you are pointing to a bad folder
		if(NOT EXISTS "${FOVE_SDK_PATH}" AND EXISTS "${sdkFolder}")
			message(FATAL_ERROR "Unable to find FOVE SDK at ${sdkFolder}\nPlease set FOVE_SDK_PATH to the location of the FOVE SDK.\nSuggested path: ${sdkFolder}")
		endif()
		set(sdkFolder "${FOVE_SDK_PATH}")
	endif()

	# Ensure the sdk folder is valid
	if(NOT EXISTS "${sdkFolder}")
		message(FATAL_ERROR "Unable to find FOVE SDK. Please set FOVE_SDK_PATH to the location of the FOVE SDK.\nNo folder exists at: ${sdkFolder}")
	endif()
	if(NOT EXISTS "${sdkFolder}/FoveAPI.h")
		message(FATAL_ERROR "Unable to find FOVE SDK headers. Please set FOVE_SDK_PATH to the location of the FOVE SDK.\nNo file exists at: ${sdkFolder}/FoveAPI.h")
	endif()
	message(STATUS "Using FOVE SDK at ${sdkFolder}")

	# Locate the client dynamic library
	if(WIN32)
		# On Windows, the file we need to link (.lib) and the runtime file (.dll) are different
		# The DLL file is copied to the same folder as the exe so Windows can find it at runtime
		set(foveClientLinkObject "${sdkFolder}/FoveClient.lib")
		set(foveClientObjectToCopy "${sdkFolder}/FoveClient.dll")
		list(APPEND genericDefinitions UNICODE)
	elseif(APPLE)
		set(foveClientLinkObject "${sdkFolder}/libFoveClient.dylib")
	else()
		set(foveClientLinkObject "${sdkFolder}/libFoveClient.so")
	endif()
	if(NOT EXISTS "${foveClientLinkObject}")
		message(FATAL_ERROR "Missing FOVE library at ${foveClientLinkObject}")
	endif()
	if(DEFINED foveClientObjectToCopy AND NOT EXISTS "${foveClientObjectToCopy}")
		message(FATAL_ERROR "Missing FOVE library at ${foveClientObjectToCopy}")
	endif()

	# Setup build parameters
	list(APPEND genericIncludeDirs "${sdkFolder}")
	list(APPEND genericLinkLibraries "${foveClientLinkObject}")
endif()

# Determine the utility headers we have for the current platform
if(WIN32)
	set(nativeUtilFiles WIN32 NativeUtil.h WindowsUtil.h WindowsUtil.cpp)
elseif(APPLE)
	set(nativeUtilFiles NativeUtil.h)
else()
	set(nativeUtilFiles NativeUtil.h LinuxUtil.h LinuxUtil.cpp)
endif()

# Create DirectX11 example, and the option to enable/disable it
if(WIN32 AND MSVC)
	option(FOVE_BUILD_DIRECTX11_EXAMPLE "Enable building of the DirectX11 Example" ON)
else()
	option(FOVE_BUILD_DIRECTX11_EXAMPLE "Enable building of the DirectX11 Example" OFF)
endif()
if(FOVE_BUILD_DIRECTX11_EXAMPLE)
	# Precompile shaders
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Shader.frag_compiled.h
		COMMAND fxc.exe /nologo /E frag /T ps_4_0 /Fh ${CMAKE_CURRENT_BINARY_DIR}/Shader.frag_compiled.h Shader.frag.hlsl
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Shader.vert_compiled.h
		COMMAND fxc.exe /nologo /E vert /T vs_4_0 /Fh ${CMAKE_CURRENT_BINARY_DIR}/Shader.vert_compiled.h Shader.vert.hlsl
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
	add_custom_target(FoveDirectX11ExampleShaders DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Shader.frag_compiled.h ${CMAKE_CURRENT_BINARY_DIR}/Shader.vert_compiled.h)

	# Declare the DirectX11 example target
	add_executable(FoveDirectX11Example ${nativeUtilFiles} DirectX11Example.cpp Util.h Util.cpp DXUtil.h DXUtil.cpp)
	add_dependencies(FoveDirectX11Example FoveDirectX11ExampleShaders)
	target_include_directories(FoveDirectX11Example PRIVATE ${genericIncludeDirs} ${CMAKE_CURRENT_BINARY_DIR})
	target_compile_definitions(FoveDirectX11Example PRIVATE ${genericDefinitions})
	target_link_libraries(FoveDirectX11Example ${genericLinkLibraries})

	# Add the DirectX11 example to our list of targets which is used below
	list(APPEND allTargets FoveDirectX11Example)
endif()

# Create the Vulkan example, and the option to enable/disable it
# This is currently only supported on Linux
if(WIN32 OR APPLE)
	option(FOVE_BUILD_VULKAN_EXAMPLE "Enable building of the Vulkan Example" OFF)
else()
	option(FOVE_BUILD_VULKAN_EXAMPLE "Enable building of the Vulkan Example" ON)
endif()
if(FOVE_BUILD_VULKAN_EXAMPLE)
	# Find the Vulkan libraries installed on the system
	find_package(Vulkan REQUIRED)
	if (NOT TARGET Vulkan::Vulkan)
		message(FATAL_ERROR "FOVE_BUILD_VULKAN_EXAMPLE is on but Vulkan library not found")
	endif()

	# Find the X11 libraries installed on the system
	find_package(X11 REQUIRED)
	if (NOT TARGET X11::X11)
		message(FATAL_ERROR "FOVE_BUILD_VULKAN_EXAMPLE is on but X11 libraries could not be find")
	endif()

	add_executable(FoveSpv2Txt "${CMAKE_CURRENT_LIST_DIR}/spv2txt.cpp")
	set(VULKAN_SHADER_IN_DIR "${CMAKE_CURRENT_LIST_DIR}/shaders")
	set(VULKAN_SHADER_OUT_DIR "${PROJECT_BINARY_DIR}/shaders")
	set(VULKAN_SHADERS "DemoScene" "TextureCopy")
	foreach(shader ${VULKAN_SHADERS})
		set(VULKAN_FRAG_SPV "${VULKAN_SHADER_IN_DIR}/${shader}.frag.spv")
		set(VULKAN_VERT_SPV "${VULKAN_SHADER_IN_DIR}/${shader}.vert.spv")
		set(VULKAN_FRAG_OUT "${VULKAN_SHADER_OUT_DIR}/${shader}.frag.spv")
		set(VULKAN_VERT_OUT "${VULKAN_SHADER_OUT_DIR}/${shader}.vert.spv")
		add_custom_command(
			OUTPUT "${VULKAN_FRAG_OUT}.h" "${VULKAN_FRAG_OUT}.c" "${VULKAN_VERT_OUT}.h" "${VULKAN_VERT_OUT}.c"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${VULKAN_SHADER_OUT_DIR}"
			COMMAND FoveSpv2Txt "${VULKAN_FRAG_SPV}" "${VULKAN_FRAG_OUT}.h" "${VULKAN_FRAG_OUT}.c" vlk_shader${shader}Frag
			COMMAND FoveSpv2Txt "${VULKAN_VERT_SPV}" "${VULKAN_VERT_OUT}.h" "${VULKAN_VERT_OUT}.c" vlk_shader${shader}Vert
			DEPENDS "${VULKAN_FRAG_SPV}" "${VULKAN_VERT_SPV}" FoveSpv2Txt)
		list(APPEND VULKAN_SPIRV_BINARY_FILES "${VULKAN_FRAG_SPV}" "${VULKAN_VERT_SPV}")
		list(APPEND VULKAN_SPIRV_TEXT_FILES "${VULKAN_FRAG_OUT}.h" "${VULKAN_FRAG_OUT}.c" "${VULKAN_VERT_OUT}.h" "${VULKAN_VERT_OUT}.c")
	endforeach(shader)
	add_custom_target(FoveVulkanShaders
		DEPENDS ${VULKAN_SPIRV_BINARY_FILES} ${VULKAN_SPIRV_TEXT_FILES}
	)

	# Declare the Vulkan example target
	add_executable(FoveVulkanExample  ${nativeUtilFiles} VulkanExample.cpp Util.h Util.cpp Model.h ${VULKAN_SPIRV_TEXT_FILES})
	add_dependencies(FoveVulkanExample FoveVulkanShaders)
	target_include_directories(FoveVulkanExample PRIVATE ${genericIncludeDirs} "${VULKAN_SHADER_OUT_DIR}")
	target_compile_definitions(FoveVulkanExample PRIVATE ${genericDefinitions})
	target_link_libraries(FoveVulkanExample
		${genericLinkLibraries}
		$<$<PLATFORM_ID:Linux>:Threads::Threads>
		Vulkan::Vulkan
		X11::X11
		${CMAKE_DL_LIBS}
	)

	# Add the Vulkan example to our list of targets which is used below
	list(APPEND allTargets FoveVulkanExample)
endif()

# Create the OpenGL example, and the option to enable/disable it
# It's disabled by default on mac since OpenGL is deprecated
if(WIN32)
	option(FOVE_BUILD_OPENGL_EXAMPLE "Enable building of the OpenGL Example" ON)
else()
	option(FOVE_BUILD_OPENGL_EXAMPLE "Enable building of the OpenGL Example" OFF)
endif()
if(FOVE_BUILD_OPENGL_EXAMPLE)
	# Declare the OpenGL example target
	add_executable(FoveOpenGLExample ${nativeUtilFiles} OpenGLExample.cpp Util.h Util.cpp OpenGLUtil.h OpenGLUtil.cpp Model.h)

	# Add the OpenGL example to our list of targets which is used below
	list(APPEND allTargets FoveOpenGLExample)

	# Link the OpenGL lbiraries
	if(WIN32)
		# On windows, just link the opengl files directly without going through CMake's find_package system
		# OpenGL can be easily linked via the opengl32.lib file
		# The header files are included in the Windows SDK
		target_include_directories(FoveOpenGLExample PRIVATE ${genericIncludeDirs})
		target_compile_definitions(FoveOpenGLExample PRIVATE ${genericDefinitions})
		target_link_libraries(FoveOpenGLExample ${genericLinkLibraries} opengl32.lib)
	else()
		# On mac and linux, we use find_package from CMake

		# On linux we additionally use XLIB + EGL to create a Window with OpenGL, for this example
		# This means no native Wayland suport (though XWayland should have us covered for now)
		# For those who don't want to render to a window, only the headset, it's possible to just use headless EGL with no window system
		if (NOT WIN32 AND NOT APPLE)
			# X11
			find_package(X11 REQUIRED)
			list(APPEND openglIncludeDirs ${X11_INCLUDE_DIR})
			list(APPEND openglLinkLibraries ${X11_LIBRARIES})

			# EGL
			list(APPEND openglPackages EGL)
			list(APPEND openglLinkLibraries OpenGL::EGL)
		endif ()

		# Find OpenGL (and other related opengl packages like EGL if needed)
		find_package(OpenGL REQUIRED OpenGL ${openglPackages})

		target_include_directories(FoveOpenGLExample PRIVATE ${genericIncludeDirs} ${openglIncludeDirs})
		target_compile_definitions(FoveOpenGLExample PRIVATE ${genericDefinitions})
		target_link_libraries(FoveOpenGLExample ${genericLinkLibraries} OpenGL::OpenGL ${openglLinkLibraries})
	endif()
endif()

# Create the data example, and the option to enable/disable it
option(FOVE_BUILD_DATA_EXAMPLE "Enable building of the Data Example" ON)
if(FOVE_BUILD_DATA_EXAMPLE)
	# Declare the Data example target
	add_executable(FoveDataExample DataExample.cpp Util.h Util.cpp)
	target_include_directories(FoveDataExample PRIVATE ${genericIncludeDirs})
	target_compile_definitions(FoveDataExample PRIVATE ${genericDefinitions})
	target_link_libraries(FoveDataExample ${genericLinkLibraries} ${openglLinkLibraries})

	# Add the Data example to our list of targets which is used below
	list(APPEND allTargets FoveDataExample)
endif()

# Add a post-build command to each target to copy the FoveClient dynamic library to the executable location
# Otherwise the executable will not be able to find the dll, and will fail to launch
if(foveClientObjectToCopy)
	foreach(target ${allTargets})
		add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${foveClientObjectToCopy}" $<TARGET_FILE_DIR:${target}>)
	endforeach()
endif()

# Set default startup project to something other than cmake's ALL_BUILD target
# This way anyone hitting build-and-run will run one of the targets instead of ALL_BUILD which isn't runable
if(MSVC AND NOT "${CppExampleBuildInTree}" AND allTargets)
	list(GET allTargets 0 firstTarget)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${firstTarget})
endif()
