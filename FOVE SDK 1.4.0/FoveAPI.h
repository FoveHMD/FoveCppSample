////////////////////
// FOVE C/C++ API //
////////////////////

// This header declares both the C and C++ APIs.

#ifndef FOVE_API_H
#define FOVE_API_H

// Disable clang format on this file so that it isn't changed from its canonical form when included in projects using clang-format.
// clang-format off

/////////////////////////////////////////////////////////////////////////////////
// Macros -----------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////

//! Macro that controls whether the C++ API will be defined
/*!
	The C API is always defined after including this header, but the C++ API is optional.
	In C++ mode, the C API is extended (for example, with default values for struct members) automatically.

	User can set this macro to 0 or 1, either in code before including this, or via compile-flag to choose.

	Defaults to 0 if the compiler is a C compiler, 1 if the compiler is a C++ compiler.
*/
#if !defined(FOVE_DEFINE_CXX_API) || defined(FOVE_DOXYGEN)
	#ifdef __cplusplus
		#define FOVE_DEFINE_CXX_API 1
	#else
		#define FOVE_DEFINE_CXX_API 0
	#endif
#endif

//! Define this to empty to remove noexcept specifiers from the API
#ifndef FOVE_NOEXCEPT
	#ifdef __cplusplus
		#define FOVE_NOEXCEPT noexcept
	#else
		#define FOVE_NOEXCEPT
	#endif
#endif

//! Defines any needed modifiers to function pointers for callbacks, e.g. to use a different calling convention
#ifndef FOVE_CALLBACK
	#ifdef _MSC_VER
		#define FOVE_CALLBACK __stdcall
	#else
		#define FOVE_CALLBACK
	#endif
#endif

//! Helper for disabling name mangling, etc
#ifndef FOVE_EXTERN_C
	#ifdef __cplusplus
		#define FOVE_EXTERN_C extern "C"
	#else
		#define FOVE_EXTERN_C
	#endif
#endif

//! Helper for declaring the public visibility of a function
#ifndef FOVE_EXPORT
	#ifdef __GNUC__
		#define FOVE_EXPORT FOVE_EXTERN_C __attribute__((visibility("default")))
	#elif defined(_MSC_VER)
		#define FOVE_EXPORT FOVE_EXTERN_C __declspec(dllexport)
	#else
		#define FOVE_EXPORT FOVE_EXTERN_C
	#endif
#endif

//! Helper for declaring a compile time constant
#ifndef FOVE_CONSTEXPR
	#ifdef __cplusplus
		#define FOVE_CONSTEXPR constexpr
	#else
		#define FOVE_CONSTEXPR const
	#endif
#endif

//! Helper to start an API enum
#ifndef FOVE_ENUM
	#if FOVE_DEFINE_CXX_API
		#define FOVE_ENUM(enumName) enum class Fove_ ## enumName
	#else
		#define FOVE_ENUM(enumName) typedef enum
	#endif
#endif

//! Helper to add a value to an API enum
#ifndef FOVE_ENUM_VAL
	#if FOVE_DEFINE_CXX_API
		#define FOVE_ENUM_VAL(enumName, valueName) valueName
	#else
		#define FOVE_ENUM_VAL(enumName, valueName) Fove_ ## enumName ## _ ## valueName
	#endif
#endif

//! Helper to finish an API enum
#ifndef FOVE_ENUM_END
	#if FOVE_DEFINE_CXX_API
		#define FOVE_ENUM_END(enumName) ; namespace FOVE_CXX_NAMESPACE { using enumName = Fove_ ## enumName; }
	#else
		#define FOVE_ENUM_END(enumName) Fove_ ## enumName
	#endif
#endif

//! Helper to start an API struct
#ifndef FOVE_STRUCT
	#if FOVE_DEFINE_CXX_API
		#define FOVE_STRUCT(structName) struct Fove_ ## structName
	#else
		#define FOVE_STRUCT(structName) typedef struct
	#endif
#endif

//! Helper to add a member to an API struct
#ifndef FOVE_STRUCT_VAL
	#if FOVE_DEFINE_CXX_API
		#define FOVE_STRUCT_VAL(memberName, defaultVal) memberName = defaultVal
	#else
		#define FOVE_STRUCT_VAL(memberName, defaultVal) memberName
	#endif
#endif

//! Define this to use std::array instead of C arrays
#ifndef FOVE_USE_STD_ARRAY
	#define FOVE_USE_STD_ARRAY 0
#endif

//! Helper to add a member array to an API struct
#ifndef FOVE_STRUCT_ARRAY
	#if FOVE_USE_STD_ARRAY
		#define FOVE_STRUCT_ARRAY(Type, size, memberName, defaultVal) static_assert(sizeof(std::array<Type, size>) == sizeof(Type) * size); std::array<Type, size> FOVE_STRUCT_VAL(memberName, defaultVal)
	#else
		#define FOVE_STRUCT_ARRAY(Type, size, memberName, defaultVal) Type FOVE_STRUCT_VAL(memberName[size], defaultVal)
	#endif
#endif

//! Helper to get the c pointer from an item defined via FOVE_STRUCT_ARRAY
#ifndef FOVE_STRUCT_ARRAY_CPTR
	#if FOVE_USE_STD_ARRAY
		#define FOVE_STRUCT_ARRAY_CPTR(member) member.data()
	#else
		#define FOVE_STRUCT_ARRAY_CPTR(member) member
	#endif
#endif

//! Helper to complete an API struct
#ifndef FOVE_STRUCT_END
	#if FOVE_DEFINE_CXX_API
		#define FOVE_STRUCT_END(structName) ; namespace FOVE_CXX_NAMESPACE { using structName = Fove_ ## structName; }
	#else
		#define FOVE_STRUCT_END(structName) Fove_ ## structName
	#endif
#endif

//! Helper to complete an API struct without aliasing it in C++
#ifndef FOVE_STRUCT_END_NO_CXX_ALIAS
	#if FOVE_DEFINE_CXX_API
		#define FOVE_STRUCT_END_NO_CXX_ALIAS(structName)
	#else
		#define FOVE_STRUCT_END_NO_CXX_ALIAS(structName) Fove_ ## structName
	#endif
#endif

//! Helper for declaring deprecated functions
#ifndef FOVE_DEPRECATED
	#ifdef __GNUC__
		#define FOVE_DEPRECATED(func, rem) __attribute__ ((deprecated(rem))) func
	#elif defined(_MSC_VER)
		#define FOVE_DEPRECATED(func, rem) __declspec(deprecated(rem)) func
	#else
		#define FOVE_DEPRECATED(func, rem) func
	#endif
#endif

#if FOVE_DEFINE_CXX_API // C++ API specific macros are contained within here

// Namespace that C++ API will be put in
/*!
	Since the C++ API is header-only, the functions in it do not need to be in any particular namespace.

	By default, everything is put in the Fove namespace, but this can be customized if the user prefers.
*/
#ifndef FOVE_CXX_NAMESPACE
	#define FOVE_CXX_NAMESPACE Fove
#endif

// Determines if exceptions are enabled or not in the C++ API
/*!
	Exceptions are enabled by default in the C++ API, because they are a core C++ feature.
	However, they are only used in specific functions, not everywhere.
	If you are working in a code base that bans exceptions, define FOVE_EXCEPTIONS to zero.
	Exceptions are automatically disabled for the unreal engine.
*/
#ifndef FOVE_EXCEPTIONS
	#ifndef __UNREAL__
		#define FOVE_EXCEPTIONS 1
	#else
		#define FOVE_EXCEPTIONS 0
	#endif
#endif

#endif // FOVE_DEFINE_CXX_API

/////////////////////////////////////////////////////////////////////////////////
// Standard includes ------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////

// The FOVE API has no dependencies other than the standard library

#if FOVE_DEFINE_CXX_API
#include <exception> // For std::exception
#include <string>    // For std::string
#include <utility>   // For std::move
#include <vector>    // For std::vector
#endif

#if FOVE_USE_STD_ARRAY
#include <array>
#endif

#include <stdbool.h> // Pull in the bool type when using C
#include <stddef.h>  // Pull in size_t
#include <stdint.h>  // Pull in fixed length types

/////////////////////////////////////////////////////////////////////////////////
// Doxygen Main Page ------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////

/*! @file FoveAPI.h
	@brief This file contains the entire FOVE API, no other headers are needed
*/

#if FOVE_DEFINE_CXX_API
/*! \mainpage FOVE C++ API Documentation
	\section intro_sec Introduction

	This is the documentation for the FOVE C++ API.

	This API allows client applications to interface with the FOVE runtime system, including
	headsets, eye tracking, position tracking, and the compositor.

	An example of using this API can be found at https://github.com/FoveHMD/FoveCppSample

	The C++ API wraps, and includes the FOVE C API.
	Items within the Fove namespace are part of the wrapper, though many are simply typedefs.
	Items outside the namespace are part of the C API, though in C++ mode, more features may be added (such as default values for struct members).

	The main place to get started is looking at the following classes:
	@ref Fove::Headset
	@ref Fove::Compositor

	\section install_sec Installation

	To use the API, simply drop FoveAPI.h into your project and include it.
	After that point you will be able to use all capabilities listed here.

	To link, simply add the FOVE shared library (or DLL on Windows) to the link libraries for your project.

	\section requirements_sec Requirements

	This API requires C++11 or later

	\section backcompat_sec Backwards Compatibility

	Except where noted in the API, the FOVE system maintains backwards compatibility with old clients.
	For example, a v0.15.0 client can talk a a v0.16.0 server.

	Forwards compatibility is not provided: a client using v1.0.0 won't function on a computer with v0.17.0 installed.
	The end user will need to update their FOVE version first.

	Updates to the patch version (the third number in the version) generally do not affect compatibility.
	So v1.0.0 and v1.0.1 are considered equivalent as far as compatibility is concerned.
	Though we may fix bugs that involve regressions in backwards compatibility in a patch version.

	ABI compatibility is not kept between different versions of this API.
	If you update the client library binary, you also need to recompile against the corresponding version of this header.
	This is something we may consider in the future.

	\section Error Handling

	By policy, every C API function returns an error code, which is from the #Fove_ErrorCode.

	The common errors for each function are documented, but it's possible for other errors to be returned.
	In particular, any function may return #Fove_ErrorCode_UnknownError, which indicates an internal bug in the implementation.

	The precedence of errors is mostly unspecified, but this is the typical precedence:

	API_NotRegistered -> [Connect_NotConnected / Data_NoUpdate] -> All other errors -> None

	All getters (functions that start with "get" or "is") return Data_NoUpdate and never Connect_NotConnected.
	Other functions will never return Data_NoUpdate, but any ones that require a connection can return Connect_NotConnected.

	Some getters return a value cached by one of the "fetch" functions. Since the fetch functions also cache an error,
	these functions return the error as of the last fetch.

	\section Threading

	All functions in the C API are thread safe, except where otherwise noted.

	Thus, it is safe to call any function(s) from multiple threads simultaneously.
	However, care should still be taken to avoid race conditions in multi-threaded applications.

	For example, if you call a function that needs some registered capability,
	simultaneously while registering that capability, then you have a race condition.
	The order of the two operations, and thus the behavior, may change randomly at runtime.
	You should synchronize the calls or otherwise handle both cases.

	The C++ wrapper, for example Object<> and Result<>, do not provide any thread safety themselves.
	However, insofar as they invoke the C API, they are thread safe.
	Since the C++ wrapper is made up of brief inline functions, you can easily check any given function.

	All getters (functions that start with "get" or "is") return quickly without waiting for network access.
	It is fine to call them every frame in a frame loop.

	Most other functions access the service over a socket, with a small timeout, and thus can block for a small while,
	especially if the service is not running.

	\section Miscellaneous

	Unless otherwise specified, all strings are encoded in UTF-8.
	*/
#else
/*! \mainpage FOVE C API Documentation
	\section intro_sec Introduction

	This is the documentation for the FOVE C API.

	This API allows client applications to interface with the FOVE runtime system, including
	headsets, eye tracking, position tracking, and the compositor.

	Various higher-level bindings are provided by FOVE, such as C++ and C#.

	\section install_sec Installation

	To use the API, simply drop FoveAPI.h into you project and include it.
	After that point you will be able to use all capabilities listed here.

	To link, simply add the FOVE shared library (or DLL on Windows) to the link libraries for your project.

	\section requirements_sec Requirements

	This API requires C99 or later

	\section backcompat_sec Backwards Compatibility

	Except where noted in the API, the FOVE system maintains backwards compatibility with old clients.
	For example, a v0.15.0 client can talk a a v0.16.0 server.

	Forwards compatibility is not provided: a client using v1.0.0 wont function on a computer with v0.17.0 installed.
	The end user will need to update their FOVE version first.

	Updates to the patch version (the third number in the version) generally do not affect compatibility.
	So v1.0.0 and v1.0.1 are considered equivalent as far as compatibility is concerned.
	Though we may fix bugs that involve regressions in backwards compatibility in a patch version.

	ABI compatibility is not kept between different versions of this API.
	If you update the client library binary, you also need to recompile against the corresponding version of this header.
	This is something we may consider in the future.

	\section Error Handling

	By policy, every API function returns an error code, which is from the #Fove_ErrorCode.

	The common errors for each function are documented, but it's possible for other errors to be returned.
	In particular, any function may return #Fove_ErrorCode_UnknownError, which indicates an internal bug in the implementation.

	The precedence of errors is mostly unspecified, but this is the typical precedence:

	API_NotRegistered -> [Connect_NotConnected / Data_NoUpdate] -> All other errors -> None

	All getters (functions that start with "get" or "is") return Data_NoUpdate and never Connect_NotConnected.
	Other functions will never return Data_NoUpdate, but any ones that require a connection can return Connect_NotConnected.

	Some getters return a value cached by one of the "fetch" functions. Since the fetch functions also cache an error,
	these functions return the error as of the last fetch.

	\section Threading

	All functions in the C API are thread safe, except where otherwise noted.

	Thus, it is safe to call any function(s) from multiple threads simultaneously.
	However, care should still be taken to avoid race conditions in multi-threaded applications.

	For example, if you call a function that needs some registered capability,
	simultaneously while registering that capability, then you have a race condition.
	The order of the two operations, and thus the behavior, may change randomly at runtime.
	You should synchronize the calls or otherwise handle both cases.

	All getters (functions that start with "get" or "is") return quickly without waiting for network access.
	It is fine to call them every frame in a frame loop.

	Most other functions access the service over a socket, with a small timeout, and thus can block for a small while,
	especially if the service is not running.

	\section Miscellaneous

	Unless otherwise specified, all strings are encoded in UTF-8.
*/
#endif

/////////////////////////////////////////////////////////////////////////////////
// Fove C/C++ Shared Types ------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////

//! List of capabilities usable by clients
/*!
	Most features require registering for the relevant capability.
	If a client queries data related to a capability it has not registered API_NotRegistered will be returned.
	After a new capability registration the Data_NoUpdate error may be returned for a few frames while
	the service is bootstrapping the new capability.

	This enum is designed to be used as a flag set, so items may be binary logic operators like |.

	The FOVE runtime will keep any given set of hardware/software running so long as one client is registering a capability.

	The registration of a capability does not necessarily mean that the capability is running.
	For example, if no position tracking camera is attached, no position tracking will occur regardless of how many clients registered for it.
*/
FOVE_ENUM(ClientCapabilities)
{
	FOVE_ENUM_VAL(ClientCapabilities, None) = 0,                       //!< No capabilities requested
	FOVE_ENUM_VAL(ClientCapabilities, OrientationTracking) = 1 << 0,   //!< Enables headset orientation tracking
	FOVE_ENUM_VAL(ClientCapabilities, PositionTracking) = 1 << 1,      //!< Enables headset position tracking
	FOVE_ENUM_VAL(ClientCapabilities, PositionImage) = 1 << 2,         //!< Enables Position camera image transfer from the runtime service to the client
	FOVE_ENUM_VAL(ClientCapabilities, EyeTracking) = 1 << 3,           //!< Enables headset eye tracking
	FOVE_ENUM_VAL(ClientCapabilities, GazeDepth) = 1 << 4,             //!< Enables gaze depth computation
	FOVE_ENUM_VAL(ClientCapabilities, UserPresence) = 1 << 5,          //!< Enables user presence detection
	FOVE_ENUM_VAL(ClientCapabilities, UserAttentionShift) = 1 << 6,    //!< Enables user attention shift computation
	FOVE_ENUM_VAL(ClientCapabilities, UserIOD) = 1 << 7,               //!< Enables the calculation of the user IOD
	FOVE_ENUM_VAL(ClientCapabilities, UserIPD) = 1 << 8,               //!< Enables the calculation of the user IPD
	FOVE_ENUM_VAL(ClientCapabilities, EyeTorsion) = 1 << 9,            //!< Enables the calculation of the user eye torsion
	FOVE_ENUM_VAL(ClientCapabilities, EyeShape) = 1 << 10,             //!< Enables the detection of the eyes shape
	FOVE_ENUM_VAL(ClientCapabilities, EyesImage) = 1 << 11,            //!< Enables Eye camera image transfer from the runtime service to the client
	FOVE_ENUM_VAL(ClientCapabilities, EyeballRadius) = 1 << 12,        //!< Enables the calculation of the user eyeball radius
	FOVE_ENUM_VAL(ClientCapabilities, IrisRadius) = 1 << 13,           //!< Enables the calculation of the user iris radius
	FOVE_ENUM_VAL(ClientCapabilities, PupilRadius) = 1 << 14,          //!< Enables the calculation of the user pupil radius
	FOVE_ENUM_VAL(ClientCapabilities, GazedObjectDetection) = 1 << 15, //!< Enables gazed object detection based on registered gazable objects
	FOVE_ENUM_VAL(ClientCapabilities, DirectScreenAccess) = 1 << 16,   //!< Give you direct access to the HMD screen and disable the Fove compositor
	FOVE_ENUM_VAL(ClientCapabilities, PupilShape) = 1 << 17,           //!< Enables the detection of the pupil shape
	FOVE_ENUM_VAL(ClientCapabilities, EyeBlink) = 1 << 18,             //!< Enables eye blink detection and counting
} FOVE_ENUM_END(ClientCapabilities);

