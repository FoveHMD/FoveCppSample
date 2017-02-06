#pragma once
#include <memory>

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
