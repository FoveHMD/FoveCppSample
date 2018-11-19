#include "Util.h"
#include <algorithm>
#include <codecvt>
#include <iterator>
#include <locale>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

string ToUtf8(const wstring& utf16)
{
	return wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(utf16);
}

wstring ToUtf16(const string& str)
{
	return wstring_convert<codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
}

Fove::Quaternion AxisAngleToQuat(const float vx, const float vy, const float vz, const float angle)
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

Fove::Quaternion Conjugate(const Fove::Quaternion q)
{
	Fove::Quaternion ret;
	ret.x = -q.x;
	ret.y = -q.y;
	ret.z = -q.z;
	ret.w = q.w;
	return ret;
}

Fove::Matrix44 QuatToMatrix(const Fove::Quaternion q)
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

Fove::Matrix44 Transpose(const Fove::Matrix44& m)
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

Fove::Vec3 TransformPoint(const Fove::Matrix44& transform, Fove::Vec3 point, float w)
{
	// w is passed separately since we don't have a Vec4 type
	return {
		transform.mat[0][0] * point.x + transform.mat[0][1] * point.y + transform.mat[0][2] * point.z + transform.mat[0][3] * w,
		transform.mat[1][0] * point.x + transform.mat[1][1] * point.y + transform.mat[1][2] * point.z + transform.mat[1][3] * w,
		transform.mat[2][0] * point.x + transform.mat[2][1] * point.y + transform.mat[2][2] * point.z + transform.mat[2][3] * w,
	};
}

Fove::Matrix44 TranslationMatrix(const float x, const float y, const float z)
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
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			float v = 0;
			for (int i = 0; i < 4; i++)
				v += m1.mat[row][i] * m2.mat[i][column];
			ret.mat[row][column] = v;
		}
	}
	return ret;
}

bool RaySphereCollision(const Fove::Ray ray, const Fove::Vec3 sphereCenter, const float sphereRadius)
{
	// Check if the sphere is behind the ray
	const Fove::Vec3 rayToCenter = sphereCenter - ray.origin;
	const float d = Dot(ray.direction, rayToCenter);
	if (d <= 0)
		return false;

	// Find the closest point on the ray to the sphere
	// This assumes the ray direction is normalized
	const Fove::Vec3 closestPoint = ray.direction * d;

	// Check if the distance to the closest point is within the radius
	// Squared values are used to avoid doing a square root to get the distance
	const float radiusSquared = sphereRadius * sphereRadius;
	const float distanceSquared = DistanceSquared(closestPoint, rayToCenter);
	return distanceSquared <= radiusSquared;
}

string GetErrorString(const ErrorType error) noexcept try {
	string ret = to_string(error);

#ifdef _WIN32
	// Call FormatMessage to get error string from winapi
	LPWSTR _buffer = nullptr;
	const DWORD size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&_buffer, 0, nullptr);
	const unique_ptr<wchar_t, decltype(&LocalFree)> buffer(_buffer, &LocalFree);
	if (size > 0) {
		// Add message to return buffer
		const wstring wstr(buffer.get(), size);
		ret += ' ';
		ret += ToUtf8(wstr);

		// Windows seems to add newlines, remove them
		ret.erase(remove_if(ret.begin(), ret.end(), [](const char c) { return c == '\r' || c == '\n'; }), ret.end());
	}
#endif

	return ret;
} catch (const exception&) {
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