//! The error codes that the Fove system may return
FOVE_ENUM(ErrorCode)
{
	FOVE_ENUM_VAL(ErrorCode, None) = 0, //!< Indicates that no error occurred

	// Connection Errors
	FOVE_ENUM_VAL(ErrorCode, Connect_NotConnected) = 7,         //!< The client lost the connection with the Fove service
	FOVE_ENUM_VAL(ErrorCode, Connect_RuntimeVersionTooOld) = 4, //!< The FOVE runtime version is too old for this client
	FOVE_ENUM_VAL(ErrorCode, Connect_ClientVersionTooOld) = 6,  //!< The client version is too old for the installed runtime

	// API usage errors
	FOVE_ENUM_VAL(ErrorCode, API_InvalidArgument) = 103,          //!< An argument passed to an API function was invalid for a reason not listed below
	FOVE_ENUM_VAL(ErrorCode, API_NotRegistered) = 104,            //!< Data was queried without first registering for that data
	FOVE_ENUM_VAL(ErrorCode, API_NullInPointer) = 110,            //!< An input argument passed to an API function was invalid for a reason other than the below reasons
	FOVE_ENUM_VAL(ErrorCode, API_InvalidEnumValue) = 111,         //!< An enum argument passed to an API function was invalid
	FOVE_ENUM_VAL(ErrorCode, API_NullOutPointersOnly) = 120,      //!< All output arguments were null on a function that requires at least one output (all getters that have no side effects)
	FOVE_ENUM_VAL(ErrorCode, API_OverlappingOutPointers) = 121,   //!< Two (or more) output parameters passed to an API function overlap in memory. Each output parameter should be a unique, separate object
	FOVE_ENUM_VAL(ErrorCode, API_MissingArgument) = 123,          //!< The service was expecting extra arguments that the client didn't provide
	FOVE_ENUM_VAL(ErrorCode, API_AlreadyInTheDesiredState) = 124, //!< For functions that request some state change, this indicates that no action was taken due to being in the correct state
	FOVE_ENUM_VAL(ErrorCode, API_Timeout) = 130,                  //!< A call to an API could not be completed within a timeout

	// Data Errors
	FOVE_ENUM_VAL(ErrorCode, Data_Unreadable) = 1002,   //!< The data couldn't be read properly from the shared memory and may be corrupted
	FOVE_ENUM_VAL(ErrorCode, Data_NoUpdate) = 1003,     //!< The data has not been updated by the system yet and is invalid
	FOVE_ENUM_VAL(ErrorCode, Data_Uncalibrated) = 1004, //!< The data is invalid because the feature in question is not calibrated
	FOVE_ENUM_VAL(ErrorCode, Data_Unreliable) = 1006,   //!< The data is unreliable because the eye tracking has been lost
	FOVE_ENUM_VAL(ErrorCode, Data_LowAccuracy) = 1007,  //!< The accuracy of the data is low

	// Hardware Errors
	FOVE_ENUM_VAL(ErrorCode, Hardware_Disconnected) = 2006,         //!> The hardware has been physically disconnected
	FOVE_ENUM_VAL(ErrorCode, Hardware_WrongFirmwareVersion) = 2007, //!> A wrong version of hardware firmware has been detected

	// Code and placeholders
	FOVE_ENUM_VAL(ErrorCode, Code_NotImplementedYet) = 4000,  //!> The function hasn't been implemented yet
	FOVE_ENUM_VAL(ErrorCode, Code_FunctionDeprecated) = 4001, //!> The function has been deprecated

	// Position Tracking
	FOVE_ENUM_VAL(ErrorCode, Position_ObjectNotTracked) = 5008, //!> The object is inactive or currently not tracked

	// Compositor
	FOVE_ENUM_VAL(ErrorCode, Compositor_NotSwapped) = 122,                      //!< This comes from submitting without calling WaitForRenderPose after a complete submit
	FOVE_ENUM_VAL(ErrorCode, Compositor_UnableToCreateDeviceAndContext) = 8000, //!< Compositor was unable to initialize its backend component
	FOVE_ENUM_VAL(ErrorCode, Compositor_UnableToUseTexture) = 8001,             //!< Compositor was unable to use the given texture (likely due to mismatched client and data types or an incompatible format)
	FOVE_ENUM_VAL(ErrorCode, Compositor_DeviceMismatch) = 8002,                 //!< Compositor was unable to match its device to the texture's, either because of multiple GPUs or a failure to get the device from the texture
	FOVE_ENUM_VAL(ErrorCode, Compositor_DisconnectedFromRuntime) = 8006,        //!< Compositor was running and is no longer responding
	FOVE_ENUM_VAL(ErrorCode, Compositor_ErrorCreatingTexturesOnDevice) = 8008,  //!< Failed to create shared textures for compositor
	FOVE_ENUM_VAL(ErrorCode, Compositor_NoEyeSpecifiedForSubmit) = 8009,        //!< The supplied Fove_Eye for submit is invalid (i.e. is Both or Neither)

	// Generic
	FOVE_ENUM_VAL(ErrorCode, UnknownError) = 9000, //!< Errors that are unknown or couldn't be classified. If possible, info will be logged about the nature of the issue

	// Objects
	FOVE_ENUM_VAL(ErrorCode, Object_AlreadyRegistered) = 10000, //!< The scene object that you attempted to register was already present in the object registry

	// Rendering
	FOVE_ENUM_VAL(ErrorCode, Render_OtherRendererPrioritized) = 11000, //!< Another renderer registered to render the process have a higher priority than current client

	// License
	FOVE_ENUM_VAL(ErrorCode, License_FeatureAccessDenied) = 12000, //!< You don't have the license rights to use the corresponding feature
	FOVE_ENUM_VAL(ErrorCode, License_Expired) = 12001,             //!< Your license is expired already
	FOVE_ENUM_VAL(ErrorCode, License_ClockError) = 12003,          //!< Your local clock is invalid
	FOVE_ENUM_VAL(ErrorCode, License_TooManyActivations) = 12004,  //!< Your license has been activated too many times
	FOVE_ENUM_VAL(ErrorCode, License_Revoked) = 12005,             //!< Your license has been revoked

	// Profiles
	FOVE_ENUM_VAL(ErrorCode, Profile_DoesntExist) = 13000,  //!< The profile doesn't exist
	FOVE_ENUM_VAL(ErrorCode, Profile_NotAvailable) = 13001, //!< The profile already exists when it shouldn't, or is otherwise taken or not available
	FOVE_ENUM_VAL(ErrorCode, Profile_InvalidName) = 13002,  //!< The profile name is not a valid name

	// Config
	FOVE_ENUM_VAL(ErrorCode, Config_DoesntExist) = 14000,  //!< The provided key doesn't exist in the config
	FOVE_ENUM_VAL(ErrorCode, Config_TypeMismatch) = 14001, //!< The value type of the key doesn't match

	// System Errors, errors that originate from the OS level API (files, sockets, etc)
	FOVE_ENUM_VAL(ErrorCode, System_UnknownError) = 15000, //!< Any system error not otherwise specified
	FOVE_ENUM_VAL(ErrorCode, System_PathNotFound) = 15001, //!< Unix: ENOENT, Windows: ERROR_PATH_NOT_FOUND or ERROR_FILE_NOT_FOUND
	FOVE_ENUM_VAL(ErrorCode, System_AccessDenied) = 15002, //!< Unix: EACCES, Windows: ERROR_ACCESS_DENIED
} FOVE_ENUM_END(ErrorCode);

//! Compositor layer type, which defines how clients are composited
FOVE_ENUM(CompositorLayerType)
{
	FOVE_ENUM_VAL(CompositorLayerType, Base) = 0,            //!< The first and main application layer
	FOVE_ENUM_VAL(CompositorLayerType, Overlay) = 0x10000,   //!< Layer over the base layer
	FOVE_ENUM_VAL(CompositorLayerType, Diagnostic) = 0x20000 //!< Layer over Overlay layer
} FOVE_ENUM_END(CompositorLayerType);

//! Predefined object ID to signify "no object"
FOVE_CONSTEXPR int fove_ObjectIdInvalid = -1;

//! The groups of objects of the scene
FOVE_ENUM(ObjectGroup)
{
	FOVE_ENUM_VAL(ObjectGroup, Group0) = 1 << 0,
	FOVE_ENUM_VAL(ObjectGroup, Group1) = 1 << 1,
	FOVE_ENUM_VAL(ObjectGroup, Group2) = 1 << 2,
	FOVE_ENUM_VAL(ObjectGroup, Group3) = 1 << 3,
	FOVE_ENUM_VAL(ObjectGroup, Group4) = 1 << 4,
	FOVE_ENUM_VAL(ObjectGroup, Group5) = 1 << 5,
	FOVE_ENUM_VAL(ObjectGroup, Group6) = 1 << 6,
	FOVE_ENUM_VAL(ObjectGroup, Group7) = 1 << 7,
	FOVE_ENUM_VAL(ObjectGroup, Group8) = 1 << 8,
	FOVE_ENUM_VAL(ObjectGroup, Group9) = 1 << 9,
	FOVE_ENUM_VAL(ObjectGroup, Group10) = 1 << 10,
	FOVE_ENUM_VAL(ObjectGroup, Group11) = 1 << 11,
	FOVE_ENUM_VAL(ObjectGroup, Group12) = 1 << 12,
	FOVE_ENUM_VAL(ObjectGroup, Group13) = 1 << 13,
	FOVE_ENUM_VAL(ObjectGroup, Group14) = 1 << 14,
	FOVE_ENUM_VAL(ObjectGroup, Group15) = 1 << 15,
	FOVE_ENUM_VAL(ObjectGroup, Group16) = 1 << 16,
	FOVE_ENUM_VAL(ObjectGroup, Group17) = 1 << 17,
	FOVE_ENUM_VAL(ObjectGroup, Group18) = 1 << 18,
	FOVE_ENUM_VAL(ObjectGroup, Group19) = 1 << 19,
	FOVE_ENUM_VAL(ObjectGroup, Group20) = 1 << 20,
	FOVE_ENUM_VAL(ObjectGroup, Group21) = 1 << 21,
	FOVE_ENUM_VAL(ObjectGroup, Group22) = 1 << 22,
	FOVE_ENUM_VAL(ObjectGroup, Group23) = 1 << 23,
	FOVE_ENUM_VAL(ObjectGroup, Group24) = 1 << 24,
	FOVE_ENUM_VAL(ObjectGroup, Group25) = 1 << 25,
	FOVE_ENUM_VAL(ObjectGroup, Group26) = 1 << 26,
	FOVE_ENUM_VAL(ObjectGroup, Group27) = 1 << 27,
	FOVE_ENUM_VAL(ObjectGroup, Group28) = 1 << 28,
	FOVE_ENUM_VAL(ObjectGroup, Group29) = 1 << 29,
	FOVE_ENUM_VAL(ObjectGroup, Group30) = 1 << 30,
	FOVE_ENUM_VAL(ObjectGroup, Group31) = 1 << 31,
} FOVE_ENUM_END(ObjectGroup);

//! Struct to list various version info about the FOVE software
/*! Contains the version for the software (both runtime and client versions).
	A negative value in any int field represents unknown.
*/
FOVE_STRUCT(Versions)
{
	int FOVE_STRUCT_VAL(clientMajor, -1);                //!< The major version number of the client library
	int FOVE_STRUCT_VAL(clientMinor, -1);                //!< The minor version number of the client library
	int FOVE_STRUCT_VAL(clientBuild, -1);                //!< The build version number of the client library
	int FOVE_STRUCT_VAL(clientYear, -1);                 //!< The build date of the client library
	int FOVE_STRUCT_VAL(clientMonth, -1);                //!< The build date of the client library (Range: 1 to 12)
	int FOVE_STRUCT_VAL(clientDay, -1);                  //!< The build date of the client library (Range: 1 to 31)
	int FOVE_STRUCT_VAL(clientProtocol, -1);             //!< The version of the communication protocol the client is using
	FOVE_STRUCT_ARRAY(char, 64, clientHash, {});         //!< Null-terminated utf8 unique identifier of the client library version (usually a 40-hex git hash)
	int FOVE_STRUCT_VAL(runtimeMajor, -1);               //!< The major version number of the fove service
	int FOVE_STRUCT_VAL(runtimeMinor, -1);               //!< The minor version number of the fove service
	int FOVE_STRUCT_VAL(runtimeBuild, -1);               //!< The build version number of the fove service
	int FOVE_STRUCT_VAL(runtimeYear ,-1);                //!< The build date of the fove service
	int FOVE_STRUCT_VAL(runtimeMonth ,-1);               //!< The build date of the fove service (Range: 1 to 12)
	int FOVE_STRUCT_VAL(runtimeDay ,-1);                 //!< The build date of the fove service (Range: 1 to 31)
	FOVE_STRUCT_ARRAY(char, 64, runtimeHash, {});        //!< Null-terminated utf8 unique identifier of the fove service, or empty if unknown
	int FOVE_STRUCT_VAL(firmware, -1);                   //!< The firmware version number
	int FOVE_STRUCT_VAL(maxFirmware, -1);                //!< Indicate the highest compatible firmware version
	int FOVE_STRUCT_VAL(minFirmware, -1);                //!< Indicate the lowest compatible firmware version
	bool FOVE_STRUCT_VAL(tooOldHeadsetConnected, false); //!< Indicate whether the connected headset is too old or not
} FOVE_STRUCT_END(Versions);

//! Struct with details about a FOVE license
FOVE_STRUCT(LicenseInfo)
{
	FOVE_STRUCT_ARRAY(unsigned char, 16, uuid, {});      //!< 128-bit UUID of this license, in binary form
	int FOVE_STRUCT_VAL(expirationYear, 0);              //!< Expiration, year (eg. 2028), 0 if there is no expiration
	int FOVE_STRUCT_VAL(expirationMonth, 0);             //!< Expiration month (1 - 12), 0 if there is no expiration
	int FOVE_STRUCT_VAL(expirationDay, 0);               //!< Expiration day (1 - 31), 0 if there is no expiration
	FOVE_STRUCT_ARRAY(char, 128, licenseType, {});       //!< Null-termianted type of license, such as "Professional"
	FOVE_STRUCT_ARRAY(char, 256, licensee, {});          //!< Null-terminated name of the person or organization that this license is for, truncated as needed
} FOVE_STRUCT_END(LicenseInfo);

//! Struct Contains hardware information for the headset
/*! Contains the serial number, manufacturer and model name for the headset.
	Values of the member fields originates from their UTF-8 string representations
	defined by headset manufacturers, and passed to us (FoveClient) by FoveService
	server through an IPC message.
	The server may be sending very long strings, but the FoveClient library will
	be truncating them in an unspecified manner to 0-terminated strings of length
	at most 256.
*/
FOVE_STRUCT(HeadsetHardwareInfo)
{
	FOVE_STRUCT_ARRAY(char, 256, serialNumber, {}); //!< Serial number, as a null-terminated UTF-8 string
	FOVE_STRUCT_ARRAY(char, 256, manufacturer, {}); //!< Manufacturer info, as a null-terminated UTF-8 string
	FOVE_STRUCT_ARRAY(char, 256, modelName, {});    //!< Model name, as a null-terminated UTF-8 string
} FOVE_STRUCT_END_NO_CXX_ALIAS(HeadsetHardwareInfo);

//! Struct representation on a quaternion
/*! A quaternion represents an orientation in 3D space.*/
FOVE_STRUCT(Quaternion)
{
	float FOVE_STRUCT_VAL(x, 0); //!< X component of the quaternion
	float FOVE_STRUCT_VAL(y, 0); //!< Y component of the quaternion
	float FOVE_STRUCT_VAL(z, 0); //!< Z component of the quaternion
	float FOVE_STRUCT_VAL(w, 1); //!< W component of the quaternion

#if FOVE_DEFINE_CXX_API // Mostly for MSVC 2015 which doesn't properly implement brace-initialization of structs
	Fove_Quaternion(float xx = 0, float yy = 0, float zz = 0, float ww = 1) : x(xx), y(yy), z(zz), w(ww) {}
#endif
} FOVE_STRUCT_END(Quaternion);

//! Struct to represent a 3D-vector
/*! A vector that represents a position in 3D space. */
FOVE_STRUCT(Vec3)
{
	float FOVE_STRUCT_VAL(x, 0); //!< X component of the vector
	float FOVE_STRUCT_VAL(y, 0); //!< Y component of the vector
	float FOVE_STRUCT_VAL(z, 0); //!< Z component of the vector

#if FOVE_DEFINE_CXX_API // Mostly for MSVC 2015 which doesn't properly implement brace-initialization of structs
	Fove_Vec3(float xx = 0, float yy = 0, float zz = 0) : x(xx), y(yy), z(zz) {}
#endif
} FOVE_STRUCT_END(Vec3);

//! Struct to represent a 2D-vector
/*! A vector that represents a position or orientation in 2D space, such as screen or image coordinates. */
FOVE_STRUCT(Vec2)
{
	float FOVE_STRUCT_VAL(x, 0); //!< X component of the vector
	float FOVE_STRUCT_VAL(y, 0); //!< Y component of the vector

#if FOVE_DEFINE_CXX_API // Mostly for MSVC 2015 which doesn't properly implement brace-initialization of structs
	Fove_Vec2(float xx = 0, float yy = 0): x(xx), y(yy) {}
#endif
} FOVE_STRUCT_END(Vec2);

//! Struct to represent a 2D-vector of integers
FOVE_STRUCT(Vec2i)
{
	int FOVE_STRUCT_VAL(x, 0); //!< X component of the vector
	int FOVE_STRUCT_VAL(y, 0); //!< Y component of the vector

#if FOVE_DEFINE_CXX_API // Mostly for MSVC 2015 which doesn't properly implement brace-initialization of structs
	Fove_Vec2i(int xx = 0, int yy = 0) : x(xx), y(yy) {}
#endif
} FOVE_STRUCT_END(Vec2i);

//! Struct to represent a Ray
/*! Stores the start point and direction of a Ray */
FOVE_STRUCT(Ray)
{
	//! The start point of the Ray
	Fove_Vec3 FOVE_STRUCT_VAL(origin, (Fove_Vec3{0, 0, 0}));
	//! The direction of the Ray
	Fove_Vec3 FOVE_STRUCT_VAL(direction, (Fove_Vec3{0, 0, 1}));
} FOVE_STRUCT_END(Ray);

//! A frame timestamp information
/*! It is returned by every update function so that you can know which frame the new data correspond to
*/
FOVE_STRUCT(FrameTimestamp)
{
	//! Incremental frame counter
	uint64_t FOVE_STRUCT_VAL(id, 0);
	//! The time at which the data was captured, in microseconds since an unspecified epoch
	uint64_t FOVE_STRUCT_VAL(timestamp, 0);
} FOVE_STRUCT_END(FrameTimestamp);

//! Struct to represent a combination of position and orientation of Fove Headset
/*! This structure is a combination of the Fove headset position and orientation in 3D space, collectively known as the "pose".
	In the future this may also contain acceleration information for the headset, and may also be used for controllers.
*/
FOVE_STRUCT(Pose)
{
	//! Incremental counter which tells if the coord captured is a fresh value at a given frame
	uint64_t FOVE_STRUCT_VAL(id, 0);
	//! The time at which the pose was captured, in microseconds since an unspecified epoch
	uint64_t FOVE_STRUCT_VAL(timestamp, 0);
	//! The Quaternion which represents the orientation of the head
	Fove_Quaternion FOVE_STRUCT_VAL(orientation, {});
	//! The angular velocity of the head
	Fove_Vec3 FOVE_STRUCT_VAL(angularVelocity, {});
	//! The angular acceleration of the head
	Fove_Vec3 FOVE_STRUCT_VAL(angularAcceleration, {});
	//! The position of headset in 3D space. Tares to (0, 0, 0). Use for sitting applications
	Fove_Vec3 FOVE_STRUCT_VAL(position, {});
	//! The position of headset including offset for camera location. Will not tare to zero. Use for standing applications
	Fove_Vec3 FOVE_STRUCT_VAL(standingPosition, {});
	//! The velocity of headset in 3D space
	Fove_Vec3 FOVE_STRUCT_VAL(velocity, {});
	//! The acceleration of headset in 3D space
	Fove_Vec3 FOVE_STRUCT_VAL(acceleration, {});
} FOVE_STRUCT_END(Pose);

//! Severity level of log messages
FOVE_ENUM(LogLevel)
{
	FOVE_ENUM_VAL(LogLevel, Debug) = 0,   //!< Debug information
	FOVE_ENUM_VAL(LogLevel, Warning) = 1, //!< An issue requiring attention
	FOVE_ENUM_VAL(LogLevel, Error) = 2,   //!< An unexpected error
} FOVE_ENUM_END(LogLevel);

//! Enum specifying the left or right eye
FOVE_ENUM(Eye)
{
	FOVE_ENUM_VAL(Eye, Left) = 0,  //!< Left eye
	FOVE_ENUM_VAL(Eye, Right) = 1, //!< Right eye
} FOVE_ENUM_END(Eye);

//! Enum specifying the state of an eye
FOVE_ENUM(EyeState)
{
	FOVE_ENUM_VAL(EyeState, NotDetected) = 0, //!< The eye is missing or the tracking was lost
	FOVE_ENUM_VAL(EyeState, Opened) = 1,      //!< The eye is present and opened
	FOVE_ENUM_VAL(EyeState, Closed) = 2,      //!< The eye is present and closed
} FOVE_ENUM_END(EyeState);

//! Struct to hold a rectangular array
FOVE_STRUCT(Matrix44)
{
	float FOVE_STRUCT_VAL(mat[4][4], {}); //!< Matrix data
} FOVE_STRUCT_END(Matrix44);

//! Struct holding information about projection frustum planes
/*! Values are given for a depth of 1 so that it's easy to multiply them by your near clipping plan, for example, to get the correct values for your use. */
FOVE_STRUCT(ProjectionParams)
{
	float FOVE_STRUCT_VAL(left, -1);   //!< Left side (low-X)
	float FOVE_STRUCT_VAL(right, 1);   //!< Right side (high-X)
	float FOVE_STRUCT_VAL(top, 1);     //!< Top (high-Y)
	float FOVE_STRUCT_VAL(bottom, -1); //!< Bottom (low-Y)
} FOVE_STRUCT_END(ProjectionParams);

//! A bounding box
FOVE_STRUCT(BoundingBox)
{
	Fove_Vec3 FOVE_STRUCT_VAL(center, {}); //!< The position of the center of the bounding box
	Fove_Vec3 FOVE_STRUCT_VAL(extend, {}); //!< The extend of the bounding box (e.g. half of its size)
} FOVE_STRUCT_END(BoundingBox);

//! Represents the pose of an object of the scene
/*!
	Pose transformations are applied in the following order on the object: scale, rotation, translation
*/
FOVE_STRUCT(ObjectPose)
{
	//! The scale of the object
	/*!
		Non-uniform scales are not supported for sphere collider shapes.
	 */
	Fove_Vec3 FOVE_STRUCT_VAL(scale, (Fove_Vec3{1, 1, 1}));                   //!<  The scale of the object in world space
	Fove_Quaternion FOVE_STRUCT_VAL(rotation, (Fove_Quaternion{0, 0, 0, 1})); //!<  The rotation of the object in world space
	Fove_Vec3 FOVE_STRUCT_VAL(position, {});                                  //!<  The position of the object in world space
	Fove_Vec3 FOVE_STRUCT_VAL(velocity, {});                                  //!<  Velocity of the object in world space
} FOVE_STRUCT_END(ObjectPose);

//! Specify the different collider shape types
FOVE_ENUM(ColliderType)
{
	FOVE_ENUM_VAL(ColliderType, Cube) = 0,   //!< A cube shape
	FOVE_ENUM_VAL(ColliderType, Sphere) = 1, //!< A sphere shape
	FOVE_ENUM_VAL(ColliderType, Mesh) = 2,   //!< A shape defined by a mesh
} FOVE_ENUM_END(ColliderType);

//! Define a cube collider shape
FOVE_STRUCT(ColliderCube)
{
	Fove_Vec3 FOVE_STRUCT_VAL(size, (Fove_Vec3{1.0f, 1.0f, 1.0f})); //!< The size of the cube
} FOVE_STRUCT_END(ColliderCube);

//! Define a sphere collider shape
FOVE_STRUCT(ColliderSphere)
{
	float FOVE_STRUCT_VAL(radius, 0.5f); //!< The radius of the sphere
} FOVE_STRUCT_END(ColliderSphere);

