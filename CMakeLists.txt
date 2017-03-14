# CMake project for FOVE C++ Examples
cmake_minimum_required(VERSION 2.6)
project(FoveCppExamples)

if ("${CppExampleBuildInTree}")
	# This allows building within the the FOVE SDK checkout (for FOVE developers)
	message(STATUS "Building FOVE CPP Examples in tree")
	link_libraries(FoveClientStatic)
else()

	# Setup a FOVE_SDK_PATH variable to allow using a FOVE SDK from a custom location
	# This older variable is no longer used since probably it got cached to an old SDK name
	set(FOVE_SDK_PATH "" CACHE PATH "Directory of extracted FOVE SDK")
	unset(FOVE_SDK_FOLDER CACHE)

	# Find the sdk path (falling back to the built-in one if FOVE_SDK_PATH is not set)
	set(sdk_folder "${CMAKE_CURRENT_LIST_DIR}/FOVE SDK 0.10.0") # Path to builtin sdk folder
	if (NOT "${FOVE_SDK_PATH}" STREQUAL "")
		# Custom error message that suggests using the built-in release if you are pointing to a bad folder
		if(NOT EXISTS "${FOVE_SDK_PATH}" AND EXISTS "${sdk_folder}")
			message(FATAL_ERROR "Unable to find FOVE SDK at ${sdk_folder}\nPlease set FOVE_SDK_PATH to the location of the FOVE SDK.\nSuggested path: ${sdk_folder}")
		endif()
		set(sdk_folder "${FOVE_SDK_PATH}")
	endif()
	
	# Enure the sdk folder is valid
	if(NOT EXISTS "${sdk_folder}")
		message(FATAL_ERROR "Unable to find FOVE SDK. Please set FOVE_SDK_PATH to the location of the FOVE SDK.\nNo folder exists at: ${sdk_folder}")
	endif()
	if(NOT EXISTS "${sdk_folder}/include/IFVRHeadset.h")
		message(FATAL_ERROR "Unable to find FOVE SDK headers. Please set FOVE_SDK_PATH to the location of the FOVE SDK.\nNo file exists at: ${sdk_folder}/include/IFVRHeadset.h")
	endif()
	message(STATUS "Using FOVE SDK at ${sdk_folder}")

	# Setup build parameters
	set(CMAKE_CXX_STANDARD 14)
	add_definitions(-DUNICODE)
	include_directories("${sdk_folder}/include")
	link_libraries("${sdk_folder}/lib/FoveClient.lib")
endif()

# Create DirectX 11 Example
add_executable(FoveDirectX11Example WIN32 DirectX11Example.cpp Util.h Util.cpp DXUtil.h DXUtil.cpp)

if (NOT "${CppExampleBuildInTree}")
	# Copy DLL to binary folder so it can be found when the compiled program runs
	set(FoveDLL "${sdk_folder}/bin/FoveClient.dll")
	if(NOT EXISTS "${FoveDLL}")
		message(FATAL_ERROR "Missing Fove DLL at ${FoveDLL}")
	endif()
	add_custom_command(TARGET FoveDirectX11Example POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FoveDLL}" $<TARGET_FILE_DIR:FoveDirectX11Example>)

	# Set default startup project to something other than cmake's ALL_BUILD target
	# This way anyone hitting build-and-run will get something instead of an error
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT FoveDirectX11Example)
	endif()
endif()
