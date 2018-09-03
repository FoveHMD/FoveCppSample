#pragma once

#include "IFVRHeadset.h"
#include "FoveResearchTypes.h"

namespace Fove
{

class IFVRHeadsetResearch : public IFVRHeadset
{
public:
	//! Register for research data, requesting what research-level data you want to receive
	virtual EFVR_ErrorCode RegisterResearchCapabilities(EFVR_ResearchCapabilities caps) = 0;
	virtual EFVR_ErrorCode UnregisterResearchCapabilities(EFVR_ResearchCapabilities caps) = 0;

	//! Debug images from cameras
	//! Image buffers pointed to from outImage are valid until the next invocation of the same function.
	virtual EFVR_ErrorCode GetEyeImageData(SFVR_BitmapImage* outImage) = 0;
	virtual EFVR_ErrorCode GetPositionImageData(SFVR_BitmapImage* outImage) = 0;

	//! Returns the size in meters of the left and right pupil
	virtual EFVR_ErrorCode GetPupilRadius(float* outLeft, float* outRight) = 0;
};

} // namespace Fove
