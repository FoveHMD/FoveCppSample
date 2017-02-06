#pragma once

#ifndef _IFVRHEADSET_H
#define _IFVRHEADSET_H

#include "FoveTypes.h"

namespace Fove
{
	class IFVRHeadset
	{
	public:
		// members
		// General
		/*! Initialise the client with desired capabilities
			\param capabilities	The desired capabilities (Gaze, Orientation, Position)
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
		//! Returns the version numbers for the client, FOVE SDK and firmware
		virtual void GetSoftwareVersions(SFVR_Versions* outVersions) = 0;
		
		//! eye tracking
		/*! Returns the direction the specified eye is looking towards
			\param eye	The desired eye
		*/
		virtual SFVR_GazeVector GetGazeVector(EFVR_Eye eye) = 0;
		//! Returns a vector (from the center of the player's eyes in world space) that can be used to approximate the point that the user is looking at.
		virtual SFVR_GazeConvergenceData GetGazeConvergence() = 0;
		//! Returns which eye - or eyes - are closed
		virtual EFVR_Eye CheckEyesClosed() = 0;
		//! Returns which eye - or eyes - are being tracked
		virtual EFVR_Eye CheckEyesTracked() = 0;

		// status
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
		//! Tares the orientation of the headset
		virtual bool TareOrientationSensor() = 0;

		// position tracking
		//! Returns whether position tracking hardware has started
		virtual bool IsPositionReady() = 0;
		//! Tares the position of the headset
		virtual bool TarePositionSensors() = 0;

		//! Returns the pose of the head-mounted display
		virtual SFVR_Pose GetHMDPose() = 0;
		/*! Returns the pose of the device identified by the specified index
			\param id	The identifier of the desired device
		*/
		virtual SFVR_Pose GetPoseByIndex(int id) = 0;

		// metrics
		/*! Returns a 4x4 projection matrix for the specified eye, and near and far planes in a left-handed coordinate system
			\param whichEye	The desired eye
		*/
		virtual SFVR_Matrix44 GetProjectionMatrixLH(EFVR_Eye whichEye, float zNear, float zFar) = 0;
		/*! Returns a 4x4 projection matrix for the specified eye, and near and far planes in a right-handed coordinate system
			\param whichEye	The desired eye
		*/
		virtual SFVR_Matrix44 GetProjectionMatrixRH(EFVR_Eye whichEye, float zNear, float zFar) = 0;
		/*! Returns values for the view frustum of the specified eye at 1 unit away. Please convert yourself by multiplying by zNear.
			\param whichEye	The specified eye
			\param l		A float pointer, whose value will be set to the left-bound of the view-frustum
			\param r		A float pointer, whose value will be set to the right-bound of the view-frustum
			\param t		A float pointer, whose value will be set to the top-bound of the view-frustum
			\param b		A float pointer, whose value will be set to the bottom-bound of the view-frustum
		*/
		virtual void AssignRawProjectionValues(EFVR_Eye whichEye, float *l, float *r, float *t, float *b) = 0;
		//! Returns the matrix that converts coordinates from eye- to head-space
		virtual SFVR_Matrix44 GetEyeToHeadMatrix(EFVR_Eye whichEye) = 0;

		//! constructor & destructor
		virtual ~IFVRHeadset() {}

		//!-- Override delete to ensure that deallocation happens within the same dll as GetFVRHeadset's allocation
		FVR_EXPORT void operator delete(void* ptr);

		//! Drift correction
		virtual EFVR_ErrorCode TriggerOnePointCalibration() = 0;
		virtual EFVR_ErrorCode ManualDriftCorrection3D(SFVR_Vec3 position) = 0;

		//! Interocular distance, returned in meters
		virtual EFVR_ErrorCode GetIOD(float& outIOD) const = 0;
	};

	FVR_EXPORT IFVRHeadset* GetFVRHeadset();
}
#endif // _IFVRHEADSET_H