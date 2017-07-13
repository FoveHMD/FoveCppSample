#pragma once

#include "FoveTypes.h"

namespace Fove
{
    //! Class to handle all Compositor inquiries
    class IFVRCompositor
    {
    public:
        //! Submit a frame to the compositor
        /*! This functions takes your feed from your game engine to render the texture to each eye
            \param whichEye Defines which eye to render the given texture to, Note: You will need to render for each eye seperately since the view vector for each eye is different
            \param texInfo  The texture pointer and color space held by the struct
            \param bounds   The bounds for the texture to be displayed
            \param pose     The pose struct used for rendering this frame
        */
        virtual EFVR_CompositorError Submit(
            EFVR_Eye whichEye,
            const SFVR_CompositorTexture &texInfo,
            const SFVR_TextureBounds &bounds,
            const SFVR_Pose &pose) = 0;

        //! Wait for the most recent pose for rendering purposes.
        virtual SFVR_Pose WaitForRenderPose() = 0;

        //! Get the last cached pose for rendering purposes.
        virtual SFVR_Pose GetLastRenderPose() = 0;

        //! Destructor
        virtual ~IFVRCompositor() {}

        //! Returns true if we are connected to a running compositor and ready to submit frames for compositing
        virtual bool IsReady() const = 0;

        //! Returns the recommended width and height to render a single eye at
        /*! A client may choose to render at different resolution (for example, to make a performance tradeoff)
            If IsReady() is false, this returns (0.0f, 0.0f)
            Values will be non-negative integers returned in floating point format
        */
        virtual SFVR_Vec2i GetSingleEyeResolution() const = 0;

        //! Override delete to ensure that deallocation happens within the same dll as GetFVRCompositor's allocation
        FVR_EXPORT void operator delete(void* ptr);
    };

    //! Creates an IFVRCompositor object
    /*! An application may create multiple of these to connect to the compositor on multiple layers
        The caller is responsible for destroying this object by calling delete
    */
    FVR_EXPORT IFVRCompositor* GetFVRCompositor(const SFVR_ClientInfo &clientInfo = SFVR_ClientInfo());
}
