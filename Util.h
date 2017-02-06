#pragma once
#include <chrono>
#include <string>
#include "FoveTypes.h"
#include "IFVRCompositor.h"

// Helper function, which will throw if the result of a Fove call returns error
// Use like this: const auto MyObject = CheckError(Fove->GetSomeObject(...));
// This works on any function that returns an error via a .error field in the return object
template<typename Type> Type&& CheckError(Type&& object, const char* const data)
{
	if (object.error != Fove::EFVR_ErrorCode::None)
		throw runtime_error("Unable to get "s + std::string(data) + ": " + to_string(static_cast<int>(object.error)));
	return move(object);
}

// Queries the ideal render resolution for a single eye from the compositor
// Since this requires the connection to be made, this will block up to a certain amount of time to retrieve the resolution
// If the timeout elapses, the default value will be returned instead
Fove::SFVR_Vec2i GetSingleEyeResolutionWithTimeout(
	const Fove::IFVRCompositor& compositor,
	std::chrono::milliseconds timeout = std::chrono::milliseconds(500),
	Fove::SFVR_Vec2i defaultValue = { 1280, 1440 });

// Converts a UTF8/ASCII string to UTF16
std::wstring ToUtf16(const std::string& str);

// Math utilities
Fove::SFVR_Quaternion AxisAngleToQuat(float vx, float vy, float vz, float angle);
Fove::SFVR_Matrix44 QuatToMatrix(Fove::SFVR_Quaternion q);
Fove::SFVR_Matrix44 Transpose(const Fove::SFVR_Matrix44& m);
Fove::SFVR_Matrix44 TranslationMatrix(float x, float y, float z);
Fove::SFVR_Matrix44 operator * (const Fove::SFVR_Matrix44& m1, const Fove::SFVR_Matrix44& m2);
