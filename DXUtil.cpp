#include "DXUtil.h"
#include "Util.h"
#include <Windows.h>

using namespace std;

string HResultToString(const HRESULT result) noexcept try {
	// Handle success case
	if (!FAILED(result))
		return "Success";

	// Pass error through standard windows TranslateMessage
	const HRESULT code = result & 0xffff; // Error code is the lower word of the HRESULT
	return GetErrorString((DWORD)code);   // Call other helper to convert win32 error to string
} catch (const exception&) {
	// If we fail in any way generating the error string, return a generic error and move on
	return "HRESULT_FAILURE";
}