//! Define a mesh collider shape
/*!
	A mesh collider can either be defined through a triangle list or through a vertex/index buffer set.
	If the index buffer pointer is null, then the vertex buffer is interpreted as a regular triangle list.
*/
FOVE_STRUCT(ColliderMesh)
{
	//! The vertices of the mesh
	/*!
		It contains the X, Y, Z positions of mesh vertices.
		Triangles are defined using "indices".
	*/
	float* FOVE_STRUCT_VAL(vertices, nullptr);

	//! The number of elements in the vertex buffer
	/*!
		Note that one vertex has three component (X, Y, Z).
		So the size of vertices array is `3 x vertexCount`.
	*/
	unsigned int FOVE_STRUCT_VAL(vertexCount, 0);

	//! The vertex indices defining the triangles of the mesh
	/*!
		Triangles are listed one after the others (and not combined using a fan or strip algorithm).
		The number of elements must equal `3 x triangleCount`.

		Outward faces are defined to be specified counter-clockwise.
		Face-direction information is not currently used but may be in the future.

		If null, the vertices are interpreted as a simple triangle list.
	*/
	unsigned int* FOVE_STRUCT_VAL(indices, nullptr);

	unsigned int FOVE_STRUCT_VAL(triangleCount, 0); //!< The number of triangles present in the mesh

	Fove_BoundingBox FOVE_STRUCT_VAL(boundingBox, {}); //!< If null the bounding box is re-calculated internally
} FOVE_STRUCT_END(ColliderMesh);

//! Represents a colliding part of a gazable object
/*!
	Colliders are used to calculate intersection between gaze rays and gazable objects
*/
FOVE_STRUCT(ObjectCollider)
{
	Fove_Vec3 FOVE_STRUCT_VAL(center, {});            //!<  The offset of the collider center collider raw shape
	Fove_ColliderType FOVE_STRUCT_VAL(shapeType, {}); //!< The shape type of the collider
	union
	{
		Fove_ColliderCube cube;
		Fove_ColliderSphere sphere;
		Fove_ColliderMesh mesh;
	} FOVE_STRUCT_VAL(shapeDefinition, {}); //!< The definition of the collider shape. It should be interpreted accordingly to shapeType
} FOVE_STRUCT_END(ObjectCollider);

//! Represents an object in a 3D world
/*!
	The bounding shapes of this object are used for ray casts to determine what the user is looking at.
	Note that multiple bounding shape types can be used simultaneously, such as a sphere and a mesh.
	\see fove_Headset_registerGazableObject
	\see fove_Headset_removeGazableObject
	\see fove_Headset_updateGazableObject
*/
FOVE_STRUCT(GazableObject)
{
	int FOVE_STRUCT_VAL(id, fove_ObjectIdInvalid); //!< Unique ID of the object. User-defined objects should use positive integers

	Fove_ObjectPose FOVE_STRUCT_VAL(pose, {}); //!< The initial pose of the object

	Fove_ObjectGroup FOVE_STRUCT_VAL(group, Fove_ObjectGroup::Group0); //!< The gazable object group this object belongs to

	unsigned int FOVE_STRUCT_VAL(colliderCount, 0);           //!< The number of collider shapes defining this object
	Fove_ObjectCollider* FOVE_STRUCT_VAL(colliders, nullptr); //!< An array of colliders defining the geometry of the gazable object
} FOVE_STRUCT_END(GazableObject);

//! Represents a camera in a 3D world
/*!
	The camera view pose determine what the user is looking at and the object mask specify which objects are rendered.
	\see fove_Headset_registerCameraObject
	\see fove_Headset_removeCameraObject
	\see fove_Headset_updateCameraObject
*/
FOVE_STRUCT(CameraObject)
{
	int FOVE_STRUCT_VAL(id, fove_ObjectIdInvalid); //!< Unique ID of the camera. User-defined id should use positive integers

	Fove_ObjectPose FOVE_STRUCT_VAL(pose, {}); //!< The camera initial pose

	Fove_ObjectGroup FOVE_STRUCT_VAL(groupMask, static_cast<Fove_ObjectGroup>(0xffffffff)); //!< The bit mask specifying which object groups the camera renders
} FOVE_STRUCT_END(CameraObject);

//! Enum for type of Graphics API
/*! Type of Graphics API
	Note: We currently only support DirectX
*/
FOVE_ENUM(GraphicsAPI)
{
	FOVE_ENUM_VAL(GraphicsAPI, DirectX) = 0,   //!< DirectX (Windows only)
	FOVE_ENUM_VAL(GraphicsAPI, OpenGL) = 1,    //!< OpenGL (All platforms, currently in BETA)
	FOVE_ENUM_VAL(GraphicsAPI, Metal) = 2,     //!< Metal (Mac only)
	FOVE_ENUM_VAL(GraphicsAPI, Vulkan) = 3,    //!< Vulkan (Currently Linux only)
	FOVE_ENUM_VAL(GraphicsAPI, DirectX12) = 4, //!< DirectX12 (Windows only)
} FOVE_ENUM_END(GraphicsAPI);

//! Enum to help interpret the alpha of texture
/*! Determines how to interpret the alpha of a compositor client texture */
FOVE_ENUM(AlphaMode)
{
	FOVE_ENUM_VAL(AlphaMode, Auto) = 0,   //!< Base layers will use One, overlay layers will use Sample
	FOVE_ENUM_VAL(AlphaMode, One) = 1,    //!< Alpha will always be one (fully opaque)
	FOVE_ENUM_VAL(AlphaMode, Sample) = 2, //!< Alpha fill be sampled from the alpha channel of the buffer
} FOVE_ENUM_END(AlphaMode);

//! Struct used to define the settings for a compositor client
FOVE_STRUCT(CompositorLayerCreateInfo)
{
	//! The layer type upon which the client will draw
	Fove_CompositorLayerType FOVE_STRUCT_VAL(type, Fove_CompositorLayerType::Base);
	//! Setting to disable timewarp, e.g. if an overlay client is operating in screen space
	bool FOVE_STRUCT_VAL(disableTimeWarp, false);
	//! Setting about whether to use alpha sampling or not, e.g. for a base client
	Fove_AlphaMode FOVE_STRUCT_VAL(alphaMode, Fove_AlphaMode::Auto);
	//! Setting to disable fading when the base layer is misbehaving, e.g. for a diagnostic client
	bool FOVE_STRUCT_VAL(disableFading, false);
	//! Setting to disable a distortion pass, e.g. for a diagnostic client, or a client intending to do its own distortion
	bool FOVE_STRUCT_VAL(disableDistortion, false);
} FOVE_STRUCT_END(CompositorLayerCreateInfo);

//! Struct used to store information about an existing compositor layer (after it is created)
/*! This exists primarily for future expandability. */
FOVE_STRUCT(CompositorLayer)
{
	//! Uniquely identifies a compositor layer
	int FOVE_STRUCT_VAL(layerId, 0);

	/*! The optimal resolution for a submitted buffer on this layer (for a single eye).
	    Clients are allowed to submit buffers of other resolutions.
	    In particular, clients can use a lower resolution buffer to reduce their rendering overhead.
	*/
	Fove_Vec2i FOVE_STRUCT_VAL(idealResolutionPerEye, {});
} FOVE_STRUCT_END(CompositorLayer);

//! Base class of API-specific texture classes
FOVE_STRUCT(CompositorTexture)
{
	//! Rendering API of this texture
	/*!
		If this is DirectX, this object must be a Fove_DX11Texture
		If this is OpenGL, this object must be a Fove_GLTexture
		If this is Metal, this object must be a Fove_MetalTexture
		If this is Vulkan, this object must be a Fove_VulkanTexture
		If this is DirectX12, this object must be a Fove_DX12Texture
		In C++ this field is initialized automatically by the subclass
	*/
	Fove_GraphicsAPI graphicsAPI;

#if FOVE_DEFINE_CXX_API
protected:
	// Create and destroy objects via one of the derived classes, based on which graphics API you are submitting with
	Fove_CompositorTexture(const Fove_GraphicsAPI api) : graphicsAPI{api} {}
	~Fove_CompositorTexture() = default;
#endif // FOVE_DEFINE_CXX_API
} FOVE_STRUCT_END(CompositorTexture);

//! Struct used to submit a DirectX 11 texture
FOVE_STRUCT(DX11Texture)
#if FOVE_DEFINE_CXX_API
	: public Fove_CompositorTexture
#endif
{
#if !FOVE_DEFINE_CXX_API
	//! Parent object
	Fove_CompositorTexture parent;
#endif

	//! Pointer to the texture ID3D11Texture2D interface
	void* FOVE_STRUCT_VAL(texture, nullptr);

	//! Pointer to the texture ID3D11ShaderResourceView interface
	void* FOVE_STRUCT_VAL(resourceView, nullptr);

#if FOVE_DEFINE_CXX_API
	Fove_DX11Texture(void* const t = nullptr, void* const rv = nullptr) : Fove_CompositorTexture{Fove_GraphicsAPI::DirectX}, texture{t}, resourceView{rv} {}
#endif // FOVE_DEFINE_CXX_API
} FOVE_STRUCT_END(DX11Texture);

//! Struct used to submit a DirectX 12 texture
FOVE_STRUCT(DX12Texture)
#if FOVE_DEFINE_CXX_API
	: public Fove_CompositorTexture
#endif
{
#if !FOVE_DEFINE_CXX_API
	//! Parent object
	Fove_CompositorTexture parent;
#endif

	//! Pointer to the texture ID3D12Resource interface
	void* FOVE_STRUCT_VAL(texture, nullptr);

#if FOVE_DEFINE_CXX_API
	Fove_DX12Texture(void* const t = nullptr) : Fove_CompositorTexture{Fove_GraphicsAPI::DirectX12}, texture{t} {}
#endif // FOVE_DEFINE_CXX_API
} FOVE_STRUCT_END(DX12Texture);

//! Struct used to submit an OpenGL texture
/*! The GL context must be active on the thread that submits this. */
FOVE_STRUCT(GLTexture)
#if FOVE_DEFINE_CXX_API
	: public Fove_CompositorTexture
#endif
{
#if !FOVE_DEFINE_CXX_API
	//! Parent object
	Fove_CompositorTexture parent;
#endif

	//! The OpenGl ID of the texture, as returned by glGenTextures
	uint32_t FOVE_STRUCT_VAL(textureId, 0);
	//! On mac, this is a CGLContextObj, otherwise this field is reserved and you must pass null
	void* FOVE_STRUCT_VAL(context, nullptr);

#if FOVE_DEFINE_CXX_API
	Fove_GLTexture(const uint32_t tid = 0, void* const c = nullptr) : Fove_CompositorTexture{Fove_GraphicsAPI::OpenGL}, textureId{tid}, context{c} {}
#endif // FOVE_DEFINE_CXX_API
} FOVE_STRUCT_END(GLTexture);

//! Struct used to submit a texture using the Apple Metal API
FOVE_STRUCT(MetalTexture)
#if FOVE_DEFINE_CXX_API
	: public Fove_CompositorTexture
#endif
{
#if !FOVE_DEFINE_CXX_API
	//! Parent object
	Fove_CompositorTexture parent;
#endif

	//! Pointer to an MTLTexture (which must have MTLTextureUsageShaderRead specified)
	void* FOVE_STRUCT_VAL(texture, nullptr);

#if FOVE_DEFINE_CXX_API
	Fove_MetalTexture(void* const t = nullptr) : Fove_CompositorTexture{Fove_GraphicsAPI::Metal}, texture{t} {}
#endif // FOVE_DEFINE_CXX_API
} FOVE_STRUCT_END(MetalTexture);

// Those opaque handles without `Fove_` prefix are defined in `vulkan/vulkan_core.h`
// and they are synonyms
typedef struct VkInstance_T* Fove_VkInstance;             //!< Opaque type representing a vulkan instance
typedef struct VkPhysicalDevice_T* Fove_VkPhysicalDevice; //!< Opaque type representing a vulkan physical device
typedef struct VkDevice_T* Fove_VkDevice;                 //!< Opaque type representing a vulkan logical device (It must have VK_KHR_external_memory and VK_KHR_external_memory_fd enabled)
typedef struct VkQueue_T* Fove_VkQueue;                   //!< Opaque type representing a vulkan queue

typedef struct VkDeviceMemory_T* Fove_VkDeviceMemory; //!< Opaque type representing a vulkan device memory region
typedef struct VkImage_T* Fove_VkImage;               //!< Opaque type representing a vulkan image
typedef struct VkImageView_T* Fove_VkImageView;       //!< Opaque type representing a vulkan image view

//! Struct to represent a suite of vulkan components to be used for texture submission
FOVE_STRUCT(VulkanContext)
{
	Fove_VkInstance FOVE_STRUCT_VAL(instance, nullptr);             //!< Vulkan handle to the vulkan instance
	Fove_VkPhysicalDevice FOVE_STRUCT_VAL(physicalDevice, nullptr); //!< Vulkan handle to the physical device
	Fove_VkDevice FOVE_STRUCT_VAL(device, nullptr);                 //!< Vulkan handle to the logical device
	Fove_VkQueue FOVE_STRUCT_VAL(graphicsQueue, nullptr);           //!< Vulkan handle to the graphics queue
	Fove_VkQueue FOVE_STRUCT_VAL(presentationQueue, nullptr);       //!< Vulkan handle to the presentation queue
	Fove_VkQueue FOVE_STRUCT_VAL(transferQueue, nullptr);           //!< Vulkan handle to the transfer queue
	uint32_t FOVE_STRUCT_VAL(graphicsQueueFamilyIndex, ~0u);        //!< Vulkan queue family index for graphics queues
	uint32_t FOVE_STRUCT_VAL(presentationQueueFamilyIndex, ~0u);    //!< Vulkan queue family index for presentation queues
	uint32_t FOVE_STRUCT_VAL(transferQueueFamilyIndex, ~0u);        //!< Vulkan queue family index for transfer queues
} FOVE_STRUCT_END(VulkanContext);

//! Struct to hold handles to resources for a texture
FOVE_STRUCT(VulkanTextureResources)
{
	Fove_VkDeviceMemory FOVE_STRUCT_VAL(deviceMemory, nullptr); //!< Vulkan handle to the device memory region for the texture
	Fove_VkImage FOVE_STRUCT_VAL(image, nullptr);               //!< Vulkan handle to the image object for the texture
	Fove_VkImageView FOVE_STRUCT_VAL(imageView, nullptr);       //!< Vulkan handle to the image view object for the texture
} FOVE_STRUCT_END(VulkanTextureResources);

//! Struct used to submit a texture using the Vulkan API
//! This is for submitting render textures that are to be composed by the Fove compositor,
//! so only single layer 2D images with 32bit RGBA pixels are supported
FOVE_STRUCT(VulkanTexture)
#if FOVE_DEFINE_CXX_API
	: public Fove_CompositorTexture
#endif
{
#if !FOVE_DEFINE_CXX_API
	//! Parent object
	Fove_CompositorTexture parent;
#endif
	Fove_VulkanContext FOVE_STRUCT_VAL(context, {});            //!< Vulkan context used to create this texture
	Fove_VulkanTextureResources FOVE_STRUCT_VAL(resources, {}); //!< Vulkan resources for this texture
	uint32_t FOVE_STRUCT_VAL(width, 0);                         //!< Width of texture data
	uint32_t FOVE_STRUCT_VAL(height, 0);                        //!< Height of texture data

#if FOVE_DEFINE_CXX_API
	Fove_VulkanTexture()
		: Fove_CompositorTexture{Fove_GraphicsAPI::Vulkan}
	{
	}
	Fove_VulkanTexture(const Fove_VulkanContext context, const Fove_VulkanTextureResources resources, const uint32_t w, const uint32_t h)
		: Fove_CompositorTexture{Fove_GraphicsAPI::Vulkan}
		, context{context}
		, resources{resources}
		, width{w}
		, height{h}
	{
	}
#endif // FOVE_DEFINE_CXX_API
} FOVE_STRUCT_END(VulkanTexture);

//! Specify a region of a texture in normalized space
/*! Coordinates in normalized space where 0 is left/top and 1 is bottom/right */
FOVE_STRUCT(TextureBounds)
{
	float FOVE_STRUCT_VAL(left, 0.0f);   //!< The horizontal coordinate of the left border
	float FOVE_STRUCT_VAL(top, 0.0f);    //!< The vertical coordinate of the top border
	float FOVE_STRUCT_VAL(right, 0.0f);  //!< The horizontal coordinate of the right border
	float FOVE_STRUCT_VAL(bottom, 0.0f); //!< The vertical coordinate of the bottom border
} FOVE_STRUCT_END(TextureBounds);

//! Struct used to conglomerate the texture settings for a single eye, when submitting a given layer
FOVE_STRUCT(CompositorLayerEyeSubmitInfo)
{
	//! Texture to submit for this eye
	/*! This may be null as long as the other submitted eye's texture isn't (thus allowing each eye to be submitted separately) */
	const Fove_CompositorTexture* FOVE_STRUCT_VAL(texInfo, nullptr);

	//! The portion of the texture that is used to represent the eye (E.g. half of it if the texture contains both eyes)
	Fove_TextureBounds FOVE_STRUCT_VAL(bounds, {});
} FOVE_STRUCT_END(CompositorLayerEyeSubmitInfo);

//! Struct used to conglomerate the texture settings when submitting a given layer
FOVE_STRUCT(CompositorLayerSubmitInfo)
{
	int FOVE_STRUCT_VAL(layerId, 0);                              //!< The layer ID as fetched from Fove_CompositorLayer
	Fove_Pose FOVE_STRUCT_VAL(pose, {});                          //!< The pose used to draw this layer, usually coming from fove_Compositor_waitForRenderPose
	Fove_CompositorLayerEyeSubmitInfo FOVE_STRUCT_VAL(left, {});  //!< Information about the left eye
	Fove_CompositorLayerEyeSubmitInfo FOVE_STRUCT_VAL(right, {}); //!< Information about the right eye
} FOVE_STRUCT_END(CompositorLayerSubmitInfo);

//! Struct used to identify a GPU adapter (Windows only)
FOVE_STRUCT(AdapterId)
{
#ifdef _WIN32
	// On windows, this forms a LUID structure
	uint32_t FOVE_STRUCT_VAL(lowPart, 0); //!< The lower part of the ID
	int32_t FOVE_STRUCT_VAL(highPart, 0); //!< The higher part of the ID
#endif
} FOVE_STRUCT_END(AdapterId);

//! A generic memory buffer
/*! No ownership or lifetime semantics are specified. Please see the comments on the functions that use this. */
FOVE_STRUCT(Buffer)
{
	//! Pointer to the start of the memory buffer
	const void* FOVE_STRUCT_VAL(data, nullptr);
	//! Length, in bytes, of the buffer
	size_t FOVE_STRUCT_VAL(length, 0);
} FOVE_STRUCT_END(Buffer);

//! Specify the shape of an eye
FOVE_STRUCT(EyeShape)
{
	//! The position the eye outline in the current eye images in pixels
	/*! The eye outline is composed of 12 points as follow:
	    - Point 0: the inside extremity of the outline (the point the closest to the nose)
	    - Point 1 to 5: bottom eyelid points going from the inside to the outside of the eye
	    - Point 6: the outside extremity of the outline (the point the furthest from the nose)
	    - Point 7 to 11: upper eyelid points going from the outside to the inside of the eye
	*/
	FOVE_STRUCT_ARRAY(Fove_Vec2, 12, outline, {});
} FOVE_STRUCT_END(EyeShape);

//! Specify the shape of a pupil as an ellipse
/*! Coordinates are in eye-image pixels from (0,0) to (camerawidth, cameraheight), with (0,0) being the top left. */
FOVE_STRUCT(PupilShape)
{
	//! The center of the ellipse
	Fove_Vec2 FOVE_STRUCT_VAL(center, {});
	//! The width and height of the ellipse
	Fove_Vec2 FOVE_STRUCT_VAL(size, {});
	//! A clockwise rotation around the center, in degrees
	float FOVE_STRUCT_VAL(angle, 0.0f);
} FOVE_STRUCT_END(PupilShape);

//! A 2D bitmap image
FOVE_STRUCT(BitmapImage)
{
	//! Timestamp of the image, in microseconds since an unspecified epoch
	uint64_t FOVE_STRUCT_VAL(timestamp, 0);
	//! BMP data (including full header that contains size, format, etc)
	/*! The height may be negative to specify a top-down bitmap. */
	Fove_Buffer FOVE_STRUCT_VAL(image, {});
} FOVE_STRUCT_END(BitmapImage);

//! Represent a calibration target of the calibration process
FOVE_STRUCT(CalibrationTarget)
{
	//! The position of the calibration target in the 3D world space
	Fove_Vec3 FOVE_STRUCT_VAL(position, {});
	//! The recommended size for the calibration target in world space unit
	/*! A recommended size of 0 means that the display of the target is not recommended at the current time */
	float FOVE_STRUCT_VAL(recommendedSize, 0.0f);
} FOVE_STRUCT_END(CalibrationTarget);

//! Indicate the state of a calibration process
/*!
	A calibration process always starts from the `NotStarted` state,
	then it goes to the `HeadsetAdjustment` step if improving the position of the HMD is necessary,
	then it can go back and forth between the `WaitingForUser` & `CollectingData` states,
	then it goes to the `ProcessingData` state and finishes with the `Successful` state.

	A failure can happen any time during the process, and stops the process where it was.

	From the `ProcessingData` state the calibration process does not require any rendering
	and gameplay can be started if wanted but new calibration won't be effective before reaching the `Successful` state.
*/
FOVE_ENUM(CalibrationState)
{
	FOVE_ENUM_VAL(CalibrationState, NotStarted),               //!< No calibration process has been started yet
	FOVE_ENUM_VAL(CalibrationState, HeadsetAdjustment),        //!< The calibration process is waiting for the user to adjust the headset
	FOVE_ENUM_VAL(CalibrationState, WaitingForUser),           //!< The calibration process is waiting for the user to get ready
	FOVE_ENUM_VAL(CalibrationState, CollectingData),           //!< The calibration process is currently collecting calibration data
	FOVE_ENUM_VAL(CalibrationState, ProcessingData),           //!< The calibration process is currently processing the collected data
	FOVE_ENUM_VAL(CalibrationState, Successful_HighQuality),   //!< The calibration is successful and of high quality
	FOVE_ENUM_VAL(CalibrationState, Successful_MediumQuality), //!< The calibration is successful and of medium quality
	FOVE_ENUM_VAL(CalibrationState, Successful_LowQuality),    //!< The calibration is successful but of low quality
	FOVE_ENUM_VAL(CalibrationState, Failed_Unknown),           //!< The calibration process failed due to an unknown issue
	FOVE_ENUM_VAL(CalibrationState, Failed_InaccurateData),    //!< The calibration process failed because of inaccurate or flawed data
	FOVE_ENUM_VAL(CalibrationState, Failed_NoRenderer),        //!< The calibration process failed after timeout because it couldn't find a renderer
	FOVE_ENUM_VAL(CalibrationState, Failed_NoUser),            //!< The calibration process failed after timeout because it couldn't detect the user
	FOVE_ENUM_VAL(CalibrationState, Failed_Aborted)            //!< The calibration process was manually aborted by the user
} FOVE_ENUM_END(CalibrationState);

