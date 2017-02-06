#pragma once

#ifndef _FOVETYPES_H
#define _FOVETYPES_H

#ifdef __GNUC__
#define FVR_DEPRECATED(func) func __attribute__ ((deprecated))
#define FVR_EXPORT __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define FVR_DEPRECATED(func) __declspec(deprecated) func
#define FVR_EXPORT __declspec(dllexport)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define FVR_DEPRECATED(func) func
#define FVR_EXPORT
#endif

#include <cmath>
#include <cstdint>
#include <cstring>

namespace Fove
{
	//! EFVR_ClientCapabilities
	/*! To be passed to the initialisation function of the client library to  */
	enum class EFVR_ClientCapabilities
	{
		Gaze = 0x01,
		Orientation = 0x02,
		Position = 0x04
	};

	inline EFVR_ClientCapabilities operator|(EFVR_ClientCapabilities a, EFVR_ClientCapabilities b)
	{
		return static_cast<EFVR_ClientCapabilities>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline EFVR_ClientCapabilities operator&(EFVR_ClientCapabilities a, EFVR_ClientCapabilities b)
	{
		return static_cast<EFVR_ClientCapabilities>(static_cast<int>(a) & static_cast<int>(b));
	}

	//! EFVR_ErrorCode enum
	/*! An enum of error codes that the system may return from GetLastError(). These will eventually be mapped to localised strings. */
	enum class EFVR_ErrorCode
	{
		None = 0,

		//! Connection Errors
		Connection_General = 1,
		Connect_NotConnected = 7,
		Connect_ServerUnreachable = 2,
		Connect_RegisterFailed = 3,
		Connect_DeregisterFailed = 6,
		Connect_RuntimeVersionTooOld = 4,
		Connect_HeartbeatNoReply = 5,
		Connect_ClientVersionTooOld = 6,

		//! Data Errors
		Data_General = 1000,
		Data_RegisteredWrongVersion = 1001,
		Data_UnreadableNotFound = 1002,
		Data_NoUpdate = 1003,
		Data_Uncalibrated = 1004,

		//! Hardware Errors
		Hardware_General = 2000,
		Hardware_CoreFault = 2001,
		Hardware_CameraFault = 2002,
		Hardware_IMUFault = 2003,
		Hardware_ScreenFault = 2004,
		Hardware_SecurityFault = 2005,
		Hardware_Disconnected = 2006,
		Hardware_WrongFirmwareVersion = 2007,

		//! Server Response Errors
		Server_General = 3000,
		Server_HardwareInterfaceInvalid = 3001,
		Server_HeartbeatNotRegistered = 3002,
		Server_DataCreationError = 3003,
		Server_ModuleError_ET = 3004,

		//! Code and placeholders
		Code_NotImplementedYet = 4000,
		Code_FunctionDeprecated = 4001,

		//! Position Tracking
		Position_NoObjectsInView = 5000,
		Position_NoDlibRegressor = 5001,
		Position_NoCascadeClassifier = 5002,
		Position_NoModel = 5003,
		Position_NoImages = 5004,
		Position_InvalidFile = 5005,
		Position_NoCamParaSet = 5006,
		Position_CantUpdateOptical = 5007,
		Position_ObjectNotTracked = 5008,
		
		//! Eye Tracking
		Eye_Left_NoDlibRegressor = 6000,
		Eye_Right_NoDlibRegressor = 6001,
		Eye_CalibrationFailed = 6002,
		Eye_LoadCalibrationFailed = 6003
	};

	//! EFVR_ClientType
	/*! Corresponds to the order in which clients are composited (Base, then Overlay, then Diagnostic) */
	enum class EFVR_ClientType
	{
		Base = 0,
		Overlay = 0x10000,
		Diagnostic = 0x20000
	};

	//! SFVR_Version
	/*! Contains the version for the software (used for runtime and client versions */
	struct SFVR_Versions
	{
		int clientMajor = 0;
		int clientMinor = 0;
		int clientBuild = 0;
		int clientProtocol = 0;
		int runtimeMajor = 0;
		int runtimeMinor = 0;
		int runtimeBuild = 0;
		int firmware = 0;
		int maxFirmware = 0;
		int minFirmware = 0;
		bool tooOldHeadsetConnected = false;
	};

	//! SFVR_Quaternion struct
	/*! A quaternion represents an orientation in 3d space.*/
	struct SFVR_Quaternion
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float w = 1;

		SFVR_Quaternion() = default;
		SFVR_Quaternion(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}

		//! Generate and return a conjugate of this quaternion
		SFVR_Quaternion Conjugate() const
		{
			return SFVR_Quaternion(-x, -y, -z, w);
		}

		SFVR_Quaternion Normalize() const
		{
			float d = std::sqrt(w*w + x*x + y*y + z*z);
			SFVR_Quaternion result(x / d, y / d, z / d, w / d);
			return result;
		}

		//! Return the result of multiplying this quaternion Q1 by another Q2 such that OUT = Q1 * Q2
		SFVR_Quaternion MultiplyBefore(const SFVR_Quaternion &second) const
		{
			auto nx =  x * second.w + y * second.z - z * second.y + w * second.x;
			auto ny = -x * second.z + y * second.w + z * second.x + w * second.y;
			auto nz =  x * second.y - y * second.x + z * second.w + w * second.z;
			auto nw = -x * second.x - y * second.y - z * second.z + w * second.w;
			return SFVR_Quaternion(nx, ny, nz, nw);
		}

		//! Return the result of multiplying this quaternion Q2 by another Q1 such that OUT = Q1 * Q2
		SFVR_Quaternion MultiplyAfter(const SFVR_Quaternion &first) const
		{
			auto nx =  first.x * w + first.y * z - first.z * y + first.w * x;
			auto ny = -first.x * z + first.y * w + first.z * x + first.w * y;
			auto nz =  first.x * y - first.y * x + first.z * w + first.w * z;
			auto nw = -first.x * x - first.y * y - first.z * z + first.w * w;
			return SFVR_Quaternion(nx, ny, nz, nw);
		}
	};

