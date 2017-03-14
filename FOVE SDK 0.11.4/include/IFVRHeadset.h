#pragma once

#ifndef _IFVRHEADSET_H
#define _IFVRHEADSET_H

#include "FoveTypes.h"

// This is purely need for doxygen documentation
/*! \mainpage Fove-SDK C++ Documentation
*
* \section intro_sec Introduction
*
* This is the Fove-SDK documentation (C++).
*
* \section install_sec Example
*
* We have a sample hosted on github: https://github.com/FoveHMD/FoveCppSample
*
*/

namespace Fove
{
    //! Class to handle all Fove headset related inquiries
    class IFVRHeadset
    {
    public:
        // members
        // General
        //! Initialise the client with desired capabilities
        /*! Initialise the client with desired capabilities
            \param capabilities The desired capabilities (Gaze, Orientation, Position), for multiple capabilities, use piped input  e.g.: EFVR_ClientCapabilities::Gaze | EFVR_ClientCapabilities::Position
        */
        virtual bool Initialise(EFVR_ClientCapabilities capabilities) = 0;
        //! Returns whether the hardware is connected
        virtual bool IsHardwareConnected() = 0;
        //! Returns whether the hardware for the requested capabilities has started
        virtual bool IsHardwareReady() = 0;
        //! Checks whether the client can run against the installed version of the FOVE SDK
        virtual EFVR_ErrorCode CheckSoftwareVersions() = 0;
        //! Returns the latest reported error
        virtual EFVR_ErrorCode GetLastError() = 0;
        //! Please call CheckSoftwareVersions
        virtual void GetSoftwareVersions(SFVR_Versions* outVersions) = 0;
        
        //! eye tracking
        /*! Returns the direction the specified eye is looking towards
            \param eye The desired eye
        */
        virtual SFVR_GazeVector GetGazeVector(EFVR_Eye eye) = 0;
        //! Returns a vector (from the center of the player's eyes in world space) that can be used to approximate the point that the user is looking at.
        virtual SFVR_GazeConvergenceData GetGazeConvergence() = 0;
        //! Returns which eye - or eyes - are closed
        virtual EFVR_Eye CheckEyesClosed() = 0;
        //! Returns which eye - or eyes - are being tracked
        virtual EFVR_Eye CheckEyesTracked() = 0;

        //! Returns whether the eye tracking hardware has started
        virtual bool IsEyeTrackingEnabled() = 0;
        //! Returns whether eye tracking has been calibrated
        virtual bool IsEyeTrackingCalibrated() = 0;
        //! Returns whether eye tracking is in the process of performing a calibration
        virtual bool IsEyeTrackingCalibrating() = 0;
        //! Returns whether eye tracking is actively tracking an eye - or eyes (hardware is enabled and eye tracking is calibrated)
        virtual bool IsEyeTrackingReady() = 0;

        // motion sensor
        //! Returns whether motion tracking hardware has started
        virtual bool IsMotionReady() = 0;
        //! Tares the orientation of the headset and returns whether it was successful
        virtual bool TareOrientationSensor() = 0;

        // position tracking
        //! Returns whether position tracking hardware has started and returns whether it was successful
        virtual bool IsPositionReady() = 0;
        //! Tares the position of the headset
        virtual bool TarePositionSensors() = 0;

        //! Returns the pose of the head-mounted display
        virtual SFVR_Pose GetHMDPose() = 0;
        //! Returns the pose of the device
        /*! Returns the pose of the device identified by the specified index
            \param id The identifier of the desired device
        */
        virtual SFVR_Pose GetPoseByIndex(int id) = 0;

        // metrics
        //! Returns a 4x4 projection matrix for left-handed coordinate system
        /*! Returns a 4x4 projection matrix for the specified eye, and near and far planes in a left-handed coordinate system
            \param whichEye The desired eye
            \param zNear    The near plane in float, Range: from 0 to zFar
            \param zFar     The far plane in float, Range: from zNear to infinity
        */
        virtual SFVR_Matrix44 GetProjectionMatrixLH(EFVR_Eye whichEye, float zNear, float zFar) = 0;
        //! Returns a 4x4 projection matrix for right-handed coordinate system
        /*! Returns a 4x4 projection matrix for the specified eye, and near and far planes in a right-handed coordinate system
            \param whichEye The desired eye
            \param zNear    The near plane in float, Range: from 0 to zFar
            \param zFar     The far plane in float, Range: from zNear to infinity
        */
        virtual SFVR_Matrix44 GetProjectionMatrixRH(EFVR_Eye whichEye, float zNear, float zFar) = 0;
        //! Returns values for the view frustum of the specified eye at 1 unit away.
        /*! Returns values for the view frustum of the specified eye at 1 unit away. Please convert yourself by multiplying by zNear.
            \param whichEye The specified eye
            \param l        A float pointer, whose value will be set to the left-bound of the view-frustum (at 1 meter from the view origin)
            \param r        A float pointer, whose value will be set to the right-bound of the view-frustum (at 1 meter from the view origin)
            \param t        A float pointer, whose value will be set to the top-bound of the view-frustum (at 1 meter from the view origin)
            \param b        A float pointer, whose value will be set to the bottom-bound of the view-frustum (at 1 meter from the view origin)
        */
        virtual void AssignRawProjectionValues(EFVR_Eye whichEye, float *l, float *r, float *t, float *b) = 0;
        //! Returns the matrix that converts coordinates from eye- to head-space
        virtual SFVR_Matrix44 GetEyeToHeadMatrix(EFVR_Eye whichEye) = 0;

        //! constructor & destructor
        virtual ~IFVRHeadset() {}

        //! Override delete to ensure that deallocation happens within the same dll as GetFVRHeadset's allocation
        FVR_EXPORT void operator delete(void* ptr);

        //! Drift correction - Not implemented yet
        virtual EFVR_ErrorCode TriggerOnePointCalibration() = 0;
        //! Manual Drift correction - Not implemented yet
        virtual EFVR_ErrorCode ManualDriftCorrection3D(SFVR_Vec3 position) = 0;

        //! Interocular distance, returned in meters
        virtual EFVR_ErrorCode GetIOD(float& outIOD) const = 0;

        /*! Returns the status of the hardware.
            \param outStatus A pointer to a SFVR_SystemHealth struct that will be used to output all system components' health states
            \param runTest   If true, the eye tracking and position tracking systems are activated (if necessary) to make sure they work
        */
        virtual void GetSystemHealth(SFVR_SystemHealth* outStatus, bool runTest) = 0;

        /*! Starts calibration if needed
            All eye tracking content should call this before using the gaze to ensure that the calibration is good
            After calling this, content should periodically poll for IsEyeTrackingCalibration() to become false,
            so as to ensure that the content is not updating while obscured by the calibrator
        */
        virtual EFVR_ErrorCode EnsureEyeTrackingCalibration() = 0;
    };

    FVR_EXPORT IFVRHeadset* GetFVRHeadset();
}
#endif // _IFVRHEADSET_H