//! Indicate the calibration method to use
FOVE_ENUM(CalibrationMethod)
{
	FOVE_ENUM_VAL(CalibrationMethod, Default),                                //!< Use the calibration method specified in the configuration file (default: single point)
	FOVE_ENUM_VAL(CalibrationMethod, OnePoint),                               //!< Use the simple point calibration method (Requires license)
	FOVE_ENUM_VAL(CalibrationMethod, Spiral),                                 //!< Use the spiral calibration method
	FOVE_ENUM_VAL(CalibrationMethod, OnePointWithNoGlassesSpiralWithGlasses), //!< Use the 1-point calibration method for user without eyeglasses, and the spiral calibration method if user has eyeglasses
	FOVE_ENUM_VAL(CalibrationMethod, ZeroPoint),                              //!< Use the zero point calibration method (Requires license)
	FOVE_ENUM_VAL(CalibrationMethod, DefaultCalibration)                      //!< Use a premade calibration profile with average human parameters built in
} FOVE_ENUM_END(CalibrationMethod);

//! Indicate whether each eye should be calibrated separately or not
FOVE_ENUM(EyeByEyeCalibration)
{
	FOVE_ENUM_VAL(EyeByEyeCalibration, Default),  //!< Use the settings coming from the configuration file (default: Disabled)
	FOVE_ENUM_VAL(EyeByEyeCalibration, Disabled), //!< Calibrate both eyes simultaneously
	FOVE_ENUM_VAL(EyeByEyeCalibration, Enabled)   //!< Calibrate each eye separately, first the left, then the right (Requires license)
} FOVE_ENUM_END(EyeByEyeCalibration);

//! Indicate whether eye torsion calibration should be run or not
FOVE_ENUM(EyeTorsionCalibration)
{
	FOVE_ENUM_VAL(EyeTorsionCalibration, Default),   //!< Use the settings coming from the configuration file (default: IfEnabled)
	FOVE_ENUM_VAL(EyeTorsionCalibration, IfEnabled), //!< Run eye torsion calibration only if the capability is currently enabled
	FOVE_ENUM_VAL(EyeTorsionCalibration, Always),    //!< Always run eye torsion calibration independently of whether the capability is used
} FOVE_ENUM_END(EyeTorsionCalibration);

//! Provide all the calibration data needed to render the current state of the calibration process
FOVE_STRUCT(CalibrationData)
{
	Fove_CalibrationMethod FOVE_STRUCT_VAL(method, Fove_CalibrationMethod::Spiral);  //!< The calibration method currently used, or Default if the method is unknown (from a future update)
	Fove_CalibrationState FOVE_STRUCT_VAL(state, Fove_CalibrationState::NotStarted); //!< The current state of the calibration
	const char* FOVE_STRUCT_VAL(stateInfo, nullptr);                                 //!< Human readable extra information about the current calibration state
	Fove_CalibrationTarget FOVE_STRUCT_VAL(targetL, {});                             //!< The current calibration target to display for the left eye
	Fove_CalibrationTarget FOVE_STRUCT_VAL(targetR, {});                             //!< The current calibration target to display for the right eye
} FOVE_STRUCT_END(CalibrationData);

//! Provide all the HMD positioning data needed to render the current state of the HMD adjustment process
FOVE_STRUCT(HmdAdjustmentData)
{
	Fove_Vec2 FOVE_STRUCT_VAL(translation, {});        //!< The HMD translation offset in eyes camera in relative units ([-1, 1])
	float FOVE_STRUCT_VAL(rotation, 0);                //!< The rotation of HMD to the eye line in radian
	bool FOVE_STRUCT_VAL(adjustmentNeeded, false);     //!< Indicate whether the HMD adjustment GUI should be displayed to correct user HMD alignment
	bool FOVE_STRUCT_VAL(hasTimeout, false);           //!< Indicate if the adjustment process has timed out, in which case the GUI should close
	Fove_Vec2 FOVE_STRUCT_VAL(idealPositionL, {});     //!< Pixel coordinate on the left camera image for the expected ideal eye position
	Fove_Vec2 FOVE_STRUCT_VAL(idealPositionR, {});     //!< Pixel coordinate on the right camera image for the expected ideal eye position
	float FOVE_STRUCT_VAL(idealPositionSpanL, 0);      //!< Radius of the tolerance area for the expected ideal eye position on the left camera image in pixels
	float FOVE_STRUCT_VAL(idealPositionSpanR, 0);      //!< Radius of the tolerance area for the expected ideal eye position on the right camera image in pixels
	Fove_Vec2 FOVE_STRUCT_VAL(estimatedPositionL, {}); //!< Pixel coordinate of left eye position which is independent on eye orientation
	Fove_Vec2 FOVE_STRUCT_VAL(estimatedPositionR, {}); //!< Pixel coordinate of right eye position which is independent on eye orientation
} FOVE_STRUCT_END(HmdAdjustmentData);

//! Parameters specifying how to run a calibration process
FOVE_STRUCT(CalibrationOptions)
{
	bool FOVE_STRUCT_VAL(lazy, false);                                                           //!< Do not restart the calibration process if it is already calibrated
	bool FOVE_STRUCT_VAL(restart, false);                                                        //!< Restart the calibration process from the beginning if it is already running
	Fove_EyeByEyeCalibration FOVE_STRUCT_VAL(eyeByEye, Fove_EyeByEyeCalibration::Default);       //!< Calibrate both eyes simultaneously or separately
	Fove_CalibrationMethod FOVE_STRUCT_VAL(method, Fove_CalibrationMethod::Default);             //!< The calibration method to use
	Fove_EyeTorsionCalibration FOVE_STRUCT_VAL(eyeTorsion, Fove_EyeTorsionCalibration::Default); //!< Whether to perform eye torsion calibration or not
} FOVE_STRUCT_END(CalibrationOptions);

/////////////////////////////////////////////////////////////////////////////////
// Fove C API -------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////

// This API requires C99 or later

// All functions in the C API return Fove_ErrorCode
// Other return parameters are written via out pointers

typedef struct Fove_Headset_* Fove_Headset;       //!< Opaque type representing a headset object
typedef struct Fove_Compositor_* Fove_Compositor; //!< Opaque type representing a compositor connection

//! Writes some text to the FOVE log
/*!
	\param level What severity level the log will use
	\param utf8Text Null-terminated text string in UTF-8
	\return #Fove_ErrorCode_None if the call succeeded
	        #Fove_ErrorCode_API_InvalidArgument if the utf8Text is nullptr
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_InvalidEnumValue if level is not one of the valid values

*/
FOVE_EXPORT Fove_ErrorCode fove_logText(Fove_LogLevel level, const char* utf8Text) FOVE_NOEXCEPT;

//! Creates and returns an Fove_Headset object, which is the entry point to the entire API
/*!
	The result headset should be destroyed using fove_Headset_destroy when no longer needed.
	\param capabilities The desired capabilities (Gaze, Orientation, Position), for multiple capabilities, use bitwise-or input such as Fove_ClientCapabilities::Gaze | Fove_ClientCapabilities::Position
	\param outHeadset A pointer where the address of the newly created headset will be written upon success
	\return #Fove_ErrorCode_None if the headset was created successfully\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see fove_Headset_destroy
*/
FOVE_EXPORT Fove_ErrorCode fove_createHeadset(Fove_ClientCapabilities capabilities, Fove_Headset** outHeadset) FOVE_NOEXCEPT;

