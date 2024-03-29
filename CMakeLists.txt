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

else()

	# Setup a FOVE_SDK_PATH variable to allow using a FOVE SDK from a custom location
	set(FOVE_SDK_PATH "" CACHE PATH "Directory of extracted FOVE SDK")

	# Find the sdk path (falling back to the built-in one if FOVE_SDK_PATH is not set)
	set(sdkFolder "${CMAKE_CURRENT_LIST_DIR}/FOVE SDK 1.2.0") # Path to builtin sdk folder
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
	# Declare the DirectX11 example target
	add_executable(FoveDirectX11Example ${nativeUtilFiles} Shader.vert.hlsl Shader.frag.hlsl DirectX11Example.cpp Util.h Util.cpp DXUtil.h DXUtil.cpp)
	target_include_directories(FoveDirectX11Example PRIVATE ${genericIncludeDirs})
	target_compile_definitions(FoveDirectX11Example PRIVATE ${genericDefinitions})
	target_link_libraries(FoveDirectX11Example ${genericLinkLibraries})

	# Tell the build system to compile the shaders to a header file, and give the shader compiler build info like the entry point
	# This is MSBuild specific, for other build systems we should make a custom step to compile these
	set_source_files_properties(Shader.vert.hlsl PROPERTIES VS_SHADER_FLAGS "/Fh \"$(OutDir)%(Filename)_compiled.h\"" VS_SHADER_TYPE Vertex VS_SHADER_MODEL 4.0 VS_SHADER_ENTRYPOINT vert)
	set_source_files_properties(Shader.frag.hlsl PROPERTIES VS_SHADER_FLAGS "/Fh \"$(OutDir)%(Filename)_compiled.h\"" VS_SHADER_TYPE Pixel  VS_SHADER_MODEL 4.0 VS_SHADER_ENTRYPOINT frag)

	# Include the directory where the compiled shader header files will be put
	# Note: include_directories/target_include_directories directories don't work here
	# Cmake treats $(OutDir) as a relative path and prepends stuff to it to make it absolute
	# Instead, use the compiler flags to add the include directory to avoid changes from cmake
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \"/I$(OutDir)\"")

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
	add_executable(FoveOpenGLExample ${nativeUtilFiles} OpenGLExample.cpp Util.h Util.cpp OpenGLUtil.h OpenGLUtil.cpp)
	target_include_directories(FoveOpenGLExample PRIVATE ${genericIncludeDirs})
	target_compile_definitions(FoveOpenGLExample PRIVATE ${genericDefinitions})
	target_link_libraries(FoveOpenGLExample ${genericLinkLibraries} "opengl32.lib")

	# Add the OpenGL example to our list of targets which is used below
	list(APPEND allTargets FoveOpenGLExample)
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
