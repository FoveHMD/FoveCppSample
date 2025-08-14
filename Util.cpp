#include "Util.h"
#include <cstdint>
#include <exception>

#ifdef _WIN32
#include <Windows.h>
#include <memory>
#include <string>
#endif

using namespace std;

string currentExceptionMessage()
{
	const auto exceptionPtr = current_exception();
	if (nullptr == exceptionPtr)
		return "no exception";

	try
	{
		rethrow_exception(exceptionPtr);
	}
	catch (const char* str)
	{
		return str;
	}
	catch (const string& str)
	{
		return str;
	}
	catch (const exception& e)
	{
		return e.what();
	}
	catch (...)
	{
	}

	return "unknown exception";
}

string toUtf8(const wstring& utf16)
{
	// This is a simple conversion just to keep this example code simple and dependency free
	// We recommend using a more advanced, performant, and well tested library for real applications

	string utf8;
	uint32_t lastCodePoint = 0; // Used for surrogate pairs
	for (const wchar_t& ch : utf16)
	{
		// First, decode the utf16 code point, handling surrogate pairs
		uint32_t codePoint = static_cast<uint32_t>(ch);
		if (lastCodePoint != 0)
		{
			// This is the second half of a surrogate pair
			codePoint = 0x10000 + ((lastCodePoint - 0xd800) << 10) + (codePoint - 0xdc00);
			lastCodePoint = 0;
		}
		else if (codePoint >= 0xd800 && codePoint <= 0xdbff)
		{
			// This is the first half of a surrogate pair
			lastCodePoint = codePoint;
			continue; // Loop around to get the next half of the pair
		}
		else
		{
			// This is not a surrogate pair, just a single code point
			// Nothing needed, codePoint is jused used below
		}

		// Write out the utf8 char
		if (codePoint <= 0x7f) // 1-byte utf8 char
		{
			utf8 += static_cast<char>(codePoint);
		}
		else if (ch <= 0x7ff) // 2-byte utf8 char
		{
			utf8 += static_cast<char>(0xc0 | ((codePoint >> 6) & 0x1f));
			utf8 += static_cast<char>(0x80 | (codePoint & 0x3f));
		}
		else if (ch <= 0xffff) // 3-byte utf8 char
		{
			utf8 += static_cast<char>(0xe0 | ((codePoint >> 12) & 0x0f));
			utf8 += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f));
			utf8 += static_cast<char>(0x80 | (codePoint & 0x3f));
		}
		else
		{
			utf8 += static_cast<char>(0xf0 | ((codePoint >> 18) & 0x07));
			utf8 += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f));
			utf8 += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f));
			utf8 += static_cast<char>(0x80 | (codePoint & 0x3f));
		}
	}
	return utf8;
}

wstring toUtf16(const string& str)
{
	// This is a simple conversion just to keep this example code simple and dependency free
	// We recommend using a more advanced, performant, and well tested library for real applications

	wstring utf16;
	for (auto it = str.begin(); it != str.end();)
	{
		// Read the next utf8 code point
		uint32_t codePoint = 0;
		if ((*it & 0x80) == 0)
		{
			codePoint = *it & 0x7f;
			++it;
		}
		else if ((*it & 0xe0) == 0xc0)
		{
			codePoint = *it & 0x1f;
			++it;
			codePoint = (codePoint << 6) | (*it & 0x3f);
			++it;
		}
		else if ((*it & 0xf0) == 0xe0)
		{
			codePoint = *it & 0x0f;
			++it;
			codePoint = (codePoint << 6) | (*it & 0x3f);
			++it;
			codePoint = (codePoint << 6) | (*it & 0x3f);
			++it;
		}
		else if ((*it & 0xf8) == 0xf0)
		{
			codePoint = *it & 0x07;
			++it;
			codePoint = (codePoint << 6) | (*it & 0x3f);
			++it;
			codePoint = (codePoint << 6) | (*it & 0x3f);
			++it;
			codePoint = (codePoint << 6) | (*it & 0x3f);
			++it;
		}
		else
		{
			// Invalid utf8, skip it
			++it;
			continue;
		}

		// Write the code point to utf16
		if (codePoint <= 0xffff)
		{
			utf16 += static_cast<wchar_t>(codePoint);
		}
		else
		{
			// Convert to a surrogate pair
			codePoint -= 0x10000;
			utf16 += static_cast<wchar_t>(0xd800 + ((codePoint >> 10) & 0x3ff));
			utf16 += static_cast<wchar_t>(0xdc00 + (codePoint & 0x3ff));
		}
	}
	return utf16;
}