//! Frees resources used by a headset object, including memory and sockets
/*!
	Upon return, this headset pointer should no longer be used.
	\return #Fove_ErrorCode_None if the headset was destroyed successfully\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see fove_createHeadset
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_destroy(Fove_Headset*) FOVE_NOEXCEPT;

//! Writes out whether an HMD is connected or not
/*!
	\param outHardwareConnected A pointer to the value to be written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_NullInPointer if param pointer is null\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isHardwareConnected(Fove_Headset*, bool* outHardwareConnected) FOVE_NOEXCEPT;

//! Writes out whether motion tracking hardware has started
/*!
	\param outMotionReady   A pointer to the variable to be written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_NullInPointer if param pointer is null\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isMotionReady(Fove_Headset*, bool* outMotionReady) FOVE_NOEXCEPT;

//! Checks whether the client can run against the installed version of the FOVE SDK
/*!
	This makes a blocking call to the runtime.

	\return #Fove_ErrorCode_None if this client is compatible with the currently running service\n
	        #Fove_ErrorCode_Connect_RuntimeVersionTooOld if not compatible with the currently running service\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_checkSoftwareVersions(Fove_Headset*) FOVE_NOEXCEPT;

//! Writes out the current software versions
/*!
	Allows you to get detailed information about the client and runtime versions.

	Instead of comparing software versions directly, you should simply call
	`CheckSoftwareVersions` to ensure that the client and runtime are compatible.

	This makes a blocking call to the runtime.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_API_NullInPointer if the param pointer is null\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_querySoftwareVersions(Fove_Headset*, Fove_Versions* outSoftwareVersions) FOVE_NOEXCEPT;

//! Returns information about any licenses currently activated
/*!
	There is the possibility of having more than one license, or none at all, so an array is provided.

	This will only return valid, activated, licenses.
	As soon as a license expires or is otherwise deactivated, it will no longer be returned from this.

	Usually you do not need to call this function directly.
	To check if a feature is available, simply use the feature, and see if you get a `License_FeatureAccessDenied` error.

	\param  outLicenseInfos An array of LicenseInfo objects to write info to. If null, nothing is written and only inOutArraySize is updated
	\param  inOutArraySize On input, this is the size of the array of pointers, and on output is the number of elements written to the array
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid
	        #Fove_ErrorCode_API_NullInPointer if the param pointer is null
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_queryLicenses(Fove_Headset*, Fove_LicenseInfo* outLicenseInfos, size_t* inOutArraySize) FOVE_NOEXCEPT;

//! Writes out the hardware information
/*!
	Allows you to get serial number, manufacturer, and model name of the headset.
	\param outHardwareInfo A pointer to the hardware info struct to write info to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_NullInPointer if the param pointer is null\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_queryHardwareInfo(Fove_Headset*, Fove_HeadsetHardwareInfo* outHardwareInfo) FOVE_NOEXCEPT;

//! Registers capabilities needed by this client
/*!
	Usually you provide the required capabilities at the creation of the headset in `fove_createHeadset`.
	But you can add and remove capabilities anytime while the object is alive.

	This is useful if you only have certain sections of your app that require extra capabilities.
	Fo example, if your app doesn't use eye tracking all the time, you can disable it temporarily to reduce CPU usage & power consumption.

	Because this may enable software or hardware components, turn on cameras, etc,
	it may take up to a few seconds before the associated data is available.
	In the meantime, typically `Fove_ErrorCode_Data_NoUpdate` is returned from related data getters.

	The service enables/disables software and hardware based on the needs of all connected clients,
	so it's possible that some other client already has the given capability registered,
	and thus the associated data may become available very quickly in these cases.

	This function makes a request to the service and waits for response (with a small timeout).
	This means it will block the thread temporarily. However, the capability set is recorded locally,
	so even if the connection to the service fails, no error will be returned. Later, upon the next
	connection to the server, the capability set will be sent automatically.

	It is completely safe to register capabilities while the service is down. No error will be returned.

	\param caps A set of capabilities to register. Registering an existing capability is a no-op
	\return #Fove_ErrorCode_None if the capability has been properly registered locally\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_API_NullInPointer if the param pointer is null\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see    fove_createHeadset
	\see    fove_Headset_unregisterCapabilities
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_registerCapabilities(Fove_Headset*, Fove_ClientCapabilities caps) FOVE_NOEXCEPT;

//! Registers passive capabilities for this client
/*!
	The difference between active capabilties (those registered with `fove_Headset_registerCapabilities`) is that
	passive capabilities are not used to enable hardware or software components. There must be at least one active
	capability registered for the required hardware/software modules to be enabled.

	However, if another app registers the same capability actively, you can use passive capabilities to read the data
	being exported from the service on behalf of another client who has registered the capability actively.

	Basically, this means "if it's on I want it, but I don't want to turn it on myself".

	Within a single client, there's no point to registering a capability passively if it's already registered actively.
	However, this is not an error, and the capability will be registered passively. The two lists are kept totally separate.

	\param caps A set of capabilities to register. Registering an existing capability is a no-op
	\return #Fove_ErrorCode_None if the capability has been properly registered locally\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_API_NullInPointer if the param pointer is null\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see    fove_createHeadset
	\see    fove_Headset_unregisterCapabilities
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_registerPassiveCapabilities(Fove_Headset*, Fove_ClientCapabilities caps) FOVE_NOEXCEPT;

//! Unregisters capabilities previously registered by this client
/*!
	Removes capabilities previously added by `fove_createHeadset` or `fove_Headset_registerCapabilities`.

	See `fove_Headset_registerCapabilities` for details on registration / unregistration.

	\param caps A set of capabilities to unregister. Unregistering an not-existing capability is a no-op
	\return #Fove_ErrorCode_None if the capability has been properly unregistered\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see    fove_createHeadset
	\see    fove_Headset_registerCapabilities
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_unregisterCapabilities(Fove_Headset*, Fove_ClientCapabilities caps) FOVE_NOEXCEPT;

//! Unregisters passive capabilities previously registered by this client
/*!
	Removes passive capabilities previously added by `fove_registerPassiveCapabilities`.

	It has no effect on active capabilities registered with `fove_registerCapabilities` or `fove_createHeadset`.

	\param caps A set of capabilities to unregister. Unregistering an not-existing capability is a no-op
	\return #Fove_ErrorCode_None if the capability has been properly unregistered\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see    fove_createHeadset
	\see    fove_Headset_registerCapabilities
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_unregisterPassiveCapabilities(Fove_Headset*, Fove_ClientCapabilities caps) FOVE_NOEXCEPT;

//! Waits for next eye camera frame to be processed
/*!
	Allows you to sync your eye tracking loop to the actual eye-camera loop.
	On each loop, you would first call this blocking function to wait for the next eye frame to be processed,
	then update the local cache of eye tracking data using the fetch functions,
	and finally get the desired eye tracking data using the getters.

	Eye tracking should be enabled by registering the `Fove_ClientCapabilities_EyeTracking` before calling this function.

	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call
	        #Fove_ErrorCode_API_Timeout if the call timed out
	\see    fove_Headset_fetchEyeTrackingData
	\see    fove_Headset_fetchEyesImage
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_waitForProcessedEyeFrame(Fove_Headset*) FOVE_NOEXCEPT;

//! Fetch the latest eye tracking data from the runtime service
/*!
	This function updates a local cache of eye tracking data, which other getters will fetch from.

	A cache is used as a means to ensure that multiple getters can be called without a frame update in between.
	Everything in the cache is from the same frame, thus you can make sequential queries for data,
	and you will get data from the same frame as long as you do not refetch in between.

	This function never blocks the thread. If no new data is available, no operation is performed.
	The timestamp can be used to know if the data has been updated or not.

	Usually, you want to call this function at the beginning of your update loop if your thread is synchronized
	with the HMD display. On the other hand, if your thread is synchronized with the eye tracker thread,
	you usually want to call it just after `fove_Headset_waitForProcessedEyeFrame`.

	Eye tracking should be enabled by registering the `Fove_ClientCapabilities_EyeTracking` before calling this function.

	\param outTimestamp A pointer to write the frame timestamp of fetched data. If null, the timestamp is not written.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call
	\see    fove_Headset_getCombinedGazeDepth
	\see    fove_Headset_getCombinedGazeRay
	\see    fove_Headset_getEyeballRadius
	\see    fove_Headset_getEyeBlinkCount
	\see    fove_Headset_getEyeShape
	\see    fove_Headset_getEyeState
	\see    fove_Headset_getEyeTorsion
	\see    fove_Headset_getEyeTrackingCalibrationState
	\see    fove_Headset_getEyeTrackingCalibrationStateDetails
	\see    fove_Headset_getGazeScreenPosition
	\see    fove_Headset_getGazeScreenPositionCombined
	\see    fove_Headset_getGazeVector
	\see    fove_Headset_getGazeVectorRaw
	\see    fove_Headset_getIrisRadius
	\see    fove_Headset_getPupilRadius
	\see    fove_Headset_getPupilShape
	\see    fove_Headset_getUserIOD
	\see    fove_Headset_getUserIPD
	\see    fove_Headset_hasHmdAdjustmentGuiTimeout
	\see    fove_Headset_isEyeBlinking
	\see    fove_Headset_isEyeTrackingCalibrated
	\see    fove_Headset_isEyeTrackingCalibratedForGlasses
	\see    fove_Headset_isEyeTrackingCalibrating
	\see    fove_Headset_isHmdAdjustmentGuiVisible
	\see    fove_Headset_isUserPresent
	\see    fove_Headset_isUserShiftingAttention
	\see    fove_Headset_waitForProcessedEyeFrame
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_fetchEyeTrackingData(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Fetch the latest eyes camera image from the runtime service
/*!
	This function updates a local cache of eyes image, that can be retrieved through `fove_Headset_getEyesImage`.

	A cache is used to ensure that multiple calls to `fove_Headset_getEyesImage` return exactly the same data
	until we request an explicit data update through the next fetch call.

	This function never blocks the thread. If no new data is available, no operation is performed.
	The timestamp can be used to know if the data has been updated or not.

	Usually, you want to call this function in conjunction with `fove_Headset_fetchEyeTrackingData` either at the beginning
	of your update loop of just after `fove_Headset_waitForProcessedEyeFrame` depending on your thread synchronization.

	Eyes image capability should be enabled by registering `Fove_ClientCapabilities_EyesImage` before calling this function.

	\param outTimestamp A pointer to write the frame timestamp of fetched data. If null, the timestamp is not written.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call
	        #Fove_ErrorCode_Data_Unreadable if the data is unreadable
	\see    fove_Headset_getEyesImage
	\see    fove_Headset_fetchEyeTrackingData
	\see    fove_Headset_waitForProcessedEyeFrame
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_fetchEyesImage(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Writes out the eye frame timestamp of the cached eye tracking data
/*!
	Basically returns the timestamp returned by the last call to `fove_Headset_fetchEyeTrackingData`.

	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param outTimestamp A pointer to write the frame timestamp of the currently cached data.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_API_NullInPointer if outTimestamp is null
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeTrackingDataTimestamp(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Writes out the eye frame timestamp of the cached eyes image
/*!
	Basically returns the timestamp returned by the last call to `fove_Headset_fetchEyesImage`.

	`Fove_ClientCapabilities_EyesImage` should be registered to use this function.

	\param outTimestamp A pointer to write the frame timestamp of the currently cached data.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_API_NullInPointer if the param pointer is null\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	\see    fove_Headset_fetchEyesImage
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyesImageTimestamp(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Writes out the gaze vector of an individual eye
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outVector  A pointer to the eye gaze vector to write to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if outVector is null
	\see    fove_Headset_fetchEyeTrackingData
	\see    fove_Headset_getGazeVectorRaw
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getGazeVector(Fove_Headset*, Fove_Eye eye, Fove_Vec3* outVector) FOVE_NOEXCEPT;

//! Writes out the raw gaze vector of an individual eye
/*!
	Returns the rawest gaze vector that the eye tracker provides.

	An eye tracker is, at it's core, a complex data processing algorithm, so there is no truly "raw" vector.
	However, any final smoothing is removed to give the most raw data possible.

	`fove_Headset_getGazeVector` is recommended for UI control, drawing gaze cursors on screen, and the like.
	`fove_Headset_getGazeVectorRaw` is recommended for raw data collection for later data analysis.

	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outVector  A pointer to the raw eye gaze vector to write to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if outVector is null
	\see    fove_Headset_fetchEyeTrackingData
	\see    fove_Headset_getGazeVector
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getGazeVectorRaw(Fove_Headset*, Fove_Eye eye, Fove_Vec3* outVector) FOVE_NOEXCEPT;

//! Writes out the user's 2D gaze position on the screens seen through the HMD's lenses
/*!
	The use of lenses and distortion correction creates a screen in front of each eye.
	This function returns 2D vectors representing where on each eye's screen the user
	is looking.
	The vectors are normalized in the range [-1, 1] along both X and Y axes such that the
	following points are true:

	Center: (0, 0)
	Bottom-Left: (-1, -1)
	Top-Right: (1, 1)

	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outPos A pointer to the eye gaze point in the HMD's virtual screen space
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if outPos is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getGazeScreenPosition(Fove_Headset*, Fove_Eye eye, Fove_Vec2* outPos) FOVE_NOEXCEPT;

//! Writes out the user's 2D gaze position
/*!
	This is a 2D equivalent of `fove_Headset_getCombinedGazeRay`, and is perhaps the simplest gaze estimation function.

	It returns an X/Y coordinate of where on the screen the user is looking.

	While in reality each eye is looking in a different direction at a different [portion of the] screen,
	they mostly agree, and this function returns effectively an average to get you a simple X/Y value.

	Center: (0, 0)
	Bottom-Left: (-1, -1)
	Top-Right: (1, 1)

	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param outPos A pointer to the eye gaze point in the HMD's virtual screen space
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if both outPos is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getGazeScreenPositionCombined(Fove_Headset*, Fove_Vec2* outPos) FOVE_NOEXCEPT;

//! Writes out eyes gaze ray resulting from the two eye gazes combined together
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	To get individual eye rays use `fove_Headset_getGazeVector` instead

	\param  outRay  A pointer to the gaze ray struct to write to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outRay` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getCombinedGazeRay(Fove_Headset*, Fove_Ray* outRay) FOVE_NOEXCEPT;

//! Writes out eyes gaze depth resulting from the two eye gazes combined together
/*!
	`Fove_ClientCapabilities_GazeDepth` should be registered to use this function.

	\param  outDepth  A pointer to the gaze depth variable to write to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outDepth` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getCombinedGazeDepth(Fove_Headset*, float* outDepth) FOVE_NOEXCEPT;

//! Writes out whether the user is shifting its attention between objects or looking at something specific (fixation or pursuit)
/*!
	This can be used to ignore eye data during large eye motions when the user is not looking at anything specific.

	`Fove_ClientCapabilities_UserAttentionShift` should be registered to use this function.

	\param  outIsShiftingAttention A pointer to a output variable to write the user attention shift status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outIsShiftingAttention` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isUserShiftingAttention(Fove_Headset*, bool* outIsShiftingAttention) FOVE_NOEXCEPT;

//! Writes out the state of an individual eye
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param  outState A pointer to the output variable to write the eye state to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outState` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeState(Fove_Headset*, Fove_Eye eye, Fove_EyeState* outState) FOVE_NOEXCEPT;

//! Writes out whether the user is currently performing a blink for the given eye
/*!
	`Fove_ClientCapabilities_EyeBlink` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param  outIsBlinking A pointer to the output variable to write the blinking state to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outIsBlinking` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isEyeBlinking(Fove_Headset*, Fove_Eye eye, bool* outIsBlinking) FOVE_NOEXCEPT;

//! Writes out the number of blink performed for the given eye since the eye tracking service started
/*!
	To count the number blinks performed during a given period of time call this function at the
	beginning and at the end of the period and make the subtraction of the two values.

	`Fove_ClientCapabilities_EyeBlink` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param  outBlinkCount A pointer to the output variable to write the blink count to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outIsBlinking` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeBlinkCount(Fove_Headset*, Fove_Eye eye, int* outBlinkCount) FOVE_NOEXCEPT;

//! Writes out whether the eye tracking hardware has started
/*!
	\param  outEyeTrackingEnabled A pointer to the output variable to write the eye tracking status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outEyeTrackingEnabled` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isEyeTrackingEnabled(Fove_Headset*, bool* outEyeTrackingEnabled) FOVE_NOEXCEPT;

//! Writes out whether eye tracking has been calibrated
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param  outEyeTrackingCalibrated A pointer to the output variable to write the eye tracking calibrated status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outEyeTrackingCalibrated` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isEyeTrackingCalibrated(Fove_Headset*, bool* outEyeTrackingCalibrated) FOVE_NOEXCEPT;

//! Writes out whether eye tracking is in the process of performing a calibration
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param  outEyeTrackingCalibrating A pointer to the output variable to write the eye tracking calibrating status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outEyeTrackingCalibrating` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isEyeTrackingCalibrating(Fove_Headset*, bool* outEyeTrackingCalibrating) FOVE_NOEXCEPT;

//! Writes out whether the eye tracking system is currently calibrated for glasses
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	This basically indicates if the user was wearing glasses during the calibration or not.
	This function returns 'Data_Uncalibrated' if the eye tracking system has not been calibrated yet

	\param outGlasses A pointer to the variable to be written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Uncalibrated if the eye tracking system is currently uncalibrated\n
	        #Fove_ErrorCode_API_NullInPointer if `outGlasses` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isEyeTrackingCalibratedForGlasses(Fove_Headset*, bool* outGlasses) FOVE_NOEXCEPT;

//! Writes out whether or not the GUI that asks the user to adjust their headset is being displayed
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param  outHmdAdjustmentGuiVisible A pointer to the output variable to write the GUI visibility status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outHmdAdjustmentGuiVisible` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isHmdAdjustmentGuiVisible(Fove_Headset*, bool* outHmdAdjustmentGuiVisible) FOVE_NOEXCEPT;

//! Writes out whether the GUI that asks the user to adjust their headset was hidden by timeout
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param  outTimeout A pointer to the output variable to write the timeout status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outTimeout` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_hasHmdAdjustmentGuiTimeout(Fove_Headset*, bool* outTimeout) FOVE_NOEXCEPT;

//! Writes out whether eye tracking is actively tracking eyes
/*!
	In other words, it returns `true` only when the hardware is ready and eye tracking is calibrated.

	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param  outEyeTrackingReady A pointer to the output variable to write the eye tracking ready status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outEyeTrackingReady` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isEyeTrackingReady(Fove_Headset*, bool* outEyeTrackingReady) FOVE_NOEXCEPT;

//! Writes out whether the user is wearing the headset or not
/*!
	When user is not present Eye tracking values shouldn't be used, as invalid.

	`Fove_ClientCapabilities_UserPresence` should be registered to use this function.

	\param  outUserPresent A pointer to the output variable to write the user presence status to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outUserPresent` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isUserPresent(Fove_Headset*, bool* outUserPresent) FOVE_NOEXCEPT;

//! Returns the eyes camera image
/*!
	Returns the most recent eye tracking camera image cached in the last call to `fove_Headset_fetchEyeTrackingData`.

	The image data buffer is invalidated upon the next call to `fove_Headset_fetchEyeTrackingData`.

	`Fove_ClientCapabilities_EyesImage` should be registered to use this function.

	\param outImage the raw image data buffer to write the eyes image data to.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreadable if the data couldn't be read properly from memory\n
	        #Fove_ErrorCode_API_NullInPointer if `outImage` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyesImage(Fove_Headset*, Fove_BitmapImage* outImage) FOVE_NOEXCEPT;

//! Returns the user IPD (Inter Pupillary Distance), in meters
/*!
	`Fove_ClientCapabilities_UserIPD` should be registered to use this function.

	\param outIPD A pointer to the output variable to write the user IPD to.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outIPD` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getUserIPD(Fove_Headset*, float* outIPD) FOVE_NOEXCEPT;

//! Returns the user IOD (Inter Occular Distance), in meters
/*!
	`Fove_ClientCapabilities_UserIOD` should be registered to use this function.

	\param outIOD A pointer to the output variable to write the user IOD to.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outIOD` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getUserIOD(Fove_Headset*, float* outIOD) FOVE_NOEXCEPT;

//! Returns the user pupils radius, in meters
/*!
	`Fove_ClientCapabilities_PupilRadius` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outRadius A pointer to the output variable to write the user pupil radius to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if both `outRadius` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getPupilRadius(Fove_Headset*, Fove_Eye eye, float* outRadius) FOVE_NOEXCEPT;

//! Returns the user iris radius, in meters
/*!
	`Fove_ClientCapabilities_IrisRadius` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outRadius A pointer to the output variable to write the user iris radius to.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if both `outRadius` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getIrisRadius(Fove_Headset*, Fove_Eye eye, float* outRadius) FOVE_NOEXCEPT;

//! Returns the user eyeballs radius, in meters
/*!
	`Fove_ClientCapabilities_EyeballRadius` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outRadius A pointer to the output variable to write the user eyeball radius to.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if both `outRadius` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeballRadius(Fove_Headset*, Fove_Eye eye, float* outRadius) FOVE_NOEXCEPT;

//! Returns the user eye torsion, in degrees
/*!
	`Fove_ClientCapabilities_EyeTorsion` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outAngle A pointer to the output variable to write the user eye torsion to.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if both `outAngle` is `nullptr`\n
	        #Fove_ErrorCode_License_FeatureAccessDenied if the current license is not sufficient for this feature
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeTorsion(Fove_Headset*, Fove_Eye eye, float* outAngle) FOVE_NOEXCEPT;

//! Returns the outline shape of the specified user eye in the Eyes camera image
/*!
	`Fove_ClientCapabilities_EyeShape` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outShape A pointer to the EyeShape struct to write eye shape to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if both `outShape` is `nullptr`\n
	        #Fove_ErrorCode_License_FeatureAccessDenied if the current license is not sufficient for this feature
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeShape(Fove_Headset*, Fove_Eye eye, Fove_EyeShape* outShape) FOVE_NOEXCEPT;

//! Returns the pupil shape of the specified user eye in the Eyes camera image
/*!
	`Fove_ClientCapabilities_PupilShape` should be registered to use this function.

	\param eye Specify which eye to get the value for
	\param outShape A pointer to the PupilShape struct to write pupil shape to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outShape` is `nullptr`\n
	        #Fove_ErrorCode_License_FeatureAccessDenied if the current license is not sufficient for this feature
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getPupilShape(Fove_Headset*, Fove_Eye eye, Fove_PupilShape* outShape) FOVE_NOEXCEPT;

//! Start the HMD adjustment process. Doing this will display the HMD adjustment GUI.
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function

	\param lazy If true, the headset adjustment GUI doesn't show if the headset position is already perfect.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_startHmdAdjustmentProcess(Fove_Headset*, bool lazy) FOVE_NOEXCEPT;

//! Tick the current HMD adjustment process and retrieve data information to render the current HMD positioning state
/*!
	\param deltaTime The time elapsed since the last rendered frame
	\param isVisible Indicate to the FOVE system that GUI for HMD adjustment is being drawn to the screen.
	This allows the HMD adjustment renderer to take as much time as it wants to display fade-in/out or other animations
	before the HMD adjustment processes is marked as completed by the `IsHmdAdjustmentGUIVisible` function.
	\param outData The current HMD positioning information

	This function is how the client declares to the FOVE system that it is available to render the HMD adjustment process.
	The FOVE system determines which of the available renderers has the highest priority,
	and returns to that renderer the information needed to render HMD adjustment process via the outData parameter.
	Even while ticking this, you may get no result because either no HMD adjustment is running,
	or a HMD adjustment process is running but some other higher priority renderer is doing the rendering.

	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	Note that it is perfectly fine not to call this function, in which case the Fove service will automatically render the HMD adjustment process for you.

	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
			#Fove_ErrorCode_API_NullInPointer if both `outData` is `nullptr`\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_License_FeatureAccessDenied if a sufficient license is not registered on this machine\n
	        #Fove_ErrorCode_Render_OtherRendererPrioritized if another process has currently the priority for rendering the process
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_tickHmdAdjustmentProcess(Fove_Headset*, float deltaTime, bool isVisible, Fove_HmdAdjustmentData* outData) FOVE_NOEXCEPT;

//! Starts eye tracking calibration
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function

	\param options The calibration options to use, or null to use default options
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_License_FeatureAccessDenied if any of the enabled options require a license beyond what is active on this machine
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_startEyeTrackingCalibration(Fove_Headset*, const Fove_CalibrationOptions* options) FOVE_NOEXCEPT;

//! Stops eye tracking calibration if it's running, does nothing if it's not running
/*
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_stopEyeTrackingCalibration(Fove_Headset*) FOVE_NOEXCEPT;

//! Get the state of the currently running calibration process
/*
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	\param outCalibrationState A pointer to the calibration state variable to write to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outCalibrationState` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeTrackingCalibrationState(Fove_Headset*, Fove_CalibrationState* outCalibrationState) FOVE_NOEXCEPT;

//! Get the detailed information about the state of the currently running calibration process.
/*!
	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	When the calibration process is not running, this returns the final state of the previously run calibration process.
	Value is undefined if no calibration process has begun since the service was started.

	\param callback A function pointer to a function that will be called up if successful
	\param callbackData An arbitrary user pointer to pass to the callback
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `callback ` is `nullptr`
	\see    fove_Headset_fetchEyeTrackingData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeTrackingCalibrationStateDetails(Fove_Headset*, void(FOVE_CALLBACK* callback)(const Fove_CalibrationData* detailsData, void* callbackData), void* callbackData) FOVE_NOEXCEPT;

//! Tick the current calibration process and retrieve data information to render the current calibration state
/*!
	\param deltaTime The time elapsed since the last rendered frame
	\param isVisible Indicate to the calibration system that something is being drawn to the screen.
	                 This allows the calibration renderer to take as much time as it wants to display success/failure messages
	                 and animate away before the calibration processes is marked as completed by the `IsEyeTrackingCalibrating` function.
	\param callback A function pointer to a function that will be called up if successful
	\param callbackData An arbitrary user pointer to pass to the callback

	This function is how the client declares to the calibration system that it is available to render calibration.
	The calibration system determines which of the available renderers has the highest priority,
	and returns to that renderer the information needed to render calibration via the outCalibrationData parameter.
	Even while ticking this, you may get no result because either no calibration is running,
	or a calibration is running but some other higher priority renderer is doing the rendering.

	`Fove_ClientCapabilities_EyeTracking` should be registered to use this function.

	Note that it is perfectly fine not to call this function, in which case the Fove service will automatically render the calibration process for you.

	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_License_FeatureAccessDenied if a sufficient license is not registered on this machine\n
	        #Fove_ErrorCode_Render_OtherRendererPrioritized if another process has currently the priority for rendering calibration process\n
	        #Fove_ErrorCode_API_NullInPointer if `callback ` is `nullptr`
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_tickEyeTrackingCalibration(Fove_Headset*, float deltaTime, bool isVisible, void(FOVE_CALLBACK* callback)(const Fove_CalibrationData* calibrationData, void* callbackData), void* callbackData) FOVE_NOEXCEPT;

//! Get the id of the object gazed by the user
/*!
	In order to be detected an object first need to be registered using the `fove_Headset_registerGazableObject` function.
	If the user is currently not looking at any specific object the `fove_ObjectIdInvalid` value is returned.
	To use this function, you need to register the `Fove_ClientCapabilities_GazedObjectDetection` first.

	This function returns data cached in the last call to `fove_Headset_fetchEyeTrackingData()`.

	\param outObjectId A pointer to the output id identifying the object the user is currently looking at
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outObjectId` is `nullptr`

	\see                fove_Headset_fetchEyeTrackingData
	\see                fove_Headset_removeGazableObject
	\see                fove_Headset_updateGazableObject
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getGazedObjectId(Fove_Headset*, int* const outObjectId) FOVE_NOEXCEPT;

//! Registers an object in the 3D world
/*!
	Registering 3D world objects allows FOVE software to identify which objects are being gazed at.
	We recommend that clients opt-in to this functionality rather than doing it themselves, as our algorithm may improve over time.
	Clients of course may do their own detection if they have special needs, such as performance needs, or just want to use their own algorithm.

	Use #fove_Headset_registerCameraObject to set the pose of the corresponding camera in the 3D world.

	Connection to the service is not required for object registration, thus you can register your world objects at will and not worry about connection or reconnection status.

	\param object       A description of the object in the 3D world. Data is copied and no reference is kept to this memory after return.
	\return             #Fove_ErrorCode_None if the object is successfully added or updated\n
	                    #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	                    #Fove_ErrorCode_API_NullInPointer if either parameter is null\n
	                    #Fove_ErrorCode_API_InvalidArgument if the object is invalid in any way (such as an invalid object id)\n
	                    #Fove_ErrorCode_Object_AlreadyRegistered if an object with same id is already registered
	\see                fove_Headset_removeGazableObject
	\see                fove_Headset_updateGazableObject
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_registerGazableObject(Fove_Headset*, const Fove_GazableObject* object) FOVE_NOEXCEPT;

//! Update a previously registered 3D object pose
/*!
	\param objectId     Id of the object passed to fove_Headset_registerGazableObject()
	\param pose         the updated pose of the object
	\return             #Fove_ErrorCode_None if the object was in the scene and is now updated\n
	                    #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	                    #Fove_ErrorCode_API_NullInPointer if either parameter is null\n
	                    #Fove_ErrorCode_API_InvalidArgument if the object was not already registered
	\see                fove_Headset_registerCameraObject
	\see                fove_Headset_removeGazableObject
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_updateGazableObject(Fove_Headset*, int objectId, const Fove_ObjectPose* pose) FOVE_NOEXCEPT;

//! Removes a previously registered 3D object from the scene
/*!
	Because of the asynchronous nature of the FOVE system, this object may still be referenced in future frames for a very short period of time.

	\param objectId     Id of the object passed to fove_Headset_registerGazableObject()
	\return             #Fove_ErrorCode_None if the object was in the scene and is now removed\n
	                    #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	                    #Fove_ErrorCode_API_InvalidArgument if the object was not already registered
	\see                fove_Headset_registerGazableObject
	\see                fove_Headset_updateGazableObject
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_removeGazableObject(Fove_Headset*, int objectId) FOVE_NOEXCEPT;

//! Registers an camera in the 3D world
/*!
	Registering 3D world objects and camera allows FOVE software to identify which objects are being gazed at.
	We recommend that clients opt-in to this functionality rather than doing it themselves, as our algorithm may improve over time.
	Clients of course may do their own detection if they have special needs, such as performance needs, or just want to use their own algorithm.

	At least 1 camera needs to be registered for automatic object gaze recognition to work. Use the object group mask of the camera to
	specify which objects the camera is capturing. The camera view pose determine the gaze raycast direction and position.
	The camera view pose should include any and all offsets from position tracking. No transforms from the headset are added in automatically.

	Connection to the service is not required for object registration, thus you can register your world objects at will and not worry about connection or reconnection status.

	\param camera       A description of the camera. Data is copied and no reference is kept to this memory after return.
	\return             #Fove_ErrorCode_None if the camera is successfully added or updated\n
	                    #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	                    #Fove_ErrorCode_API_NullInPointer if either parameter is null\n
	                    #Fove_ErrorCode_API_InvalidArgument if the object is invalid in any way (such as an invalid object id)\n
	                    #Fove_ErrorCode_Object_AlreadyRegistered if an object with same id is already registered
	\see                fove_Headset_removeCameraObject
	\see                fove_Headset_updateCameraObject
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_registerCameraObject(Fove_Headset*, const Fove_CameraObject* camera) FOVE_NOEXCEPT;

//! Update the pose of a registered camera
/*!
	\param cameraId     Id of the camera passed to fove_Headset_registerCameraObject()
	\param pose         the updated pose of the camera
	\return             #Fove_ErrorCode_None if the object was in the scene and is now removed\n
	                    #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	                    #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	                    #Fove_ErrorCode_API_NullInPointer if either parameter is null\n
	\see                fove_Headset_registerCameraObject
	\see                fove_Headset_removeCameraObject
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_updateCameraObject(Fove_Headset*, int cameraId, const Fove_ObjectPose* pose) FOVE_NOEXCEPT;

//! Removes a previously registered camera from the scene
/*!
	\param cameraId     Id of the camera passed to fove_Headset_registerCameraObject()
	\return             #Fove_ErrorCode_None if the object was in the scene and is now removed\n
	                    #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	                    #Fove_ErrorCode_API_InvalidArgument is returned if the object was not already registered
	\see                fove_Headset_registerCameraObject
	\see                fove_Headset_updateCameraObject
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_removeCameraObject(Fove_Headset*, int cameraId) FOVE_NOEXCEPT;

//! Tares the orientation of the headset
/*!
	Any or both of `Fove_ClientCapabilities_OrientationTracking` and `Fove_ClientCapabilities_PositionTracking`
	should be registered to use this function.

	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_tareOrientationSensor(Fove_Headset*) FOVE_NOEXCEPT;

//! Writes out whether position tracking hardware has started and returns whether it was successful
/*!
	`Fove_ClientCapabilities_PositionTracking` should be registered to use this function.

	\param outPositionReady A pointer to the variable to be written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outPositionReady` is `nullptr`
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_isPositionReady(Fove_Headset*, bool* outPositionReady) FOVE_NOEXCEPT;

//! Tares the position of the headset
/*!
	`Fove_ClientCapabilities_PositionTracking` should be registered to use this function.

	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_tarePositionSensors(Fove_Headset*) FOVE_NOEXCEPT;

//! Fetch the latest pose data, and cache it locally
/*!
	This function caches the headset pose for later retrieval by `fove_Headset_getPose`.

	This function never blocks the thread. If no new data is available, no operation is performed.
	The timestamp can be used to know if the data has been updated or not.

	The HMD pose is updated at much higher frame rate than the eye tracking data and there is no equivalent to
	`fove_Headset_waitForProcessedEyeFrame` for the pose. For rendering purposes you should use the pose returned by
	`fove_Compositor_waitForRenderPose` which provides the best render pose estimate for the current frame.
	For other purposes, just fetch the HMD pose once at the beginning of your update loop. This will ensure consistent data
	throughout all your update loop code.

	\param outTimestamp A pointer to write the frame timestamp of fetched data. If null, the timestamp is not written.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if neither position nor orientation tracking is registered
	\see    fove_Headset_getPose
	\see    fove_Compositor_waitForRenderPose
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_fetchPoseData(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Fetch the latest position camera image, and cache it locally
/*!
	This function caches the position camera image for later retrieval by `fove_Headset_getPositionImage`.

	This function never blocks the thread. If no new data is available, no operation is performed.
	The timestamp can be used to know if the data has been updated or not.

	There is no equivalent to `fove_Headset_waitForProcessedEyeFrame` for the position image that allow you to synchronize
	with the position image update. We recommend you to fetch the position image only once every beginning of update
	loop if needed to ensure consistent data throughout the update loop code.

	\param outTimestamp A pointer to the timestamp of fetched data. If null, the timestamp is not written.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if neither position nor orientation tracking is registered
	\see    fove_Headset_getPositionImage
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_fetchPositionImage(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Writes out the frame timestamp of the cached pose data
/*!
	Basically returns the timestamp returned by the last call to `fove_Headset_fetchPoseData`.

	\param outTimestamp A pointer to write the frame timestamp of the currently cached data.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if neither position nor orientation tracking is registered\n
	        #Fove_ErrorCode_API_NullInPointer if outTimestamp is null
	\see    fove_Headset_fetchPoseData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getPoseDataTimestamp(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Writes out the frame timestamp of the cached position image
/*!
	Basically returns the timestamp returned by the last call to `fove_Headset_fetchPositionImage`.

	\param outTimestamp A pointer to write the frame timestamp of the currently cached data.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if not connected to the service or if the service hasn't written any data out yet\n
	        #Fove_ErrorCode_API_NotRegistered if position image is not registered\n
	        #Fove_ErrorCode_API_NullInPointer if outTimestamp is null
	\see    fove_Headset_fetchPositionImage
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getPositionImageTimestamp(Fove_Headset*, Fove_FrameTimestamp* outTimestamp) FOVE_NOEXCEPT;

//! Writes out the pose of the head-mounted display
/*!
	\param outPose  A pointer to the variable to be written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreliable if the returned data is too unreliable to be used\n
	        #Fove_ErrorCode_Data_LowAccuracy if the returned data is of low accuracy\n
	        #Fove_ErrorCode_API_NullInPointer if `outPose` is `nullptr`
	\see    fove_Headset_fetchPoseData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getPose(Fove_Headset*, Fove_Pose* outPose) FOVE_NOEXCEPT;

//! Returns the position camera image
/*!
	Returns the most recent position tracking image cached in the last call to `fove_Headset_fetchPoseData`.

	The image data buffer is invalidated upon the next call to `fove_Headset_fetchPoseData`.

	`Fove_ClientCapabilities_PositionImage` should be registered to use this function.

	\param outImage the raw image data buffer to write the position image data to.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NotRegistered if the required capability has not been registered prior to this call\n
	        #Fove_ErrorCode_Data_NoUpdate if the capability is registered but no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_Data_Unreadable if the data couldn't be read properly from memory
	        #Fove_ErrorCode_API_NullInPointer if `outImage` is `nullptr`
	\see    fove_Headset_fetchPoseData
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getPositionImage(Fove_Headset*, Fove_BitmapImage* outImage) FOVE_NOEXCEPT;

//! Writes out the values of passed-in left-handed 4x4 projection matrices
/*!
	Writes 4x4 projection matrices for both eyes using near and far planes in a left-handed coordinate
	system. Either outLeftMat or outRightMat may be `nullptr` to only write the other matrix, however setting
	both to `nullptr` is considered invalid and will return `Fove_ErrorCode::API_NullOutPointersOnly`.
	\param zNear        The near plane in float, Range: from 0 to zFar
	\param zFar         The far plane in float, Range: from zNear to infinity
	\param outLeftMat   A pointer to the matrix you want written
	\param outRightMat  A pointer to the matrix you want written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if both `outLeftMat` and `outRightMat` are `nullptr`
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getProjectionMatricesLH(Fove_Headset*, float zNear, float zFar, Fove_Matrix44* outLeftMat, Fove_Matrix44* outRightMat) FOVE_NOEXCEPT;

//! Writes out the values of passed-in right-handed 4x4 projection matrices
/*!
	Writes 4x4 projection matrices for both eyes using near and far planes in a right-handed coordinate
	system. Either outLeftMat or outRightMat may be `nullptr` to only write the other matrix, however setting
	both to `nullptr` is considered invalid and will return `Fove_ErrorCode::API_NullOutPointersOnly`.
	\param zNear        The near plane in float, Range: from 0 to zFar
	\param zFar         The far plane in float, Range: from zNear to infinity
	\param outLeftMat   A pointer to the matrix you want written
	\param outRightMat  A pointer to the matrix you want written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if both `outLeftMat` and `outRightMat` are `nullptr`
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getProjectionMatricesRH(Fove_Headset*, float zNear, float zFar, Fove_Matrix44* outLeftMat, Fove_Matrix44* outRightMat) FOVE_NOEXCEPT;

//! Writes out values for the view frustum of the specified eye at 1 unit away
/*!
	Writes out values for the view frustum of the specified eye at 1 unit away. Please multiply them by zNear to
	convert to your correct frustum near-plane. Either outLeft or outRight may be `nullptr` to only write the
	other struct, however setting both to `nullptr` is considered and error and the function will return
	`Fove_ErrorCode::API_NullOutPointersOnly`.
	\param outLeft  A pointer to the struct describing the left camera projection parameters
	\param outRight A pointer to the struct describing the right camera projection parameters
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if both `outLeft` and `outRight` are `nullptr`
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getRawProjectionValues(Fove_Headset*, Fove_ProjectionParams* outLeft, Fove_ProjectionParams* outRight) FOVE_NOEXCEPT;

//! Writes out the matrices to convert from eye to head space coordinates
/*!
	This is simply a translation matrix that returns +/- IOD/2
	\param outLeft   A pointer to the matrix where left-eye transform data will be written
	\param outRight  A pointer to the matrix where right-eye transform data will be written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if both `outLeft` and `outRight` are `nullptr`
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getEyeToHeadMatrices(Fove_Headset*, Fove_Matrix44* outLeft, Fove_Matrix44* outRight) FOVE_NOEXCEPT;

//! Interocular distance to use for rendering in meters
/*!
	This may or may not reflect the actual IOD of the user (see getUserIOD),
	but is the value used by the rendering system for the distance to split the left/right
	cameras for stereoscopic rendering.
	We recommend calling this each frame when doing stereoscopic rendering.

	\param outIOD A pointer to the render IOD variable to write to
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Data_NoUpdate if no valid data has been returned by the service yet\n
	        #Fove_ErrorCode_API_NullInPointer if `outIOD` is `nullptr`
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_getRenderIOD(Fove_Headset*, float* outIOD) FOVE_NOEXCEPT;

//! Creates a new profile
/*!
	The FOVE system keeps a set of profiles so that different users on the same system can store data, such as calibrations, separately.
	Profiles persist to disk and survive restart.
	Third party applications can control the profile system and store data within it.

	This function creates a new profile, but does not add any data or switch to it.
	\param newName Null-terminated UTF-8 unique name of the profile to create
	\return #Fove_ErrorCode_None if the profile was successfully created\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_Profile_InvalidName if newName was invalid\n
	        #Fove_ErrorCode_Profile_NotAvailable if the name is already taken\n
	        #Fove_ErrorCode_API_NullInPointer if newName is null
	\see fove_Headset_deleteProfile
	\see fove_Headset_queryCurrentProfile
	\see fove_Headset_queryProfileDataPath
	\see fove_Headset_listProfiles
	\see fove_Headset_renameProfile
	\see fove_Headset_setCurrentProfile
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_createProfile(Fove_Headset*, const char* newName) FOVE_NOEXCEPT;

//! Renames an existing profile
/*!
	This function renames an existing profile. This works on the current profile as well.
	\param oldName Null-terminated UTF-8 name of the profile to be renamed
	\param newName Null-terminated UTF-8 unique new name of the profile
	\return #Fove_ErrorCode_None if the profile was successfully renamed\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_Profile_DoesntExist if the requested profile at oldName doesn't exist\n
	        #Fove_ErrorCode_Profile_NotAvailable If the new named is already taken\n
	        #Fove_ErrorCode_API_InvalidArgument If the old name and new name are the same\n
	        #Fove_ErrorCode_API_NullInPointer if oldName or newName is null
	\see fove_Headset_createProfile
	\see fove_Headset_deleteProfile
	\see fove_Headset_queryCurrentProfile
	\see fove_Headset_queryProfileDataPath
	\see fove_Headset_listProfiles
	\see fove_Headset_setCurrentProfile
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_renameProfile(Fove_Headset*, const char* oldName, const char* newName) FOVE_NOEXCEPT;

//! Deletes an existing profile
/*!
	This function deletes an existing profile.

	If the deleted profile is the current profile, then no current profile is set after this returns.
	In such a case, it is undefined whether any existing profile data loaded into memory may be kept around.

	\param profileName Null-terminated UTF-8 name of the profile to be deleted
	\return #Fove_ErrorCode_None if the profile was successfully deleted\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_Profile_DoesntExist if the requested profile at profileName doesn't exist\n
	        #Fove_ErrorCode_API_NullInPointer if profileName is null
	\see fove_Headset_createProfile
	\see fove_Headset_queryCurrentProfile
	\see fove_Headset_queryProfileDataPath
	\see fove_Headset_listProfiles
	\see fove_Headset_renameProfile
	\see fove_Headset_setCurrentProfile
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_deleteProfile(Fove_Headset*, const char* profileName) FOVE_NOEXCEPT;

//! Lists all existing profiles
/*!
	Invoking any other function on this headset from the callback will yield undefined behavior.

	callbackProfileName is a non-null, null-terminated UTF-8 string containing the name of a profile.

	\param callback A function pointer to a function that will be called once for each profile, in no particular order
	\param callbackData An arbitrary user pointer to pass to the callback
	\return #Fove_ErrorCode_None if the profile names were successfully listed\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NullInPointer if callback is null
	\see fove_Headset_createProfile
	\see fove_Headset_deleteProfile
	\see fove_Headset_queryCurrentProfile
	\see fove_Headset_queryProfileDataPath
	\see fove_Headset_renameProfile
	\see fove_Headset_setCurrentProfile
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_listProfiles(Fove_Headset*, void(FOVE_CALLBACK* callback)(const char* callbackProfileName, void* callbackData), void* callbackData) FOVE_NOEXCEPT;

//! Sets the current profile
/*!
	When changing profile, the FOVE system will load up data, such as calibration data, if it is available.
	If loading a profile with no calibration data, whether or not the FOVE system keeps old data loaded into memory is undefined.

	Please note that no-ops are OK but you should check for #Fove_ErrorCode_Profile_NotAvailable.

	\param profileName Name of the profile to make current, in UTF-8
	\return #Fove_ErrorCode_None if the profile was successfully set as the current profile\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_Profile_DoesntExist if there is no such profile\n
	        #Fove_ErrorCode_Profile_NotAvailable if the requested profile is the current profile\n
	        #Fove_ErrorCode_API_NullInPointer if profileName is null
	\see fove_Headset_createProfile
	\see fove_Headset_deleteProfile
	\see fove_Headset_queryCurrentProfile
	\see fove_Headset_queryProfileDataPath
	\see fove_Headset_listProfiles
	\see fove_Headset_renameProfile
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_setCurrentProfile(Fove_Headset*, const char* profileName) FOVE_NOEXCEPT;

//! Gets the current profile
/*!
	The callback is not invoked if there is no current profile.

	callbackProfileName is a non-null, null-terminated UTF-8 string with the name of the current profile.

	Invoking any other function on this headset from the callback will yield undefined behavior.
	\param callback A function pointer to a function that will be called up to onces if successful,
	or an empty string if no profile is set.
	\param callbackData An arbitrary user pointer to pass to the callback
	\return #Fove_ErrorCode_None if the profile name was successfully retrieved\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_API_NullInPointer if callback is null
	\see fove_Headset_createProfile
	\see fove_Headset_deleteProfile
	\see fove_Headset_queryProfileDataPath
	\see fove_Headset_listProfiles
	\see fove_Headset_renameProfile
	\see fove_Headset_setCurrentProfile
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_queryCurrentProfile(Fove_Headset*, void(FOVE_CALLBACK* callback)(const char* callbackProfileName, void* callbackData), void* callbackData) FOVE_NOEXCEPT;

//! Gets the data folder for a given profile
/*!
	Allows you to retrieve a filesytem directory where third party apps can write data associated with this profile. This directory will be created before return.

	Since multiple applications may write stuff to a profile, please prefix any files you create with something unique to your application.

	There are no special protections on profile data, and it may be accessible to any other app on the system. Do not write sensitive data here.

	This is intended for simple uses. For advanced uses that have security concerns, or want to sync to a server, etc,
	third party applications are encouraged to use their own separate data store keyed by profile name.
	They will need to test for profile name changes and deletions manually in that case.

	Invoking any other function on this headset from the callback will yield undefined behavior.

	callbackProfileName is a non-null, null-terminated UTF-8 string with the path.

	\param callback A function pointer to a function that will be called once if successful. Not invoked in the event of error
	\param callbackData An arbitrary user pointer to pass to the callback
	\param profileName A null-terminated UTF-8 string with the name of the profile to be queried, or an empty string if no profile is set
	\return #Fove_ErrorCode_None if the profile was successfully deleted\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_Profile_DoesntExist if there is no such profile\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_API_NullInPointer if profileName or callback is null
	\see fove_Headset_createProfile
	\see fove_Headset_deleteProfile
	\see fove_Headset_queryCurrentProfile
	\see fove_Headset_listProfiles
	\see fove_Headset_renameProfile
	\see fove_Headset_setCurrentProfile
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_queryProfileDataPath(Fove_Headset*, const char* profileName, void(FOVE_CALLBACK* callback)(const char* callbackProfileName, void* callbackData), void* callbackData) FOVE_NOEXCEPT;

//! Returns whether the Headset has access to the given feature
/*!
	If the provided feature name doesn't exist, then `false` and `#Fove_ErrorCode_None` are returned.

	\param inFeatureName A null-terminated UTF-8 string with the name of the feature to query
	\param outHasAccess Output variable set to true if the headset can access the given feature
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NullInPointer if inFeatureName is null\n
	        #Fove_ErrorCode_API_NullOutPointersOnly if outHasAccess is null
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_hasAccessToFeature(Fove_Headset*, const char* inFeatureName, bool* outHasAccess) FOVE_NOEXCEPT;

//! Activates a license
/*!
	A license must be activated before using most features of the FOVE system.

	Since FOVE provides default means (GUI/CLI) to activate licenses, this is not strictly required for most applications.

	However, this function provides a programmatic means for the application to handle license key entry itself.
	An application may fully script license activation, so that the end user doesn't need to enter anything,
	or the application may display a prompt the user in it's own GUI to enter a license key, depending on the use case.

	Once a license is activated it is persisted to disk, so this does not need to be called repeatedly.

	If there is an existing license, it will be deactivated automatically.
	Currently only one license at a time is supported, but this may change in the future.

	If the license has a PC limit, this will take one slot within that limit.

	Because this makes a remote server call, this function will sometimes block a little while.
	It is not recommended to call this from your GUI thread, or other realtime threads.

	Activating a license multiple times from the same machine is a no-op, and will return Fove_ErrorCode_API_AlreadyInTheDesiredState.

	\param licenseKey Full text of the license key to be activated
	\return #Fove_ErrorCode_None if the activation succeeded\n
	        #Fove_ErrorCode_API_AlreadyInTheDesiredState if the license was already activated\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid or the license key was invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_Timeout if the call is not completed within a period of time\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
	        #Fove_ErrorCode_License_Expired if the license was already expired\n
	        #Fove_ErrorCode_License_ClockError if the local clock is invalid\n
	        #Fove_ErrorCode_License_TooManyActivations if the license has been activated too many times\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_activateLicense(Fove_Headset*, const char* licenseKey) FOVE_NOEXCEPT;

//! Deactivates a previously activated license
/*!
	Due to caching, features of the license may become unavailable shortly after this call, not necessarily immediately.

	If the license has a PC limit, this PC will no longer count against it.

	Because this makes a remote server call, this function will sometimes block a little while.
	It is not recommended to call this from your GUI thread, or other realtime threads.

	\param licenseData A GUID of license to be deactivated, or the full text of a license key to be deactivated, or an empty string if all licenses need to be deactivated.
	\return #Fove_ErrorCode_None if the deactivation succeeded\n
	        #Fove_ErrorCode_API_AlreadyInTheDesiredState if no license has been deactivated during the operation\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid or if the license data was invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_Timeout if the call is not completed within a period of time\n
	        #Fove_ErrorCode_Connect_NotConnected if not connected to the service\n
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_deactivateLicense(Fove_Headset*, const char* licenseData) FOVE_NOEXCEPT;

//! Returns a compositor interface from the given headset
/*!
	Each call to this function creates a new object. The object should be destroyed with fove_Compositor_destroy
	It is fine to call this function multiple times with the same headset, the same pointer will be returned.
	It is ok for the compositor to outlive the headset passed in.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the headset object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NullInPointer if the outCompositor pointer is null
	\see fove_Compositor_destroy
*/
FOVE_EXPORT Fove_ErrorCode fove_Headset_createCompositor(Fove_Headset*, Fove_Compositor** outCompositor) FOVE_NOEXCEPT;

