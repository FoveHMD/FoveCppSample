#pragma once

#include "FoveTypes.h"

namespace Fove
{
    //! Class to handle all Compositor inquiries
    class IFVRCompositor
    {
    public:
        //! Create a layer for this client.
        /*! This function create a layer upon which frames may be rendered, the details of which are passed in via
            an SFVR_CompositorLayerCreateInfo. It returns an ID for the layer, which must then be used when submitting frames.
            For information about the settings available when creating a layer, please check SFVR_CompositorLayerCreateInfo.
            \param layerInfo    The settings for the layer to be created.
        */
        virtual EFVR_ErrorCode CreateLayer(const SFVR_CompositorLayerCreateInfo& layerInfo, SFVR_CompositorLayer* outLayer) = 0;

        //! Submit a frame to the compositor
        /*! This function takes the feed from your game engine to the compositor for output.
            \param submitInfo   An array of layerCount SFVR_LayerSubmitInfo structs, each of which provides texture data for a unique layer.
            \param layerCount   The number of layers you are submitting
        */
        virtual EFVR_ErrorCode SubmitGroup(const SFVR_CompositorLayerSubmitInfo* submitInfo, std::size_t layerCount) = 0;

        //! Convenience helper to submit a single layer's frame to the compositor
        /*! Use the more SubmitGroup function if you have multiple layers.
        \param submitInfo   An object of type SFVR_LayerSubmitInfo, with information about the layer to be submitted.
        */
        EFVR_ErrorCode Submit(const SFVR_CompositorLayerSubmitInfo& singleLayer) { return SubmitGroup(&singleLayer, 1); }

        //! Wait for the most recent pose for rendering purposes.
        /*! All compositor clients should use this function as the sole means of limiting their frame rate.
            This allows the client to render at the correct frame rate for the HMD display.
            Upon this function returning, the client should proceed directly to rendering, to reduce the chance of missing the frame.
            If outPose is not null, this function will return the latest pose as a conveience to the caller.
            In general, a client's main loop should look like:
            {
                Update();                            // Run AI, physics, etc, for the next frame
                compositor.WaitForRenderPose(&pose); // Wait for the next frame, and get the pose
                Draw(pose);                          // Render the scene using the new pose
            }
        */
        virtual EFVR_ErrorCode WaitForRenderPose(SFVR_Pose* outPose) = 0;

        //! Get the last cached pose for rendering purposes.
        virtual EFVR_ErrorCode GetLastRenderPose(SFVR_Pose* outPose) const = 0;

        //! Closes any existing connections to the compositor, and cleans up any resources used by this class
        virtual ~IFVRCompositor() {}

        //! Returns true if we are connected to a running compositor and ready to submit frames for compositing
        virtual EFVR_ErrorCode IsReady(bool* out) const = 0;

        //! Returns the ID of the GPU currently attached to the headset.
        //! For systems with multiple GPUs, submitted textures to the compositor must from the same GPU that the compositor is using
        virtual EFVR_ErrorCode GetAdapterId(SFVR_AdapterId* out) = 0;

        //! Override delete to ensure that deallocation happens within the same dll as GetFVRCompositor's allocation
        FVR_EXPORT void operator delete(void* ptr);

        // Below are deprecated functions. Each has a non-deprecated equivalent above that existing code should use instead.
        // These will be removed in a future version.

        FVR_DEPRECATED(virtual SFVR_Pose WaitForRenderPose(),
            "Please use the version which takes an SFVR_Pose pointer and returns an EFVR_ErrorCode.") = 0;

        FVR_DEPRECATED(virtual SFVR_Pose GetLastRenderPose(),
            "Please use the version which takes an SFVR_Pose pointer and returns an EFVR_ErrorCode.") = 0;

        FVR_DEPRECATED(virtual bool IsReady() const,
            "Please use the version which takes a bool pointer and returns an EFVR_ErrorCode.") = 0;

        FVR_DEPRECATED(virtual SFVR_Vec2i GetSingleEyeResolution() const,
            "Please use the resolution passed in the SFVR_CompositorLayer struct returned by CreateLayer.") = 0;

        //! Submit a frame to the compositor
        /*! This function takes your feed from your game engine to the compositor for output
            NOTE: If this client has more than one layer, this function will error. Use SubmitGroup instead.
            NOTE: Frames will be sent synchronously when both left and right eyes have been submitted.
            \param whichEye Defines for which eye the given texture is to be output
            \param texInfo  The texture pointer and color space held by the struct
            \param bounds   The bounds for the texture to be displayed
            \param pose     The pose struct used for rendering this frame
        */
        FVR_DEPRECATED(virtual EFVR_ErrorCode Submit(
            EFVR_Eye whichEye,
            const SFVR_CompositorTexture &texInfo,
            const SFVR_TextureBounds &bounds,
            const SFVR_Pose &pose),
            "This overload of Submit is deprecated. Please use one of the other submit functions using SFVR_CompositorLayerSubmitInfo.") = 0;
    };

    //! Creates an IFVRCompositor object
    /*! Returns null in the event of an error
        The caller is reponsible for deleting the returned pointer when finished, preferably via RAII, such as std::unique_ptr<IFVRCompositor>
    */
    FVR_EXPORT IFVRCompositor* GetFVRCompositor();

    //! Creates an IFVRCompositor object and adds a layer
    /*! This is for backwards compatibility purposes and will be removed in a future version
    */
    FVR_EXPORT FVR_DEPRECATED(IFVRCompositor* GetFVRCompositor(const SFVR_CompositorLayerCreateInfo&),
        "GetFVRCompositor no longer takes an argument. Layers can be added CreateLayer");
}
