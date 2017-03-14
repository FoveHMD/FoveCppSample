#pragma once
#include <string>
#include <atlbase.h>  // For CComPtr
#include <winerror.h> // For HRESULT

// Returns a string representing the result, with additional details if possible
std::string HResultToString(HRESULT result) noexcept;

// Helpers for binding a single object as an array-of-one to a DX function that requires an array
// This is only suitable for input arrays, not output parameters
template<typename Type> struct InputArrayBinding;
template<typename Type> InputArrayBinding<Type> BindInputArray(const Type object) { return InputArrayBinding<Type>{ object }; }
template<typename Type> InputArrayBinding<Type*> BindInputArray(const CComPtr<Type>& object) { return BindInputArray<Type*>(object); }

// Implementation BindInputArray
template<typename Type>
struct InputArrayBinding
{
	InputArrayBinding(const Type o) : obj(o) {}
	InputArrayBinding(const InputArrayBinding&) = delete;
	InputArrayBinding(InputArrayBinding&& o) : obj(o.obj) {}
	operator const Type*() && { return ptr; } // && forces this to only be used with rvalues/temporaries
private:
	const Type obj;
	const Type* ptr = &obj;
};