//! Frees resources used by the compositor object, including memory and sockets
/*!
	Upon return, this compositor pointer should no longer be used.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the compositor object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see fove_Headset_createCompositor
*/
FOVE_EXPORT Fove_ErrorCode fove_Compositor_destroy(Fove_Compositor*) FOVE_NOEXCEPT;

//! Creates a new layer within the compositor
/*!
	This function create a layer upon which frames may be submitted to the compositor by this client.

	A connection to the compositor must exist for this to pass.
	This means you need to wait for fove_Compositor_isReady before calling this function.
	However, if connection to the compositor is lost and regained, this layer will persist.
	For this reason, you should not recreate your layers upon reconnection, simply create them once.

	There is no way to delete a layer once created, other than to destroy the Fove_Compositor object.
	This is a feature we would like to add in the future.

	\param layerInfo The settings for the layer to be created
	\param outLayer A struct where the defaults of the newly created layer will be written
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the compositor object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NullInPointer if the outLayer pointer is null
	        #Fove_ErrorCode_API_Timeout if request reached timeout before completing. This usually indicate that the connection to the compositor has just been lost\n
	        #Fove_ErrorCode_Compositor_DisconnectedFromRuntime if the client is currently disconnected from the compositor
	\see fove_Compositor_submit
*/
FOVE_EXPORT Fove_ErrorCode fove_Compositor_createLayer(Fove_Compositor*, const Fove_CompositorLayerCreateInfo* layerInfo, Fove_CompositorLayer* outLayer) FOVE_NOEXCEPT;

//! Submit a frame to the compositor
/*! This function takes the feed from your game engine to the compositor for output.

	Before submitting frames to the compositor you need to create render layers using the `fove_Compositor_createLayer` function.
	The `fove_Compositor_createLayer` function returns an unique layer id for each layer created.
	Referencing these ids in `submitInfo` you can specify which layer to submit a frame to

	A connection to the compositor must have been established for this to pass.
	If the connection hasn't completed yet, `fove_ErrorCode_Compositor_DisconnectedFromRuntime` is returned.
	In this case you should retry calling this function periodically until it succeeds.

	In case of disconnection or crash of the compositor, the created layers persist.
	For this reason, you don't need to recreate the layers when it happens.
	Just keep submitting until the compositor restarts and connection is recovered.

	\param submitInfo   An array of layerCount Fove_CompositorLayerSubmitInfo structs, each of which provides texture data for a unique layer
	\param layerCount   The number of layers you are submitting
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the compositor object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_Timeout if request reached timeout before completing. This usually indicate that the connection to the compositor has just been lost\n
	        #Fove_ErrorCode_Compositor_DisconnectedFromRuntime if the client is currently disconnected from the compositor
	\see fove_Compositor_createLayer
*/
FOVE_EXPORT Fove_ErrorCode fove_Compositor_submit(Fove_Compositor*, const Fove_CompositorLayerSubmitInfo* submitInfo, size_t layerCount) FOVE_NOEXCEPT;

//! Wait for the next pose to use for rendering purposes
/*! All compositor clients should use this function as the sole means of limiting their frame rate.
	This allows the client to render at the correct frame rate for the HMD display and with the most adequate HMD pose.
	Upon this function returning, the client should proceed directly to rendering, to reduce the chance of missing the frame.

	If outPose is not null, this function returns the pose that should be use to render the current frame.
	If outPose is null, fove_Compositor_getLastRenderPose will return the same pose as the last call, or oherwise a default pose if never called with non-null.
	This pose can also be get later using the `fove_Compositor_getLastRenderPose` function.

	In general, a client's main loop should look like:
	{
		Update();                            // Run AI, physics, etc, for the next frame
		compositor.WaitForRenderPose(&pose); // Wait for the next frame, and get the pose
		Draw(pose);                          // Render the scene using the new pose
	}

	\param outPose The latest pose of the headset.
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_Timeout if request reached timeout before completing. This usually indicate that the connection to the compositor has just been lost\n
	        #Fove_ErrorCode_Compositor_DisconnectedFromRuntime if the client is currently disconnected from the compositor
	        #Fove_ErrorCode_API_InvalidArgument if the compositor object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	\see fove_Compositor_getLastRenderPose
*/
FOVE_EXPORT Fove_ErrorCode fove_Compositor_waitForRenderPose(Fove_Compositor*, Fove_Pose* outPose) FOVE_NOEXCEPT;

//! Get the last cached pose for rendering purposes
FOVE_EXPORT Fove_ErrorCode fove_Compositor_getLastRenderPose(Fove_Compositor*, Fove_Pose* outPose) FOVE_NOEXCEPT;

//! Returns true if we are connected to a running compositor and ready to submit frames for compositing
FOVE_EXPORT Fove_ErrorCode fove_Compositor_isReady(Fove_Compositor*, bool* outIsReady) FOVE_NOEXCEPT;

