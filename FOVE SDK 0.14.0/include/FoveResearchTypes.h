#pragma once

#include <vector>

namespace Fove
{

enum class EFVR_ResearchCapabilities
{
	None = 0x00,
	EyeImage = 0x01,
	PositionImage = 0x02,
};

// hide functions for documentation
/// @cond EFVR_ResearchCapabilities_Functions
inline EFVR_ResearchCapabilities operator|(EFVR_ResearchCapabilities a, EFVR_ResearchCapabilities b)
{
	return static_cast<EFVR_ResearchCapabilities>(static_cast<int>(a) | static_cast<int>(b));
}
inline EFVR_ResearchCapabilities operator&(EFVR_ResearchCapabilities a, EFVR_ResearchCapabilities b)
{
	return static_cast<EFVR_ResearchCapabilities>(static_cast<int>(a) & static_cast<int>(b));
}
inline EFVR_ResearchCapabilities operator~(EFVR_ResearchCapabilities a)
{
	// bitwise negation
	return static_cast<EFVR_ResearchCapabilities>(~static_cast<int>(a));
}
/// @endcond

enum class EFVR_BitmapImageType
{
	StereoEye = 0x00,
	Position = 0x01
};

struct SFVR_BitmapImage
{
	uint64_t timestamp = 0;
	EFVR_BitmapImageType type;
	std::vector<unsigned char> image;
};

} // namespace Fove
