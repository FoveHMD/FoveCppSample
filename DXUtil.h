#pragma once
#include <memory>
#include <string>
#include <winerror.h> // For HRESULT

// Helper for deleting directX objects (see DXObj below)
template<typename DXType>
struct DXReleaseHelper
{
	void operator () (DXType* const object)
	{
		if (object)
			object->Release();
	}
};

// RAII helper for DirectX objects to avoid old school manual memory management
// This is similar to CComPointer but safer, lower overhead, and more standardized
template<typename DXType>
using DXObj = std::unique_ptr<DXType, DXReleaseHelper<DXType>>;

// Returns a string representing the result, with additional details if possible
std::string HResultToString(HRESULT result) noexcept;