Fove::Quaternion axisAngleToQuat(const float vx, const float vy, const float vz, const float angle)
{
	const float s = sin(angle / 2);
	const float c = cos(angle / 2);
	Fove::Quaternion ret;
	ret.x = vx * s;
	ret.y = vy * s;
	ret.z = vz * s;
	ret.w = c;
	return ret;
}

Fove::Quaternion conjugate(const Fove::Quaternion q)
{
	Fove::Quaternion ret;
	ret.x = -q.x;
	ret.y = -q.y;
	ret.z = -q.z;
	ret.w = q.w;
	return ret;
}

Fove::Matrix44 quatToMatrix(const Fove::Quaternion q)
{
	Fove::Matrix44 ret;
	ret.mat[0][0] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
	ret.mat[0][1] = 2 * q.x * q.y - 2 * q.z * q.w;
	ret.mat[0][2] = 2 * q.x * q.z + 2 * q.y * q.w;
	ret.mat[0][3] = 0;
	ret.mat[1][0] = 2 * q.x * q.y + 2 * q.z * q.w;
	ret.mat[1][1] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
	ret.mat[1][2] = 2 * q.y * q.z - 2 * q.x * q.w;
	ret.mat[1][3] = 0;
	ret.mat[2][0] = 2 * q.x * q.z - 2 * q.y * q.w;
	ret.mat[2][1] = 2 * q.y * q.z + 2 * q.x * q.w;
	ret.mat[2][2] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;
	ret.mat[2][3] = 0;
	ret.mat[3][0] = 0;
	ret.mat[3][1] = 0;
	ret.mat[3][2] = 0;
	ret.mat[3][3] = 1;
	return ret;
}

Fove::Matrix44 transpose(const Fove::Matrix44& m)
{
	Fove::Matrix44 ret;
	ret.mat[0][0] = m.mat[0][0];
	ret.mat[0][1] = m.mat[1][0];
	ret.mat[0][2] = m.mat[2][0];
	ret.mat[0][3] = m.mat[3][0];
	ret.mat[1][0] = m.mat[0][1];
	ret.mat[1][1] = m.mat[1][1];
	ret.mat[1][2] = m.mat[2][1];
	ret.mat[1][3] = m.mat[3][1];
	ret.mat[2][0] = m.mat[0][2];
	ret.mat[2][1] = m.mat[1][2];
	ret.mat[2][2] = m.mat[2][2];
	ret.mat[2][3] = m.mat[3][2];
	ret.mat[3][0] = m.mat[0][3];
	ret.mat[3][1] = m.mat[1][3];
	ret.mat[3][2] = m.mat[2][3];
	ret.mat[3][3] = m.mat[3][3];
	return ret;
}

Fove::Vec3 transformPoint(const Fove::Matrix44& transform, Fove::Vec3 point, float w)
{
	// w is passed separately since we don't have a Vec4 type
	return {
		transform.mat[0][0] * point.x + transform.mat[0][1] * point.y + transform.mat[0][2] * point.z + transform.mat[0][3] * w,
		transform.mat[1][0] * point.x + transform.mat[1][1] * point.y + transform.mat[1][2] * point.z + transform.mat[1][3] * w,
		transform.mat[2][0] * point.x + transform.mat[2][1] * point.y + transform.mat[2][2] * point.z + transform.mat[2][3] * w,
	};
}

Fove::Matrix44 translationMatrix(const float x, const float y, const float z)
{
	Fove::Matrix44 ret;
	ret.mat[0][0] = 1;
	ret.mat[0][1] = 0;
	ret.mat[0][2] = 0;
	ret.mat[0][3] = x;
	ret.mat[1][0] = 0;
	ret.mat[1][1] = 1;
	ret.mat[1][2] = 0;
	ret.mat[1][3] = y;
	ret.mat[2][0] = 0;
	ret.mat[2][1] = 0;
	ret.mat[2][2] = 1;
	ret.mat[2][3] = z;
	ret.mat[3][0] = 0;
	ret.mat[3][1] = 0;
	ret.mat[3][2] = 0;
	ret.mat[3][3] = 1;
	return ret;
}

Fove::Matrix44 operator*(const Fove::Matrix44& m1, const Fove::Matrix44& m2)
{
	Fove::Matrix44 ret;
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

string getErrorString(const ErrorType error) noexcept
try
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
		ret += toUtf8(wstr);

		// Windows seems to add newlines, remove them
		ret.erase(remove_if(ret.begin(), ret.end(), [](const char c) { return c == '\r' || c == '\n'; }), ret.end());
	}
#endif

	return ret;
}
catch (...)
{
	// If we fail in any way generating the error string, return a generic error and move on
	return "ERROR_FAILURE";
}

string getLastErrorAsString() noexcept
{
#ifdef _WIN32
	return getErrorString(GetLastError());
#else
	return 0; // Not implemented for other platforms yet
#endif
}