	//! SFVR_HeadOrientation struct
	/*! Represents the orientation of the Fove headset in 3d space along with some meta-information about how old this data is. */
	struct SFVR_HeadOrientation
	{
		//! Error Code
		/*! error:	if true => the rest of the data is in an unknown state. */
		EFVR_ErrorCode error = EFVR_ErrorCode::None;
		//! ID
		/*! Incremental counter which tells if the coord captured is a fresh value at a given frame */
		std::uint64_t id = 0;
		//! Timestamp
		/*! The time at which the coord was captured, based on system time */
		std::uint64_t timestamp = 0;
		//! Quaternion
		/*! The Quaternion which represents the orientation of the head. */
		SFVR_Quaternion quat;
	};

	//! SFVR_Vec3 struct
	/*! A vector that represents an position in 3d space. */
	struct SFVR_Vec3
	{
		float x = 0;
		float y = 0;
		float z = 0;

		SFVR_Vec3() = default;
		SFVR_Vec3(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
	};

	//! SFVR_Vec2 struct
	/*! A vector that represents an position in 2d space. Usually used when refering to screen or image coordinates. */
	struct SFVR_Vec2
	{
		float x = 0;
		float y = 0;

		SFVR_Vec2() = default;
		SFVR_Vec2(float ix, float iy) : x(ix), y(iy) {}
	};

	//! SFVR_Vec2 struct
	/*! A 2-component integer vector. */
	struct SFVR_Vec2i
	{
		int x = 0;
		int y = 0;

		SFVR_Vec2i() = default;
		SFVR_Vec2i(int ix, int iy) : x(ix), y(iy) {}
	};

	struct SFVR_Ray
	{
		SFVR_Vec3 origin = { 0, 0, 0 };
		SFVR_Vec3 direction = { 0, 0, 1 };

		SFVR_Ray() = default;
		SFVR_Ray(const SFVR_Vec3& _origin, const SFVR_Vec3& _direction) : origin(_origin), direction(_direction) {}
	};

	//! SFVR_Pose struct
	/*! This structure is a combination of the Fove headset position and orientation in 3d space, collectively known as the "pose".
		In the future this may also contain accelleration information for the headset, and may also be used for controllers.
	*/
	struct SFVR_Pose
	{
		//! Error Code
		/*! error:	if true => the rest of the data is in an unknown state. */
		EFVR_ErrorCode error = EFVR_ErrorCode::None;
		//! ID
		/*! Incremental counter which tells if the coord captured is a fresh value at a given frame */
		std::uint64_t id = 0;
		//! Timestamp
		/*! The time at which the coord was captured, based on system time */
		std::uint64_t timestamp = 0;
		//! Quaternion
		/*! The Quaternion which represents the orientation of the head. */
		SFVR_Quaternion orientation = { 0, 0, 0, 1 };
		//! Vector3
		/*! The angular velocity of the head. */
		SFVR_Vec3 angularVelocity = { 0, 0, 0 };
		//! Vector3
		/*! The angular acceleration of the head. */
		SFVR_Vec3 angularAcceleration = { 0, 0, 0 };
		//! Vector3
		/*! The position of headset in 3D space */
		SFVR_Vec3 position = { 0, 0, 0 };
		//! Vector3
		/*! The velocity of headset in 3D space */
		SFVR_Vec3 velocity = { 0, 0, 0 };
		//! Vector3
		/*! The acceleration of headset in 3D space */
		SFVR_Vec3 acceleration = { 0, 0, 0 };
	};

	//! SFVR_GazeVector struct
	/*! 
	*/
	struct SFVR_GazeVector
	{
		EFVR_ErrorCode error = EFVR_ErrorCode::None;
		std::uint64_t id = 0;
		std::uint64_t timestamp = 0;
		SFVR_Vec3 vector = { 0, 0, 1 };
	};

