#pragma once
#include "FoveTypes.h"
#include "IFVRCompositor.h"
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>

// Helper function to throw an exception if the passed error code is not None
inline void CheckError(const Fove::EFVR_ErrorCode code, const char* const data)
{
	if (code != Fove::EFVR_ErrorCode::None)
		throw std::runtime_error("Unable to get " + std::string(data) + ": " + std::to_string(static_cast<int>(code)));
}

// Helper function, which will throw if the result of a Fove call returns error
// Use like this: const auto MyObject = CheckError(Fove->GetSomeObject(...));
// This works on any function that returns an error via a .error field in the return object
template <typename Type>
Type&& CheckError(Type&& object, const char* const data)
{
	CheckError(object.error, data);
	return std::move(object);
}

// Conversions between UTF-8 and UTF-16
std::string ToUtf8(const std::wstring& utf16);
std::wstring ToUtf16(const std::string& str);

// Math utilities
Fove::SFVR_Quaternion AxisAngleToQuat(float vx, float vy, float vz, float angle);
Fove::SFVR_Matrix44 QuatToMatrix(Fove::SFVR_Quaternion q);
Fove::SFVR_Matrix44 Transpose(const Fove::SFVR_Matrix44& m);
Fove::SFVR_Matrix44 TranslationMatrix(float x, float y, float z);
Fove::SFVR_Vec3 TransformPoint(const Fove::SFVR_Matrix44& transform, Fove::SFVR_Vec3 point, float w);
Fove::SFVR_Matrix44 operator*(const Fove::SFVR_Matrix44& m1, const Fove::SFVR_Matrix44& m2);
inline Fove::SFVR_Vec3 operator*(Fove::SFVR_Vec3 v, float scalar) { return { v.x * scalar, v.y * scalar, v.z * scalar }; }
inline Fove::SFVR_Vec3 operator/(Fove::SFVR_Vec3 v, float scalar) { return { v.x / scalar, v.y / scalar, v.z / scalar }; }
inline Fove::SFVR_Vec3 operator+(Fove::SFVR_Vec3 v1, Fove::SFVR_Vec3 v2) { return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z }; }
inline Fove::SFVR_Vec3 operator-(Fove::SFVR_Vec3 v1, Fove::SFVR_Vec3 v2) { return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z }; }
inline float Dot(Fove::SFVR_Vec3 v1, Fove::SFVR_Vec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
inline float MagnitudeSquared(Fove::SFVR_Vec3 v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
inline float Magnitude(Fove::SFVR_Vec3 v) { return std::sqrt(MagnitudeSquared(v)); }
inline float DistanceSquared(Fove::SFVR_Vec3 v1, Fove::SFVR_Vec3 v2) { return MagnitudeSquared(v1 - v2); }
inline Fove::SFVR_Vec3 Normalize(Fove::SFVR_Vec3 v) { return v / Magnitude(v); }
bool RaySphereCollision(Fove::SFVR_Ray ray, Fove::SFVR_Vec3 sphereCenter, float sphereRadius);

// Error utilities
using ErrorType = unsigned long;                      // Equivalent to DWORD on windows
std::string GetErrorString(ErrorType error) noexcept; // Turns an OS-specific error into a string with detailed info if possible
std::string GetLastErrorAsString() noexcept;          // Passes the OS-specific last-error function (GetLastError() on windows) through GetErrorString

// Helper function to change an enum into the underlying value
template <typename EnumType>
typename std::underlying_type<EnumType>::type EnumToUnderlyingValue(const EnumType enumValue)
{
	return static_cast<typename std::underlying_type<EnumType>::type>(enumValue);
}