//! Returns the ID of the GPU currently attached to the headset
/*! For systems with multiple GPUs, submitted textures to the compositor must from the same GPU that the compositor is using

	A connection to the compositor must exist for this to pass.
	This means you need to wait for fove_Compositor_isReady before calling this function.

	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the compositor object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_Timeout if request reached timeout before completing. This usually indicate that the connection to the compositor has just been lost\n
	        #Fove_ErrorCode_Compositor_DisconnectedFromRuntime if the client is currently disconnected from the compositor
*/
FOVE_EXPORT Fove_ErrorCode fove_Compositor_queryAdapterId(Fove_Compositor*, Fove_AdapterId* outAdapterId) FOVE_NOEXCEPT;

//! Fetches the GPU-side mirror texture for the compositor
/*! This texture is updated by the compositor each frame and is intended to be used for mirroring the HMD display to the PC screen.
	It contains the left and right side in one texture, and should be called every frame.

	You must invoke fove_Compositor_submit() once and wait a frame before this can function.
	This is required to attach to your graphics content and setup resources.
	You must also pass the correct type for outTexture, based on the API passed to the submit function.

	\param outTexture A pointer to the texture to write the mirror texture to
	\param outWidth The width of the mirror texture
	\param outHeight The height of the mirror texture
	\return #Fove_ErrorCode_None if the call succeeded\n
	        #Fove_ErrorCode_API_InvalidArgument if the compositor object is invalid\n
	        #Fove_ErrorCode_UnknownError if an unexpected internal error occurred\n
	        #Fove_ErrorCode_API_NullOutPointersOnly if outTexture is null\n
	        #Fove_ErrorCode_Compositor_NotSwapped if one output frame involving fove_Compositor_submit() hasn't been completed\n
	        #Fove_ErrorCode_Compositor_UnableToUseTexture if invoked on a non-Windows platform or the compositor can't expose a cross-process DXGI texture\n
	        #Fove_ErrorCode_Compositor_DisconnectedFromRuntime if the client is currently disconnected from the compositor
*/
FOVE_EXPORT Fove_ErrorCode fove_Compositor_getMirrorTexture(Fove_Compositor*, Fove_CompositorTexture* outTexture, int* outWidth, int* outHeight) FOVE_NOEXCEPT;

//! Get the value of the provided key from the FOVE config
/*!
	\param key The key name of the value to retrieve, null-terminated and in UTF-8
	\param outValue The value associated to the key if found.
	\return #Fove_ErrorCode_None if the value was successfully retrieved\n
	        #Fove_ErrorCode_API_NullInPointer if key or outValue is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the queried key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not a boolean
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_getValue_bool(const char* key, bool* outValue) FOVE_NOEXCEPT;

//! Get the value of the provided key from the FOVE config
/*!
	\param key The key name of the value to retrieve, null-terminated and in UTF-8
	\param outValue The value associated to the key if found.
	\return #Fove_ErrorCode_None if the value was successfully retrieved\n
	        #Fove_ErrorCode_API_NullInPointer if key or outValue is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the queried key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not an int
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_getValue_int(const char* key, int* outValue) FOVE_NOEXCEPT;

//! Get the value of the provided key from the FOVE config
/*!
	\param key The key name of the value to retrieve, null-terminated and in UTF-8
	\param outValue The value associated to the key if found.
	\return #Fove_ErrorCode_None if the value was successfully retrieved\n
	        #Fove_ErrorCode_API_NullInPointer if key or outValue is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the queried key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not a float
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_getValue_float(const char* key, float* outValue) FOVE_NOEXCEPT;

//! Get the value of the provided key from the FOVE config
/*!
	\param key The key name of the value to retrieve, null-terminated and in UTF-8
	\param callback A function pointer to a function that will be called once if the key is present
	\param callbackData An arbitrary pointer passed to the callback
	\return #Fove_ErrorCode_None if the value was successfully retrieved\n
	        #Fove_ErrorCode_API_NullInPointer if key or outValue is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the queried key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not a string
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_getValue_string(const char* key, void(FOVE_CALLBACK* callback)(const char* value, void* callbackData), void* callbackData) FOVE_NOEXCEPT;

//! Set the value of the provided key to the FOVE config
/*!
	\param key The key name of the value to set, null-terminated and in UTF-8
	\param value The new value to set as the key value.
	\return #Fove_ErrorCode_None if the value was successfully set\n
	        #Fove_ErrorCode_API_NullInPointer if key is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the provided key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not a boolean\n
	        #Fove_ErrorCode_System_AccessDenied if the config file is not writable\n
	        #Fove_ErrorCode_System_UnknownError if any other system error happened with the config file
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_setValue_bool(const char* key, bool value) FOVE_NOEXCEPT;

//! Set the value of the provided key to the FOVE config
/*!
	\param key The key name of the value to set, null-terminated and in UTF-8
	\param value The new value to set as the key value.
	\return #Fove_ErrorCode_None if the value was successfully set\n
	        #Fove_ErrorCode_API_NullInPointer if key is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the provided key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not an int\n
	        #Fove_ErrorCode_System_AccessDenied if the config file is not writable\n
	        #Fove_ErrorCode_System_UnknownError if any other system error happened with the config file
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_setValue_int(const char* key, int value) FOVE_NOEXCEPT;

//! Set the value of the provided key to the FOVE config
/*!
	\param key The key name of the value to set, null-terminated and in UTF-8
	\param value The new value to set as the key value.
	\return #Fove_ErrorCode_None if the value was successfully set\n
	        #Fove_ErrorCode_API_NullInPointer if key is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the provided key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not a float\n
	        #Fove_ErrorCode_System_AccessDenied if the config file is not writable\n
	        #Fove_ErrorCode_System_UnknownError if any other system error happened with the config file
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_setValue_float(const char* key, float value) FOVE_NOEXCEPT;

//! Set the value of the provided key to the FOVE config
/*!
	\param key The key name of the value to set, null-terminated and in UTF-8
	\param value The new value to set as the key value.
	\return #Fove_ErrorCode_None if the value was successfully set\n
	        #Fove_ErrorCode_API_NullInPointer if key or value is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the provided key doesn't exist\n
	        #Fove_ErrorCode_Config_TypeMismatch if the key exists but its value type is not a string\n
	        #Fove_ErrorCode_System_AccessDenied if the config file is not writable\n
	        #Fove_ErrorCode_System_UnknownError if any other system error happened with the config file
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_setValue_string(const char* key, const char* value) FOVE_NOEXCEPT;

//! Reset the value of the provided key to its default value
/*!
	\param key The key name of the value to reset, null-terminated and in UTF-8
	\return #Fove_ErrorCode_None if the value was successfully reset\n
	        #Fove_ErrorCode_API_NullInPointer if key is null\n
	        #Fove_ErrorCode_Config_DoesntExist if the provided key doesn't exist\n
	        #Fove_ErrorCode_System_AccessDenied if the config file is not writable\n
	        #Fove_ErrorCode_System_UnknownError if any other system error happened with the config file
*/
FOVE_EXPORT Fove_ErrorCode fove_Config_clearValue(const char* key) FOVE_NOEXCEPT;

/////////////////////////////////////////////////////////////////////////////////
// Fove C++ API -----------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////

// This API is header only so C++ ABI compatibility is not an issue
// The only external references used are via the C API

#if FOVE_DEFINE_CXX_API

// Helpers for bit-joining capabilities
/// @cond Capabilities_Functions
inline constexpr Fove_ClientCapabilities operator|(Fove_ClientCapabilities a, Fove_ClientCapabilities b) { return static_cast<Fove_ClientCapabilities>(static_cast<int>(a) | static_cast<int>(b)); }
inline constexpr Fove_ClientCapabilities operator&(Fove_ClientCapabilities a, Fove_ClientCapabilities b) { return static_cast<Fove_ClientCapabilities>(static_cast<int>(a) & static_cast<int>(b)); }
inline constexpr Fove_ClientCapabilities operator~(Fove_ClientCapabilities a) { return static_cast<Fove_ClientCapabilities>(~static_cast<int>(a)); }
inline constexpr Fove_ObjectGroup operator|(Fove_ObjectGroup a, Fove_ObjectGroup b) { return static_cast<Fove_ObjectGroup>(static_cast<int>(a) | static_cast<int>(b)); }
/// @endcond

namespace FOVE_CXX_NAMESPACE
{

//! Class to hold two copies of something, one for each left and right respectively
/*!
	This is similar to std::pair but more explicitly meant for stereo items.
*/
template <typename Type>
struct Stereo
{
	Type l{}; //!< Left side
	Type r{}; //!< Right side
};

//! Struct Contains hardware information for the headset
/*!
	C++ version of the C API's Fove_HeadsetHardwareInfo
*/
struct HeadsetHardwareInfo
{
	std::string serialNumber; //!< Serial number in UTF-8
	std::string manufacturer; //!< Manufacturer info in UTF-8
	std::string modelName;    //!< Model name in UTF-8
};

//! Exception type that is thrown when an error is ignored in the FOVE API
struct Exception : public std::exception
{
public:
	ErrorCode error = ErrorCode::None;

	Exception(const ErrorCode e) : error{e} {}

	const char* what() const noexcept override
	{
		return "Fove API Exception"; // Todo: base this on the error
	}
};

//! Helper function to determine which errors can coexist with values
inline bool isValid(const ErrorCode err) { return err == ErrorCode::None || err == ErrorCode::Data_LowAccuracy; }

//! Class for return values from the C++ API
/*!
	This class hold an error code, and possible a value if there was not a severe error.
	Usually there is an error OR a value, but some errors (eg Data_LowAccuracy) coexist with a value.

	Whether or not there is a value is determined by isValid().
	If the value of an invalid result is requested, an exception is thrown.
	If you do not want exceptions to be thrown, make sure to check the isValid() first.

	nullptr_t is a special type that indicates "no value", just an error field.
*/
template <typename Value = std::nullptr_t>
class Result
{
	ErrorCode m_err = ErrorCode::None;
	Value m_value{};

public:
	Result() = default;                                //!< Constructs a result with None a default-initialized value
	Result(const ErrorCode err) : m_err{err} {}        //!< Constructs a result with an error and a default-initialized value
	Result(const Value& data) : m_value{data} {}       //!< Constructs a result with None and a value
	Result(Value&& data) : m_value{std::move(data)} {} //!< Constructs a result with None and a value
	Result(const ErrorCode err, const Value& data) : m_err{err}, m_value{data} {}       //!< Constructs a result with an error and a value
	Result(const ErrorCode err, Value&& data) : m_err{err}, m_value{std::move(data)} {} //!< Constructs a result with an error and a value

	//! Returns the error code
	ErrorCode getError() const { return m_err; }
	ErrorCode& getErrorMutable() { return m_err; }

	//! Returns true if the returned data is valid
	bool isValid() const { return FOVE_CXX_NAMESPACE::isValid(m_err); }

	//! Returns true if the returned data is valid and considered reliable (i.e. of good accuracy)
	bool isReliable() const { return m_err == ErrorCode::None; }

	//! Returns the value if isValid() is true, otherwise throws
	Value& getValue() &
	{
		throwIfInvalid();
		return m_value;
	}

	//! Returns the value if isValid() is true, otherwise throws
	Value&& getValue() &&
	{
		throwIfInvalid();
		return std::move(m_value);
	}

	//! Returns the value if isValid() is true, otherwise throws
	const Value& getValue() const&
	{
		throwIfInvalid();
		return m_value;
	}

	//! Returns the value without checking the error field
	/*!
		It is expected that the error was checked manually before calling this field, for example via throwIfInvalid().
		The return value is undefined if isValid() is false.
	*/
	const Value& getValueUnchecked() const { return m_value; }
	Value& getValueUncheckedMutable() { return m_value; }

	//! Returns the value if available, otherwise returns the provided default
	/*!
		\param defValue A value to return if this object is not valid
	*/
	Value valueOr(Value defValue) const { return isValid() ? m_value : std::move(defValue); }

	//! Throws if there is an error, otherwise is a no-op
	/*!
		If exceptions are disabled, this will terminate the program instead of throwing.
	*/
	void throwIfInvalid() const
	{
		if (!isValid())
		{
#if FOVE_EXCEPTIONS
			throw Exception{m_err};
#else
			std::terminate();
#endif
		}
	}

	//! Explicit conversion to bool, for use in if statements
	explicit operator bool () const { return isValid(); }

	Value      & operator* ()       { return  getValue(); } //!< Pointer-like semantics to fetch value, throws if invalid
	Value const& operator* () const { return  getValue(); } //!< Pointer-like semantics to fetch value, throws if invalid
	Value      * operator->()       { return &getValue(); } //!< Pointer-like semantics to fetch value, throws if invalid
	Value const* operator->() const { return &getValue(); } //!< Pointer-like semantics to fetch value, throws if invalid

	//! Helper function to create an error by calling a C API function
	template <typename Call, typename... Args>
	static Result invoke(Call* call, const Args... args)
	{
		Value ret{};
		const ErrorCode err = (*call)(args..., &ret);
		return {err, ret};
	}

	//! Variant of invoke for functions that have a left and a right output
	template <typename Call, typename... Args>
	static Result invokeStereo(Call* call, const Args... args)
	{
		Value ret{};
		const ErrorCode err = (*call)(args..., &ret.l, &ret.r);
		return {err, ret};
	}

	//! Variant of invoke for functions that return a client-side-allocated array
	template <typename ItemType, typename Call, typename... Args>
	static Result invokeVector(Call* call, const Args... args)
	{
		auto helper = [](const ItemType item, void* const ptr) {
			Value& vec = *reinterpret_cast<Value*>(ptr);
			vec.emplace_back(item);
		};

		Value vec;
		const ErrorCode err = (*call)(args..., helper, &vec);
		return {err, vec};
	}

	//! Variant of invoke for functions that return a client-provided callback
	template <typename ItemType, typename Call, typename Callback, typename... Args>
	static Result<> invokeCallback(Call* call, Callback&& callback, const Args... args)
	{
		using CallbackPtr = decltype(&callback);
		auto thunk = [](const ItemType value, void* ptr) {
			Callback&& callback = std::move(*reinterpret_cast<CallbackPtr>(ptr));
			std::forward<Callback>(callback)(value);
		};

		const ErrorCode err = (*call)(args..., thunk, static_cast<CallbackPtr>(&callback));
		return err;
	}

	//! Variant of invoke for functions that get the value through a callback
	template <typename ItemType, typename Call, typename... Args>
	static Result invokeThroughCallback(Call* call, const Args... args)
	{
		Value ret{};
		const ErrorCode err = invokeCallback<ItemType>(call, [&](const Value& str) { ret = str; }, args...).getError();
		return {err, ret};
	}
};

//! Base class for classes in the FOVE C++ API
template <typename CType>
class Object
{
public:
	//! Returns the underlying C type which the caller can use to invoke the C API directly, or null if not valid
	CType* getCObject() const { return m_object; }

	//! Returns true if this object is non-empty
	/*! An object may be empty if it's contained data was moved to another variable. */
	bool isValid() const { return m_object != nullptr; }

protected:
	CType* m_object = nullptr;

	Object() = default;
	Object(CType& obj) : m_object{&obj} {}
	Object(Object&& o) : m_object{o.m_object} { o.m_object = nullptr; }

	//! Destructor is protected to avoid needing to be virtual
	~Object() = default;
};

//! Compositor API
/*!
	This class is a wrapper around the C API's Fove_Compositor.

	It is the main way to draw content to a headset.
*/
class Compositor : public Object<Fove_Compositor>
{
public:
	//! Creates an empty compositor
	/*!
		Please use Headset::createCompositor() to get a valid compositor
		\see Headset::createCompositor()
	*/
	Compositor() = default;

	//! Creates a compositor from an existing C API object
	/*!
		This is not normally invoked directly, rather Headset::createCompositor(), which wraps this, is typically used.
		\see Headset::createCompositor()
	*/
	Compositor(Fove_Compositor& compositor) : Object{compositor} {}

	//! Move constructs a compositor
	/*!
		\param other May be empty or non-empty. By return, it will be empty.
	*/
	Compositor(Compositor&& other) : Object{static_cast<Compositor&&>(static_cast<Object&>(other))} {}

	//! Destroys the existing compositor if any, then moves the one referenced by \p other, if any, into this object
	/*!
		\param other May be empty or non-empty. By return, it will be empty.
	*/
	Compositor& operator=(Compositor&& other)
	{
		const Result<> err = destroy();
		if (!err)
			fove_logText(LogLevel::Error, "fove_Compositor_destroy failed");

		m_object = other.m_object;
		other.m_object = nullptr;
		return *this;
	}

	//! Destroys the compositor, freeing any resources used (including all layers)
	/*! Since an error cannot be returned, any error from fove_Compositor_destroy will be logged. */
	~Compositor()
	{
		if (!destroy())
			fove_logText(LogLevel::Error, "fove_Compositor_destroy failed");
	}

	//! Destroys the compositor object, releasing resources
	/*!
		After this call, this object will be in an empty state and future calls will fail.
		This is handled by the destructor, usually the user doesn't need to call this.
	*/
	Result<> destroy()
	{
		Fove_Compositor* const object = m_object;
		m_object = nullptr;
		return object ? fove_Compositor_destroy(object) : ErrorCode::None;
	}

	//! Wraps fove_Compositor_createLayer()
	Result<CompositorLayer> createLayer(const CompositorLayerCreateInfo& layerInfo)
	{
		return Result<CompositorLayer>::invoke(&fove_Compositor_createLayer, m_object, &layerInfo);
	}

	//! Wraps fove_Compositor_submit()
	Result<> submit(const CompositorLayerSubmitInfo* submitInfo, const size_t layerCount)
	{
		return fove_Compositor_submit(m_object, submitInfo, layerCount);
	}

	//! Alternate version of submit() that simply takes one layer
	Result<> submit(const CompositorLayerSubmitInfo& submitInfo)
	{
		return submit(&submitInfo, 1);
	}

	//! Wraps fove_Compositor_waitForRenderPose()
	Result<Pose> waitForRenderPose()
	{
		return Result<Pose>::invoke(&fove_Compositor_waitForRenderPose, m_object);
	}

	//! Wraps fove_Compositor_getLastRenderPose()
	Result<Pose> getLastRenderPose()
	{
		return Result<Pose>::invoke(&fove_Compositor_getLastRenderPose, m_object);
	}

	//! Wraps fove_Compositor_isReady()
	Result<bool> isReady()
	{
		return Result<bool>::invoke(&fove_Compositor_isReady, m_object);
	}

	//! Wraps fove_Compositor_queryAdapterId()
	/*!
		\param ignore This is not used and will be removed in a future version
	*/
	Result<AdapterId> queryAdapterId(AdapterId* ignore = nullptr)
	{
		return Result<AdapterId>::invoke(&fove_Compositor_queryAdapterId, m_object);
	}
};

//! Main API for using headsets
/*!
	This class is a wrapper around the C API's Fove_Headset, and is the main class of the FOVE API.
*/
class Headset : public Object<Fove_Headset>
{
public:
	//! Creates an empty headset
	/*!
		Please use Headset::create() to create a valid headset.
		\see Headset::create
	*/
	Headset() = default;

	//! Creates a headset from an existing C API object
	/*!
		This is not normally invoked directly, rather Headset::create(), which wraps this, is typically used.
		\see Headset::create
	*/
	Headset(Fove_Headset& headset) : Object{headset} {}

	//! Move constructs a headset
	/*!
		\param other May be empty or non-empty. By return, it will be empty.
	*/
	Headset(Headset&& other) : Object{static_cast<Object&&>(other)} {}

	//! Destroys the existing headset if any, then moves the one referenced by \p other, if any, into this object
	/*!
		\param other May be empty or non-empty. By return, it will be empty.
	*/
	Headset& operator=(Headset&& other)
	{
		const Result<> err = destroy();
		if (!err)
			fove_logText(LogLevel::Error, "fove_Headset_destroy failed");

		m_object = other.m_object;
		other.m_object = nullptr;
		return *this;
	}

	//! Creates a new headset object with the given capabilities
	static Result<Headset> create(const ClientCapabilities capabilities)
	{
		const Result<Fove_Headset*> ret = Result<Fove_Headset*>::invoke(fove_createHeadset, capabilities);
		if (!ret)
			return {ret.getError()};

		return {Headset{*ret.getValueUnchecked()}};
	}

	//! Destroys the headset, releasing any resources
	/*! Since an error cannot be returned, any error from fove_Headset_destroy will be logged. */
	~Headset()
	{
		if (!destroy())
			fove_logText(LogLevel::Error, "fove_Headset_destroy failed");
	}

	//! Destroys the headset, releasing resources
	/*!
		After this call, this object will be in an empty state and future calls will fail.
		This is handled by the destructor, usually the user doesn't need to call this.
	*/
	Result<> destroy()
	{
		Fove_Headset* const object = m_object;
		m_object = nullptr;
		return object ? fove_Headset_destroy(object) : ErrorCode::None;
	}

	//! Wraps fove_Headset_isHardwareConnected()
	Result<bool> isHardwareConnected()
	{
		return Result<bool>::invoke(fove_Headset_isHardwareConnected, m_object);
	}

	//! Wraps fove_Headset_isMotionReady()
	Result<bool> isMotionReady()
	{
		return Result<bool>::invoke(fove_Headset_isMotionReady, m_object);
	}

	//! Wraps fove_Headset_checkSoftwareVersions()
	Result<> checkSoftwareVersions()
	{
		return fove_Headset_checkSoftwareVersions(m_object);
	}

	//! Wraps fove_Headset_querySoftwareVersions()
	Result<Versions> querySoftwareVersions()
	{
		return Result<Versions>::invoke(fove_Headset_querySoftwareVersions, m_object);
	}

