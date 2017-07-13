#pragma once

#ifdef __GNUC__
#define FVR_DEPRECATED(func, rem) func __attribute__ ((deprecated(rem)))
#define FVR_EXPORT __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define FVR_DEPRECATED(func, rem) __declspec(deprecated(rem)) func
#define FVR_EXPORT __declspec(dllexport)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define FVR_DEPRECATED(func, rem) func
#define FVR_EXPORT
#endif

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Fove
{
    //! Client capabilities to be requested
    /*! To be passed to the initialisation function of the client library to  */
    enum class EFVR_ClientCapabilities
    {
        None = 0x00,
        Gaze = 0x01,
        Orientation = 0x02,
        Position = 0x04
    };

    // hide functions for documentation
    /// @cond EFVR_ClientCapabilities_Functions
    inline EFVR_ClientCapabilities operator|(EFVR_ClientCapabilities a, EFVR_ClientCapabilities b)
    {
        return static_cast<EFVR_ClientCapabilities>(static_cast<int>(a) | static_cast<int>(b));
    }
    inline EFVR_ClientCapabilities operator&(EFVR_ClientCapabilities a, EFVR_ClientCapabilities b)
    {
        return static_cast<EFVR_ClientCapabilities>(static_cast<int>(a) & static_cast<int>(b));
    }
    inline EFVR_ClientCapabilities operator~(EFVR_ClientCapabilities a)
    {
        // bitwise negation
        return static_cast<EFVR_ClientCapabilities>(~static_cast<int>(a));
    }
    /// @endcond

    //! An enum of error codes that the system may return
    /*! An enum of error codes that the system may return from GetLastError(). These will eventually be mapped to localised strings. */
    enum class EFVR_ErrorCode
    {
        None = 0,

        //! Connection Errors
        Connection_General = 1,
        Connect_NotConnected = 7,
        Connect_ServerUnreachable = 2,
        Connect_RegisterFailed = 3,
        Connect_DeregisterFailed = 8,
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
        Position_NoCameraFound = 5009,

        //! Eye Tracking
        Eye_Left_NoDlibRegressor = 6000,
        Eye_Right_NoDlibRegressor = 6001,
        Eye_CalibrationFailed = 6002,
        Eye_LoadCalibrationFailed = 6003,

        //! User
        User_General = 7000,
        User_ErrorLoadingProfile = 7001,
    };

    //! Enum Corresponds to the order in which clients are composited
    /*! Corresponds to the order in which clients are composited (Base, then Overlay, then Diagnostic) */
    enum class EFVR_ClientType
    {
        Base = 0,             /*!< The first layer all the way in the background */
        Overlay = 0x10000,    /*!< Layer over the Base */
        Diagnostic = 0x20000  /*!< Layer over Overlay */
    };

    //! EFVR_Status
    /*! An enum used for the system status health check that tells you which parts of the hardware and software are functioning */
    enum class EFVR_HealthStatus
    {
        Unknown,
        Healthy,
        Uncalibrated,
        Sleeping,
        Disconnected,
        Error,
    };

    enum class EFVR_BitmapImageType
    {
        StereoEye = 0x00,
        Position = 0x01
    };

    struct SFVR_BitmapImage
    {
        uint64_t timestamp = 0;
        EFVR_ErrorCode error = EFVR_ErrorCode::Data_NoUpdate;
        EFVR_BitmapImageType type;
        std::vector<unsigned char> image;
    };

    //! SFVR_SystemHealth
    /*! Contains the health status and error codes for the HMD, position camera, position LEDs, eye camera and eye LEDs */
    struct SFVR_SystemHealth
    {
        /*! The health status of the HMD */
        EFVR_HealthStatus HMD = EFVR_HealthStatus::Unknown;
        /*! Any error message from the HMD */
        EFVR_ErrorCode HMDError = EFVR_ErrorCode::None;
        /*! The health status of the position camera */
        EFVR_HealthStatus PositionCamera = EFVR_HealthStatus::Unknown;
        /*! Any error message from the position camera */
        EFVR_ErrorCode PositionCameraError = EFVR_ErrorCode::None;
        /*! The health status of the eye cameras */
        EFVR_HealthStatus EyeCamera = EFVR_HealthStatus::Unknown;
        /*! Any error message from the eye cameras */
        EFVR_ErrorCode EyeCameraError = EFVR_ErrorCode::None;
        /*! The health status of the position LEDs */
        EFVR_HealthStatus PositionLEDs = EFVR_HealthStatus::Unknown;
        /*! The health status of the eye LEDs */
        EFVR_HealthStatus EyeLEDs = EFVR_HealthStatus::Unknown;

        char reserved[40]; // consume these as we add more statuses
    };

    //! Struct Contains the version for the software
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

    //! Struct representation on a quaternion
    /*! A quaternion represents an orientation in 3d space.*/
    struct SFVR_Quaternion
    {
        float x = 0;
        float y = 0;
        float z = 0;
        float w = 1;

        //! default quaternion constructor by c++
        SFVR_Quaternion() = default;

        /*! Initialize the Quaternion
            \param ix x-component of Quaternion
            \param iy y-component of Quaternion
            \param iz z-component of Quaternion
            \param iw w-component of Quaternion
        */
        SFVR_Quaternion(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}

        //! Generate and return a conjugate of this quaternion
        SFVR_Quaternion Conjugate() const
        {
            return SFVR_Quaternion(-x, -y, -z, w);
        }

        //! Normalize the Quaternion
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

    //! Struct to represent a 3D-vector
    /*! A vector that represents an position in 3d space. */
    struct SFVR_Vec3
    {
        float x = 0;
        float y = 0;
        float z = 0;

        //! default vector constructor by c++
        SFVR_Vec3() = default;

        /*! Initialize the Vector
            \param ix x-component of Vector
            \param iy y-component of Vector
            \param iz z-component of Vector
        */
        SFVR_Vec3(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
    };

    //! Struct to represent a 2D-vector
    /*! A vector that represents an position in 2d space. Usually used when refering to screen or image coordinates. */
    struct SFVR_Vec2
    {
        float x = 0;
        float y = 0;

        //! default vector constructor by c++
        SFVR_Vec2() = default;

        /*! Initialize the 2-component float vector
            \param ix The x component of the vector
            \param iy The y component of the vector
        */
        SFVR_Vec2(float ix, float iy) : x(ix), y(iy) {}
    };

    //! Struct to represent a 2D-vector
    /*! A 2-component integer vector. */
    struct SFVR_Vec2i
    {
        int x = 0;
        int y = 0;

        //! default vector constructor by c++
        SFVR_Vec2i() = default;

        /*! Initialize the 2-component integer vector
            \param ix The x component of the vector
            \param iy The y component of the vector
        */
        SFVR_Vec2i(int ix, int iy) : x(ix), y(iy) {}
    };

    //! Struct to represent a Ray
    /*! Stores the start point and direction of a Ray */
    struct SFVR_Ray
    {
        //! The start point of the Ray
        SFVR_Vec3 origin;
        //! The direction of the Ray
        SFVR_Vec3 direction = { 0, 0, 1 };

        //! default Ray constructor by c++
        SFVR_Ray() = default;

        /*! Initialize the Ray
            \param _origin    The start point of the ray
            \param _direction The direction of the ray
        */
        SFVR_Ray(const SFVR_Vec3& _origin, const SFVR_Vec3& _direction) : origin(_origin), direction(_direction) {}
    };

    //! Struct to represent a combination of position and orientation of Fove Headset
    /*! This structure is a combination of the Fove headset position and orientation in 3d space, collectively known as the "pose".
        In the future this may also contain accelleration information for the headset, and may also be used for controllers.
    */
    struct SFVR_Pose
    {
        /*! error: if true => the rest of the data is in an unknown state. */
        EFVR_ErrorCode error = EFVR_ErrorCode::None;
        /*! Incremental counter which tells if the coord captured is a fresh value at a given frame */
        std::uint64_t id = 0;
        /*! The time at which the pose was captured, in milliseconds since an unspecified epoch */
        std::uint64_t timestamp = 0;
        /*! The Quaternion which represents the orientation of the head. */
        SFVR_Quaternion orientation;
        /*! The angular velocity of the head. */
        SFVR_Vec3 angularVelocity;
        /*! The angular acceleration of the head. */
        SFVR_Vec3 angularAcceleration;
        /*! The position of headset in 3D space */
        SFVR_Vec3 position;
        /*! The velocity of headset in 3D space */
        SFVR_Vec3 velocity;
        /*! The acceleration of headset in 3D space */
        SFVR_Vec3 acceleration;
    };

    //! Struct to represent the vector which eyes converge at
    /*! Stores the Gaze vector which both eyes converge at
    */
    struct SFVR_GazeVector
    {
        /*! error: if true => the rest of the data is in an unknown state. */
        EFVR_ErrorCode error = EFVR_ErrorCode::None;
        /*! Incremental counter which tells if the convergence data is a fresh value at a given frame */
        std::uint64_t id = 0;
        /*! The time at which the gaze data was captured, in milliseconds since an unspecified epoch */
        std::uint64_t timestamp = 0;
        SFVR_Vec3 vector = { 0, 0, 1 };
    };

    //! Struct to represent the vector pointing where the user is looking at.
    /*! The vector (from the center of the player's head in world space) that can be used to approximate the point that the user is looking at. */
    struct SFVR_GazeConvergenceData
    {
        /*! error: if true => the rest of the data is in an unknown state. */
        EFVR_ErrorCode error = EFVR_ErrorCode::None;
        /*! Incremental counter which tells if the convergence data is a fresh value at a given frame */
        std::uint64_t id = 0;
        /*! The time at which the convergence data was captured, in milliseconds since an unspecified epoch */
        std::uint64_t timestamp = 0;
        /*! The ray pointing towards the expected convergence point */
        SFVR_Ray ray;
        /*! The expected distance to the convergence point, Range: 0 to Infinity*/
        float distance = 0.f;
        /*! The accuracy of the convergence point, Range: 0 to +1 */
        float accuracy = 0.f;
    };

    //! Enum to identify which eye is being used.
    /*! This is usually returned with any eye tracking information and tells the client which eye(s) the information is based on. */
    enum class EFVR_Eye
    {
        Neither = 0, /*!< Neither eye */
        Left = 1,    /*!< Left eye only */
        Right = 2,   /*!< Right eye only */
        Both = 3     /*!< Both eyes */
    };

    //! Struct to hold a rectangular array
    /*! A rectangular array of numbers, symbols, or expressions, arranged in rows and columns.  */
    struct SFVR_Matrix44
    {
        float mat[4][4] = {};
    };

    //! Struct to hold a rectangular array
    /*! A rectangular array of numbers, symbols, or expressions, arranged in rows and columns.  */
    struct SFVR_Matrix34
    {
        float mat[3][4] = {};
    };

    //! Enum with errors pertaining to Compositor
    /*! Errors pertaining to Compositor */
    enum class EFVR_CompositorError
    {
        None = 0, /*!< No Error */

        UnableToCreateDeviceAndContext = 100, /*!< Compositor was unable to initialize its backend component. */
        UnableToUseTexture = 101,             /*!< Compositor was unable to use the given texture (likely due to mismatched client and data types or an incompatible format). */
        DeviceMismatch = 102,                 /*!< Compositor was unable to match its device to the texture's, either because of multiple GPUs or a failure to get the device from the texture. */
        IncompatibleCompositorVersion = 103,  /*!< Compositor client is not compatible with the currently running compositor. */

        UnableToFindRuntime = 200,            /*!< Compositor isn't running or isn't responding. */
        RuntimeAlreadyClaimed = 201,          /*!< Deprecated */
        DisconnectedFromRuntime = 202,        /*!< Compositor was running and is no longer responding. */

        ErrorCreatingShaders = 300,           /*!< Deprecated */
        ErrorCreatingTexturesOnDevice = 301,  /*!< Failed to create shared textures for compositor. */

        NoEyeSpecifiedForSubmit = 400,        /*!< The supplied EFVR_Eye for submit is invalid (i.e. is Both or Neither). */

        InvalidValue = 500,                   /*!< Currently unused, maybe utilized in the future. */

        UnknownError = 99999,                 /*!< Misc. errors otherwise unhandled. */
    };

    //! enum for type of Graphics API
    /*! Type of Graphics API
        Note: We currently only support DirectX
    */
    enum class EFVR_GraphicsAPI
    {
        DirectX = 0, /*!< Good old Windows DirectX API */
        OpenGL = 1,  /*!< OpenGL not implemented */
        Vulkan = 2   /*!< Vulcan not implemented */
    };

    //! Enum to represent the Color space for a given texture
    /*! Color space for a given texture
        Note: Auto is not setup for use yet
    */
    enum class EFVR_ColorSpace
    {
        Auto = 0,          /*!< Not Setup for use*/
        Linear = 1,        /*!< The texture is not Gamma Corrected i.e. is linear in format*/
        GammaCorrected = 2 /*!< The texture is already Gamma Corrected*/
    };

    //! Enum to help interpret the alpha of texture
    /*! Determines how to interpret the alpha of a compositor client texture */
    enum class EFVR_AlphaMode
    {
        Auto = 0,   /*!< Base layers will use One, overlay layers will use Sample */
        One = 1,    /*!< Alpha will always be one (fully opaque) */
        Sample = 2, /*!< Alpha fill be sampled from the alpha channel of the buffer */
    };

    //! Struct used to define the settings for a compositor client.
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

    //! Union used by compositor clients to pass textures.
    /*! Texture union used by compositor clients to pass textures. */
    union UFVR_CompositorTexture
    {
        //! Texture pointer
        void* pTexture;
        //! Set the absolute maximum size of this union.
        uint8_t reserved__[64];

        /*! Initialize the Union member variables
            \param pTex Texture pointer used to render texture
        */
        UFVR_CompositorTexture(void *pTex) : pTexture(pTex) {}
    };

    //! Struct used by the Compositor to setup texture
    /*! Texture structure used by the Compositor
    */
    struct SFVR_CompositorTexture
    {
        //! Texture Pointer
        UFVR_CompositorTexture texture = { 0 };
        //! Colorspace of the texture
        EFVR_ColorSpace colorSpace;

        /*! Initialize the Struct member variables
            \param pTexture Texture pointer used to render texture
            \param cs       Color space of the texture
        */
        SFVR_CompositorTexture(void *pTexture, EFVR_ColorSpace cs = EFVR_ColorSpace::Auto) : texture(pTexture), colorSpace(cs) {}
    };

    //! Struct to represent coordinates in normalized space
    /*! Coordinates in normalized space where 0 is left/top and 1 is bottom/right */
    struct SFVR_TextureBounds
    {
        ///@{
        float left = 0;
        float top = 0;
        float right = 0;
        float bottom = 0;
        ///@}
    };
}
