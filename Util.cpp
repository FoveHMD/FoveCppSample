#include "Util.h"
#include <thread>
#include <codecvt>
#include <iterator>
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

Fove::SFVR_Vec2i GetSingleEyeResolutionWithTimeout(const Fove::IFVRCompositor& compositor, const chrono::milliseconds timeout, const Fove::SFVR_Vec2i defaultValue)
{
	const auto start = chrono::high_resolution_clock::now();

	do
	{
		// Query compositor for resolution (it will return 0,0 if the info is not yet available)
		const Fove::SFVR_Vec2i res = compositor.GetSingleEyeResolution();
		if (res.x > 0 && res.y > 0)
			return res;

		// Sleep a tiny bit before the next query
		this_thread::yield();

	} while (chrono::high_resolution_clock::now() - start < timeout);

	return defaultValue;
}

string ToUtf8(const wstring& utf16)
{
	return wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(utf16);
}

wstring ToUtf16(const string& str)
{
	return wstring_convert<codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
}

Fove::SFVR_Quaternion AxisAngleToQuat(const float vx, const float vy, const float vz, const float angle)
{
	const float s = sin(angle / 2);
	const float c = cos(angle / 2);
	return Fove::SFVR_Quaternion(vx * s, vy * s, vz * s, c);
}

Fove::SFVR_Matrix44 QuatToMatrix(const Fove::SFVR_Quaternion q)
{
	Fove::SFVR_Matrix44 ret;
	ret.mat[0][0] = 1 - 2 * q.y*q.y - 2 * q.z*q.z; ret.mat[0][1] = 2 * q.x*q.y - 2 * q.z*q.w; ret.mat[0][2] = 2 * q.x*q.z + 2 * q.y*q.w; ret.mat[0][3] = 0;
	ret.mat[1][0] = 2 * q.x*q.y + 2 * q.z*q.w; ret.mat[1][1] = 1 - 2 * q.x*q.x - 2 * q.z*q.z; ret.mat[1][2] = 2 * q.y*q.z - 2 * q.x*q.w; ret.mat[1][3] = 0;
	ret.mat[2][0] = 2 * q.x*q.z - 2 * q.y*q.w; ret.mat[2][1] = 2 * q.y*q.z + 2 * q.x*q.w; ret.mat[2][2] = 1 - 2 * q.x*q.x - 2 * q.y*q.y; ret.mat[2][3] = 0;
	ret.mat[3][0] = 0; ret.mat[3][1] = 0; ret.mat[3][2] = 0; ret.mat[3][3] = 1;
	return ret;
}

Fove::SFVR_Matrix44 Transpose(const Fove::SFVR_Matrix44& m)
{
	Fove::SFVR_Matrix44 ret;
	ret.mat[0][0] = m.mat[0][0]; ret.mat[0][1] = m.mat[1][0]; ret.mat[0][2] = m.mat[2][0]; ret.mat[0][3] = m.mat[3][0];
	ret.mat[1][0] = m.mat[0][1]; ret.mat[1][1] = m.mat[1][1]; ret.mat[1][2] = m.mat[2][1]; ret.mat[1][3] = m.mat[3][1];
	ret.mat[2][0] = m.mat[0][2]; ret.mat[2][1] = m.mat[1][2]; ret.mat[2][2] = m.mat[2][2]; ret.mat[2][3] = m.mat[3][2];
	ret.mat[3][0] = m.mat[0][3]; ret.mat[3][1] = m.mat[1][3]; ret.mat[3][2] = m.mat[2][3]; ret.mat[3][3] = m.mat[3][3];
	return ret;
}

Fove::SFVR_Matrix44 TranslationMatrix(const float x, const float y, const float z)
{
	Fove::SFVR_Matrix44 ret;
	ret.mat[0][0] = 1; ret.mat[0][1] = 0; ret.mat[0][2] = 0; ret.mat[0][3] = x;
	ret.mat[1][0] = 0; ret.mat[1][1] = 1; ret.mat[1][2] = 0; ret.mat[1][3] = y;
	ret.mat[2][0] = 0; ret.mat[2][1] = 0; ret.mat[2][2] = 1; ret.mat[2][3] = z;
	ret.mat[3][0] = 0; ret.mat[3][1] = 0; ret.mat[3][2] = 0; ret.mat[3][3] = 1;
	return ret;
}

Fove::SFVR_Matrix44 operator * (const Fove::SFVR_Matrix44& m1, const Fove::SFVR_Matrix44& m2)
{
	Fove::SFVR_Matrix44 ret;
	for (int row = 0; row < 4; row++)
	{
		for (int column = 0; column < 4; column++)
		{
			float v = 0;
			for (int i = 0; i < 4; i++)
				v += m1.mat[row][i] * m2.mat[i][column];
			ret.mat[row][column] = v;
		}
	}
	return ret;
}

string GetErrorString(const ErrorType error) noexcept try
{
	string ret = to_string(error);

#ifdef _WIN32
	// Call FormatMessage to get error string from winapi
	LPWSTR _buffer = nullptr;
	const DWORD size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&_buffer, 0, nullptr);
	const unique_ptr<wchar_t, decltype(&LocalFree)> buffer(_buffer, &LocalFree);
	if (size > 0)
	{
		// Add message to return buffer
		const wstring wstr(buffer.get(), size);
		ret += ' ';
		ret += ToUtf8(wstr);

		// Windows seems to add newlines, remove them
		ret.erase(remove_if(ret.begin(), ret.end(), [](const char c) { return c == '\r' || c == '\n'; }), ret.end());
	}
#endif

	return ret;
}
catch (const exception&)
{
	// If we fail in any way generating the error string, return a generic error and move on
	return "ERROR_FAILURE";
}

string GetLastErrorAsString() noexcept
{
#ifdef _WIN32
	return GetErrorString(GetLastError());
#else
	return 0; // Not implemented for other platforms yet
#endif
}