	//! SFVR_GazeConvergenceData struct
	/*! The vector (from the center of the player's head in world space) that can be used to approximate the point that the user is looking at. */
	struct SFVR_GazeConvergenceData
	{
		//! Error Code
		/*! error:	if true => the rest of the data is in an unknown state. */
		EFVR_ErrorCode error = EFVR_ErrorCode::None;
		//! ID
		/*! Incremental counter which tells if the convergence data is a fresh value at a given frame */
		std::uint64_t id = 0;
		//! Timestamp
		/*! The time at which the convergence data was captured, based on system time */
		std::uint64_t timestamp = 0;
		//! Ray
		/*! The ray pointing towards the expected convergence point */
		SFVR_Ray ray;
		//! float
		/*! The expected distance to the convergence point */
		float distance = 0.f;
		//! float
		/*! The accuracy of the convergence point */
		float accuracy = 0.f;
	};

	//! EFVR_Eye enum
	/*! This is usually returned with any eye tracking information and tells the client which eye(s) the information is based on. */
	enum class EFVR_Eye
	{
		Neither = 0,
		Left = 1,
		Right = 2,
		Both = 3
	};

	//! SFVR_Matrix44 struct
	/*! A rectangular array of numbers, symbols, or expressions, arranged in rows and columns.  */
	struct SFVR_Matrix44
	{
		float mat[4][4] = {};
	};

	//! SFVR_Matrix34 struct
	/*! A rectangular array of numbers, symbols, or expressions, arranged in rows and columns.  */
	struct SFVR_Matrix34
	{
		float mat[3][4] = {};
	};

	//! EFVR_CompositorError enum
	/*! Errors pertaining to Compositor */
	enum class EFVR_CompositorError
	{
		None = 0,

		UnableToCreateDeviceAndContext = 100,
		UnableToUseTexture = 101,
		DeviceMismatch = 102,
		IncompatibleCompositorVersion = 103,

		UnableToFindRuntime = 200,
		RuntimeAlreadyClaimed = 201,
		DisconnectedFromRuntime = 202,

		ErrorCreatingShaders = 300,
		ErrorCreatingTexturesOnDevice = 301,

		NoEyeSpecifiedForSubmit = 400,

		InvalidValue = 500,

		UnknownError = 99999,
	};

	//! EFVR_GraphicsAPI enum
	/*! Type of Graphics API */
	enum class EFVR_GraphicsAPI
	{
		DirectX = 0,
		OpenGL = 1,
		Vulkan = 2
	};

	//! EFVR_ColorSpace enum
	/*! Color space for a given texture */
	enum class EFVR_ColorSpace
	{
		Auto = 0,
		Linear = 1,
		GammaCorrected = 2
	};

	//! EFVR_AlphaMode enum
	/*! Determiness how to interpret the alpha of a compositor client texture */
	enum class EFVR_AlphaMode
	{
		Auto = 0,      // Base layers will use One, overlay layers will use Sample
		One = 1,       // Alpha will always be one (fully opaque)
		Sample = 2,    // Alpha fill be sampled from the alpha channel of the buffer
	};

	//! SFVR_ClientInfo struct
	/*! Structure used to define the settings for a compositor client.*/
	struct SFVR_ClientInfo
	{
		//! The type (layer) upon which the client will draw.
		EFVR_ClientType type = EFVR_ClientType::Base;
		//! The graphics API the client will use.
		EFVR_GraphicsAPI api = EFVR_GraphicsAPI::DirectX;

		//! Setting to disable timewarp, e.g. if an overlay client is operating in screen space.
		bool disableTimeWarp = false;
		//! Setting about whether to use alpha sampling or not, e.g. for a base client.
		EFVR_AlphaMode alphaMode = EFVR_AlphaMode::Auto;
		//! Setting to disable fading when the base layer is misbehaving, e.g. for a diagnostic client.
		bool disableFading = false;
		//! Setting to disable a distortion pass, e.g. for a diagnostic client, or a client intending to do its own distortion.
		bool disableDistortion = false;
	};

	//! UFVR_Texture union
	/*! Texture union used by compositor clients to pass textures. */
	union UFVR_CompositorTexture
	{
		void* pTexture;
		uint8_t reserved__[64]; // Set the absolute maximum size of this union.

		UFVR_CompositorTexture(void *pTex) : pTexture(pTex) {}
	};

	//! SFVR_CompositorTexture struct
	/*! Texture structure used by the Compositor */
	struct SFVR_CompositorTexture
	{
		//! Texture Pointer
		UFVR_CompositorTexture texture = { 0 };
		EFVR_ColorSpace colorSpace;
		SFVR_CompositorTexture(void *pTexture, EFVR_ColorSpace cs = EFVR_ColorSpace::Auto) : texture(pTexture), colorSpace(cs) {}
	};

	//! SFVR_TextureBounds struct
	/*! Coordinates in normalized space where 0 is left/top and 1 is bottom/right */
	struct SFVR_TextureBounds
	{
		float left = 0, top = 0, right = 0, bottom = 0;
	};
}
#endif // _FOVETYPES_H