	//! Wraps fove_Headset_queryLicenses()
	Result<std::vector<LicenseInfo>> queryLicenses()
	{
		size_t numLicenses = 0;
		ErrorCode err = fove_Headset_queryLicenses(m_object, nullptr, &numLicenses);
		if (err != ErrorCode::None)
			return err;

		std::vector<LicenseInfo> licenses;
		if (numLicenses > 0)
		{
			licenses.resize(numLicenses);
			err = fove_Headset_queryLicenses(m_object, &licenses[0], &numLicenses);
			if (err != ErrorCode::None)
				return err;

			licenses.resize(numLicenses); // There's a small possibility the size changed between the two calls to queryLicenses
		}

		return licenses;
	}

	//! Wraps fove_Headset_queryHardwareInfo()
	Result<HeadsetHardwareInfo> queryHeadsetHardwareInfo()
	{
		const Result<Fove_HeadsetHardwareInfo> cRet = Result<Fove_HeadsetHardwareInfo>::invoke(fove_Headset_queryHardwareInfo, m_object);
		if (!cRet.isValid())
			return cRet.getError();

		// Convert to CXX version of struct (std::string instead of C string)
		HeadsetHardwareInfo cxxRet;
		cxxRet.manufacturer = FOVE_STRUCT_ARRAY_CPTR(cRet.getValueUnchecked().manufacturer);
		cxxRet.modelName = FOVE_STRUCT_ARRAY_CPTR(cRet.getValueUnchecked().modelName);
		cxxRet.serialNumber = FOVE_STRUCT_ARRAY_CPTR(cRet.getValueUnchecked().serialNumber);
		return cxxRet;
	}

	//! Wraps fove_Headset_registerCapabilities()
	Result<> registerCapabilities(const ClientCapabilities caps)
	{
		return fove_Headset_registerCapabilities(m_object, caps);
	}

	//! Wraps fove_Headset_registerPassiveCapabilities()
	Result<> registerPassiveCapabilities(const ClientCapabilities caps)
	{
		return fove_Headset_registerPassiveCapabilities(m_object, caps);
	}

	//! Wraps fove_Headset_unregisterCapabilities()
	Result<> unregisterCapabilities(const ClientCapabilities caps)
	{
		return fove_Headset_unregisterCapabilities(m_object, caps);
	}

	//! Wraps fove_Headset_unregisterPassiveCapabilities()
	Result<> unregisterPassiveCapabilities(const ClientCapabilities caps)
	{
		return fove_Headset_unregisterPassiveCapabilities(m_object, caps);
	}

	//! Wraps fove_Headset_waitForProcessedEyeFrame()
	Result<> waitForProcessedEyeFrame()
	{
		return fove_Headset_waitForProcessedEyeFrame(m_object);
	}

	//! Wraps fove_Headset_fetchEyeTrackingData()
	Result<Fove_FrameTimestamp> fetchEyeTrackingData()
	{
		return Result<Fove_FrameTimestamp>::invoke(&fove_Headset_fetchEyeTrackingData, m_object);
	}

	//! Wraps fove_Headset_fetchEyesImage()
	Result<Fove_FrameTimestamp> fetchEyesImage()
	{
		return Result<Fove_FrameTimestamp>::invoke(&fove_Headset_fetchEyesImage, m_object);
	}

	//! Wraps fove_Headset_getEyeTrackingDataTimestamp()
	Result<Fove_FrameTimestamp> getEyeTrackingDataTimestamp()
	{
		return Result<Fove_FrameTimestamp>::invoke(&fove_Headset_getEyeTrackingDataTimestamp, m_object);
	}

	//! Wraps fove_Headset_getEyesImageTimestamp()
	Result<Fove_FrameTimestamp> getEyesImageTimestamp()
	{
		return Result<Fove_FrameTimestamp>::invoke(&fove_Headset_getEyesImageTimestamp, m_object);
	}

	//! Wraps fove_Headset_getGazeVector()
	Result<Vec3> getGazeVector(Eye eye)
	{
		return Result<Vec3>::invoke(fove_Headset_getGazeVector, m_object, eye);
	}

	//! Wraps fove_Headset_getGazeVectorRaw()
	Result<Vec3> getGazeVectorRaw(Eye eye)
	{
		return Result<Vec3>::invoke(fove_Headset_getGazeVectorRaw, m_object, eye);
	}

	//! Wraps fove_Headset_getGazeScreenPosition()
	Result<Vec2> getGazeScreenPosition(Eye eye)
	{
		return Result<Vec2>::invoke(fove_Headset_getGazeScreenPosition, m_object, eye);
	}

	//! Wraps fove_Headset_getGazeScreenPosition()
	Result<Vec2> getGazeScreenPositionCombined()
	{
		return Result<Vec2>::invoke(fove_Headset_getGazeScreenPositionCombined, m_object);
	}

	//! Wraps fove_Headset_getCombinedGazeRay()
	Result<Fove_Ray> getCombinedGazeRay()
	{
		return Result<Fove_Ray>::invoke(fove_Headset_getCombinedGazeRay, m_object);
	}

	//! Wraps fove_Headset_getCombinedGazeDepth()
	Result<float> getCombinedGazeDepth()
	{
		return Result<float>::invoke(fove_Headset_getCombinedGazeDepth, m_object);
	}

	//! Wraps fove_Headset_isUserShiftingAttention()
	Result<bool> isUserShiftingAttention()
	{
		return Result<bool>::invoke(fove_Headset_isUserShiftingAttention, m_object);
	}

	//! Wraps fove_Headset_getEyeState()
	Result<EyeState> getEyeState(Eye eye)
	{
		return Result<EyeState>::invoke(fove_Headset_getEyeState, m_object, eye);
	}

	//! Wraps fove_Headset_isEyeBlinking()
	Result<bool> isEyeBlinking(Eye eye)
	{
		return Result<bool>::invoke(fove_Headset_isEyeBlinking, m_object, eye);
	}

	//! Wraps fove_Headset_getEyeBlinkCount()
	Result<int> getEyeBlinkCount(Eye eye)
	{
		return Result<int>::invoke(fove_Headset_getEyeBlinkCount, m_object, eye);
	}

	//! Wraps fove_Headset_isEyeTrackingEnabled()
	Result<bool> isEyeTrackingEnabled()
	{
		return Result<bool>::invoke(fove_Headset_isEyeTrackingEnabled, m_object);
	}

	//! Wraps fove_Headset_isEyeTrackingCalibrated()
	Result<bool> isEyeTrackingCalibrated()
	{
		return Result<bool>::invoke(fove_Headset_isEyeTrackingCalibrated, m_object);
	}

	//! Wraps fove_Headset_isEyeTrackingCalibrating()
	Result<bool> isEyeTrackingCalibrating()
	{
		return Result<bool>::invoke(fove_Headset_isEyeTrackingCalibrating, m_object);
	}

	//! Wraps fove_Headset_isEyeTrackingCalibratedForGlasses()
	Result<bool> isEyeTrackingCalibratedForGlasses()
	{
		return Result<bool>::invoke(fove_Headset_isEyeTrackingCalibratedForGlasses, m_object);
	}

	//! Wraps fove_Headset_isHmdAdjustmentGuiVisible()
	Result<bool> isHmdAdjustmentGuiVisible()
	{
		return Result<bool>::invoke(fove_Headset_isHmdAdjustmentGuiVisible, m_object);
	}

	//! Wraps fove_Headset_hasHmdAdjustmentGuiTimeout()
	Result<bool> hasHmdAdjustmentGuiTimeout()
	{
		return Result<bool>::invoke(fove_Headset_hasHmdAdjustmentGuiTimeout, m_object);
	}

	//! Wraps fove_Headset_isEyeTrackingReady()
	Result<bool> isEyeTrackingReady()
	{
		return Result<bool>::invoke(fove_Headset_isEyeTrackingReady, m_object);
	}

	//! Wraps fove_Headset_isUserPresent()
	Result<bool> isUserPresent()
	{
		return Result<bool>::invoke(fove_Headset_isUserPresent, m_object);
	}

	//! Wraps fove_Headset_getEyesImage()
	Result<Fove_BitmapImage> getEyesImage()
	{
		return Result<Fove_BitmapImage>::invoke(&fove_Headset_getEyesImage, m_object);
	}

	//! Wraps fove_Headset_getUserIPD()
	Result<float> getUserIPD()
	{
		return Result<float>::invoke(fove_Headset_getUserIPD, m_object);
	}

	//! Wraps fove_Headset_getUserIOD()
	Result<float> getUserIOD()
	{
		return Result<float>::invoke(fove_Headset_getUserIOD, m_object);
	}

	//! Wraps fove_Headset_getPupilRadius()
	Result<float> getPupilRadius(Eye eye)
	{
		return Result<float>::invoke(fove_Headset_getPupilRadius, m_object, eye);
	}

	//! Wraps fove_Headset_getIrisRadius()
	Result<float> getIrisRadius(Eye eye)
	{
		return Result<float>::invoke(fove_Headset_getIrisRadius, m_object, eye);
	}

	//! Wraps fove_Headset_getEyeballRadius()
	Result<float> getEyeballRadius(Eye eye)
	{
		return Result<float>::invoke(fove_Headset_getEyeballRadius, m_object, eye);
	}

	//! Wraps fove_Headset_getEyeTorsion()
	Result<float> getEyeTorsion(Eye eye)
	{
		return Result<float>::invoke(fove_Headset_getEyeTorsion, m_object, eye);
	}

	//! Wraps fove_Headset_getEyeShape()
	Result<EyeShape> getEyeShape(Eye eye)
	{
		return Result<EyeShape>::invoke(&fove_Headset_getEyeShape, m_object, eye);
	}

	//! Wraps fove_Headset_getPupilShape()
	Result<PupilShape> getPupilShape(Eye eye)
	{
		return Result<PupilShape>::invoke(&fove_Headset_getPupilShape, m_object, eye);
	}

	//! Wraps fove_Headset_startHmdAdjustmentProcess()
	Result<> startHmdAdjustmentProcess(bool lazy = true)
	{
		return fove_Headset_startHmdAdjustmentProcess(m_object, lazy);
	}

	//! Wraps fove_Headset_tickHmdAdjustmentProcess()
	Result<> tickHmdAdjustmentProcess(float deltaTime, bool isVisible, Fove_HmdAdjustmentData& outData)
	{
		return fove_Headset_tickHmdAdjustmentProcess(m_object, deltaTime, isVisible, &outData);
	}

	//! Wraps fove_Headset_startEyeTrackingCalibration()
	Result<> startEyeTrackingCalibration(const Fove_CalibrationOptions& options = {})
	{
		return fove_Headset_startEyeTrackingCalibration(m_object, &options);
	}

	//! Wraps fove_Headset_stopEyeTrackingCalibration()
	Result<> stopEyeTrackingCalibration()
	{
		return fove_Headset_stopEyeTrackingCalibration(m_object);
	}

	//! Wraps fove_Headset_getEyeTrackingCalibrationState()
	Result<Fove_CalibrationState> getEyeTrackingCalibrationState()
	{
		return Result<Fove_CalibrationState>::invoke(&fove_Headset_getEyeTrackingCalibrationState, m_object);
	}

	//! Wraps fove_Headset_getEyeTrackingCalibrationStateDetails()
	template <typename Callback>
	Result<> getEyeTrackingCalibrationStateDetails(Callback&& callback)
	{
		return Result<>::invokeCallback<Fove_CalibrationData*>(&fove_Headset_getEyeTrackingCalibrationStateDetails, std::forward<Callback>(callback), m_object);
	}

	//! Wraps fove_Headset_tickEyeTrackingCalibration()
	template <typename Callback>
	Result<> tickEyeTrackingCalibration(const float dt, bool isVisible, Callback&& callback)
	{
		return Result<>::invokeCallback<Fove_CalibrationData*>(&fove_Headset_tickEyeTrackingCalibration, std::forward<Callback>(callback), m_object, dt, isVisible);
	}

	//! Wraps fove_Headset_getGazedObjectId()
	Result<int> getGazedObjectId()
	{
		return Result<int>::invoke(fove_Headset_getGazedObjectId, m_object);
	}

	//! Wraps fove_Headset_registerGazableObject()
	Result<> registerGazableObject(const Fove_GazableObject& object)
	{
		return fove_Headset_registerGazableObject(m_object, &object);
	}

	//! Wraps fove_Headset_updateGazableObject()
	Result<> updateGazableObject(const int objectId, const Fove_ObjectPose& objectPose)
	{
		return fove_Headset_updateGazableObject(m_object, objectId, &objectPose);
	}

	//! Wraps fove_Headset_removeGazableObject()
	Result<> removeGazableObject(const int objectId)
	{
		return fove_Headset_removeGazableObject(m_object, objectId);
	}

	//! Wraps fove_Headset_registerCameraObject()
	Result<> registerCameraObject(const Fove_CameraObject& camera)
	{
		return fove_Headset_registerCameraObject(m_object, &camera);
	}

	//! Wraps fove_Headset_updateCameraObject()
	Result<> updateCameraObject(const int cameraId, const Fove_ObjectPose& pose)
	{
		return fove_Headset_updateCameraObject(m_object, cameraId, &pose);
	}

	//! Wraps fove_Headset_removeCameraObject()
	Result<> removeCameraObject(const int cameraId)
	{
		return fove_Headset_removeCameraObject(m_object, cameraId);
	}

	//! Wraps fove_Headset_tareOrientationSensor()
	Result<> tareOrientationSensor()
	{
		return fove_Headset_tareOrientationSensor(m_object);
	}

	//! Wraps fove_Headset_isPositionReady()
	Result<bool> isPositionReady()
	{
		return Result<bool>::invoke(fove_Headset_isPositionReady, m_object);
	}

	//! Wraps fove_Headset_tarePositionSensors()
	Result<> tarePositionSensors()
	{
		return fove_Headset_tarePositionSensors(m_object);
	}

	//! Wraps fove_Headset_fetchPoseData()
	Result<Fove_FrameTimestamp> fetchPoseData()
	{
		return Result<Fove_FrameTimestamp>::invoke(fove_Headset_fetchPoseData, m_object);
	}

	//! Wraps fove_Headset_fetchPositionImage()
	Result<Fove_FrameTimestamp> fetchPositionImage()
	{
		return Result<Fove_FrameTimestamp>::invoke(fove_Headset_fetchPositionImage, m_object);
	}

	//! Wraps fove_Headset_getPoseDataTimestamp()
	Result<Fove_FrameTimestamp> getPoseDataTimestamp()
	{
		return Result<Fove_FrameTimestamp>::invoke(fove_Headset_getPoseDataTimestamp, m_object);
	}

	//! Wraps fove_Headset_getPositionImageTimestamp()
	Result<Fove_FrameTimestamp> getPositionImageTimestamp()
	{
		return Result<Fove_FrameTimestamp>::invoke(fove_Headset_getPositionImageTimestamp, m_object);
	}

	//! Wraps fove_Headset_getPose()
	Result<Pose> getPose()
	{
		return Result<Pose>::invoke(fove_Headset_getPose, m_object);
	}

	//! Wraps fove_Headset_getPositionImage()
	Result<Fove_BitmapImage> getPositionImage()
	{
		return Result<Fove_BitmapImage>::invoke(fove_Headset_getPositionImage, m_object);
	}

	//! Wraps fove_Headset_getProjectionMatricesLH()
	Result<Stereo<Matrix44>> getProjectionMatricesLH(const float zNear, const float zFar)
	{
		return Result<Stereo<Matrix44>>::invokeStereo(fove_Headset_getProjectionMatricesLH, m_object, zNear, zFar);
	}

	//! Wraps fove_Headset_getProjectionMatricesRH()
	Result<Stereo<Matrix44>> getProjectionMatricesRH(const float zNear, const float zFar)
	{
		return Result<Stereo<Matrix44>>::invokeStereo(fove_Headset_getProjectionMatricesRH, m_object, zNear, zFar);
	}

	//! Wraps fove_Headset_getRawProjectionValues()
	Result<Stereo<ProjectionParams>> getRawProjectionValues()
	{
		return Result<Stereo<ProjectionParams>>::invokeStereo(fove_Headset_getRawProjectionValues, m_object);
	}

	//! Wraps fove_Headset_getEyeToHeadMatrices()
	Result<Stereo<Matrix44>> getEyeToHeadMatrices()
	{
		return Result<Stereo<Matrix44>>::invokeStereo(&fove_Headset_getEyeToHeadMatrices, m_object);
	}

	//! Wraps fove_Headset_getIOD()
	Result<float> getRenderIOD()
	{
		return Result<float>::invoke(&fove_Headset_getRenderIOD, m_object);
	}

	//! Wraps fove_Headset_createProfile()
	Result<> createProfile(const std::string& name)
	{
		return fove_Headset_createProfile(m_object, name.c_str());
	}

	//! Wraps fove_Headset_renameProfile()
	Result<> renameProfile(const std::string& oldName, const std::string& newName)
	{
		return fove_Headset_renameProfile(m_object, oldName.c_str(), newName.c_str());
	}

	//! Wraps fove_Headset_deleteProfile()
	Result<> deleteProfile(const std::string& profileName)
	{
		return fove_Headset_deleteProfile(m_object, profileName.c_str());
	}

	//! Wraps fove_Headset_listProfiles()
	template <typename Callback>
	Result<> listProfiles(Callback&& callback)
	{
		return Result<>::invokeCallback<const char*>(&fove_Headset_listProfiles, std::forward<Callback>(callback), m_object);
	}

	//! Wraps fove_Headset_listProfiles()
	Result<std::vector<std::string>> listProfiles()
	{
		return Result<std::vector<std::string>>::invokeVector<const char*>(&fove_Headset_listProfiles, m_object);
	}

	//! Wraps fove_Headset_setCurrentProfile()
	Result<> setCurrentProfile(const std::string& profileName)
	{
		return fove_Headset_setCurrentProfile(m_object, profileName.c_str());
	}

	//! Wraps fove_Headset_queryCurrentProfile()
	template <typename Callback>
	Result<> queryCurrentProfile(Callback&& callback)
	{
		return Result<>::invokeCallback<const char*>(&fove_Headset_queryCurrentProfile, std::forward<Callback>(callback), m_object);
	}

	//! Wraps fove_Headset_queryCurrentProfile()
	Result<std::string> queryCurrentProfile()
	{
		return Result<std::string>::invokeThroughCallback<const char*>(&fove_Headset_queryCurrentProfile, m_object);
	}

	//! Wraps fove_Headset_queryProfileDataPath()
	template <typename Callback>
	Result<> queryProfileDataPath(const std::string& profileName, Callback&& callback)
	{
		return Result<>::invokeCallback<const char*>(&fove_Headset_queryProfileDataPath, std::forward<Callback>(callback), m_object, profileName.c_str());
	}

	//! Wraps fove_Headset_queryProfileDataPath()
	Result<std::string> queryProfileDataPath(const std::string& profileName)
	{
		return Result<std::string>::invokeThroughCallback<const char*>(&fove_Headset_queryProfileDataPath, m_object, profileName.c_str());
	}

	//! Wraps fove_Headset_hasAccessToFeature()
	Result<bool> hasAccessToFeature(const std::string& inFeatureName)
	{
		return Result<bool>::invoke(&fove_Headset_hasAccessToFeature, m_object, inFeatureName.c_str());
	}

	//! Wraps fove_Headset_activateLincese()
	Result<> activateLicense(const std::string& licenseKey)
	{
		return fove_Headset_activateLicense(m_object, licenseKey.c_str());
	}

	//! Wraps fove_Headset_deactivateLincese()
	Result<> deactivateLicense(const std::string& licenseKey)
	{
		return fove_Headset_deactivateLicense(m_object, licenseKey.c_str());
	}

	//! Creates a new compositor object
	Result<Compositor> createCompositor()
	{
		const Result<Fove_Compositor*> ret = Result<Fove_Compositor*>::invoke(fove_Headset_createCompositor, m_object);
		if (!ret)
			return {ret.getError()};

		return {Compositor{*ret.getValueUnchecked()}};
	}
};

// Wrapper for the fove_Config APIs
namespace Details
{

template <typename T>
struct ConfigHelperStruct
{
	static_assert(sizeof(T) == sizeof(T) + 1, "You invoked setConfigValue or getConfigValue with a type other than bool/float/int/string");
};

template <>
struct ConfigHelperStruct<bool>
{
	constexpr static auto set = &fove_Config_setValue_bool;
	constexpr static auto get = &fove_Config_getValue_bool;
};

template <>
struct ConfigHelperStruct<int>
{
	constexpr static auto set = &fove_Config_setValue_int;
	constexpr static auto get = &fove_Config_getValue_int;
};

template <>
struct ConfigHelperStruct<float>
{
	constexpr static auto set = &fove_Config_setValue_float;
	constexpr static auto get = &fove_Config_getValue_float;
};

} // namespace Details

template <typename T>
Result<T> getConfigValue(const std::string& key)
{
	return Result<T>::invoke(Details::ConfigHelperStruct<T>::get, key.c_str());
}

template <>
inline Result<std::string> getConfigValue(const std::string& key)
{
	return Result<std::string>::invokeThroughCallback<const char*>(&fove_Config_getValue_string, key.c_str());
}

template <typename T>
Result<> setConfigValue(const std::string& key, const T& value)
{
	return Details::ConfigHelperStruct<T>::set(key.c_str(), value);
}

template <>
inline Result<> setConfigValue(const std::string& key, const std::string& value)
{
	return fove_Config_setValue_string(key.c_str(), value.c_str());
}

inline Result<> clearConfigValue(const std::string& key)
{
	return fove_Config_clearValue(key.c_str());
}

//! Wraps fove_logText()
inline Result<> logText(const LogLevel level, const std::string& utf8Text)
{
	return fove_logText(level, utf8Text.c_str());
}

} // namespace FOVE_CXX_NAMESPACE

#endif // FOVE_DEFINE_CXX_API

// clang-format on

#endif // FOVE_API_H
