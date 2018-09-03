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

//! SFVR_Buffer
/*! A generic memory buffer.
 *  No ownership semantics are provided. Please see the comments on the function which
 *  returned the buffer to see how long the data is valid.
 */
struct SFVR_Buffer
{
	//! Pointer to the start of the memory buffer
	const void* data;
	//! Length, in bytes, of the buffer
	size_t length;
};

struct SFVR_BitmapImage
{
	//! Timestamp of the image, in milliseconds since an unspecified epoch.
	uint64_t timestamp = 0;
	//! Type of the bitmap for disambiguation.
	EFVR_BitmapImageType type;
	//! BMP data (including full header that contains size, format, etc)
	//! Note: The height may be negative to specify a top-down bitmap.
	SFVR_Buffer image;
};

} // namespace Fove
