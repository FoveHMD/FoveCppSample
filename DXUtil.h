#pragma once
#include <memory>
#include <string>
#include "atlstr.h"

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


std::string HResultToString(HRESULT hr) {
	LPWSTR output;
	if (FAILED(hr)) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&output, 0, NULL);
	}
	return CW2A(output);
}
