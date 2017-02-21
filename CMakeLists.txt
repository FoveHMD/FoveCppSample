# CMake project for FOVE C++ Examples
cmake_minimum_required(VERSION 2.6)
project(FoveCppExamples)

if ("${CppExampleBuildInTree}")
	# This allows building within the the FOVE SDK checkout (for FOVE developers)
	message(STATUS "Building FOVE CPP Examples in tree")
	link_libraries(FoveClientStatic)
else()

	# Find SDK RELEASE FOLDER
	set(FOVE_SDK_FOLDER "${CMAKE_CURRENT_LIST_DIR}/FOVE SDK 0.10.0" CACHE PATH "Directory of extracted FOVE SDK")
	if(NOT EXISTS "${FOVE_SDK_FOLDER}")
		message(FATAL_ERROR "Please set FOVE_SDK_FOLDER to the location of the FOVE SDK.\nNo folder exists at: ${CMAKE_CURRENT_LIST_DIR}/SDK_Release")
	endif()

	# Setup build parameters
	set(CMAKE_CXX_STANDARD 14)
	add_definitions(-DUNICODE)
	include_directories("${FOVE_SDK_FOLDER}/include")
	link_libraries("${FOVE_SDK_FOLDER}/lib/FoveClient.lib")
endif()

# Create DirectX 11 Example
add_executable(FoveDirectX11Example WIN32 DirectX11Example.cpp Util.h Util.cpp DXUtil.h DXUtil.cpp)

if (NOT "${CppExampleBuildInTree}")
	# Copy DLL to binary folder so it can be found when the compiled program runs
	set(FoveDLL "${FOVE_SDK_FOLDER}/bin/FoveClient.dll")
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
