#pragma once
#include "FoveAPI.h"
#include "NativeUtil.h"
#include "Util.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

// This header defines a bunch of utility functions for using OpenGL
// These functions makes the implementation of the GL example itself much cleaner, shorter, and safer

// Include the correct GL header for this platform
#ifdef _WIN32
#include <Windows.h>
#include <gl/GL.h>
#include <map>
#elif defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GL/glcorearb.h> // Note: Once we include GL/glcorearb.h, we no longer are allowed to include GL/gl.h nor GL/glext.h
#undef GL_GLEXT_PROTOTYPES
#endif

// On windows we need to take the extra step to define all GL API stuff that we need, above GL 1.1
// This is because Windows does not provide any modern GL headers
// Normally you would use a library that ships the GL3.h header and fetches the functions automatically
// However this example demonstrates how to do it without dependencies
// We only need a small subset of the GL API anyway
#ifdef _WIN32

// This template is instances once per gl function below, generating the needed code to find, check, and call the given GL function
template <typename Return = void, typename... Args>
Return getGLFunc(const char* const funcName, Args... args)
{
	// Query the location of the function on the first call
	static map<const char*, const void*> procLocations;
	const void*& location = procLocations[funcName];
	if (!location)
	{
		location = reinterpret_cast<void*>(wglGetProcAddress(funcName));
		if (!location)
			throw std::runtime_error(std::string("Unable to find ") + funcName);
	}

	// Cast the function
	using Func = Return(APIENTRY*)(Args...);
	Func func = (Func)location;

	return func(args...);
}

typedef char GLchar;
typedef long GLsizeiptr;

#define GL_ARRAY_BUFFER 0x8892
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COMPILE_STATUS 0x8B81
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_FRAMEBUFFER 0x8D40
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#define GL_LINK_STATUS 0x8B82
#define GL_RENDERBUFFER 0x8D41
#define GL_STATIC_DRAW 0x88E4
#define GL_VERTEX_SHADER 0x8B31

inline void glAttachShader(GLuint program, GLuint shader)
{
	getGLFunc("glAttachShader", program, shader);
}
inline void glBindBuffer(GLenum target, GLuint buffer) { getGLFunc("glBindBuffer", target, buffer); }
inline void glBindFramebuffer(GLenum target, GLuint framebuffer) { getGLFunc("glBindFramebuffer", target, framebuffer); }
inline void glBindRenderbuffer(GLenum target, GLuint renderbuffer) { getGLFunc("glBindRenderbuffer", target, renderbuffer); }
inline void glBindVertexArray(GLuint array) { getGLFunc("glBindVertexArray", array); }
inline void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) { getGLFunc("glBufferData", target, size, data, usage); }
inline GLenum glCheckFramebufferStatus(GLenum target) { return getGLFunc<GLenum>("glCheckFramebufferStatus", target); }
inline void glCompileShader(GLuint shader) { getGLFunc("glCompileShader", shader); }
inline GLuint glCreateProgram() { return getGLFunc<GLuint>("glCreateProgram"); }
inline GLuint glCreateShader(GLenum shaderType) { return getGLFunc<GLuint>("glCreateShader", shaderType); }
inline void glDeleteBuffers(GLsizei n, const GLuint* buffers) { getGLFunc("glDeleteBuffers", n, buffers); }
inline void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) { getGLFunc("glDeleteFramebuffers", n, framebuffers); }
inline void glDeleteProgram(GLuint program) { getGLFunc("glDeleteProgram", program); }
inline void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) { getGLFunc("glDeleteRenderbuffers", n, renderbuffers); }
inline void glDeleteShader(GLuint shader) { getGLFunc("glDeleteShader", shader); }
inline void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) { getGLFunc("glDeleteVertexArrays", n, arrays); }
inline void glDetachShader(GLuint program, GLuint shader) { getGLFunc("glDetachShader", program, shader); }
inline void glEnableVertexAttribArray(GLuint index) { getGLFunc("glEnableVertexAttribArray", index); }
inline void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) { getGLFunc("glFramebufferRenderbuffer", target, attachment, renderbuffertarget, renderbuffer); }
inline void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) { getGLFunc("glFramebufferTexture", target, attachment, texture, level); }
inline void glGenBuffers(GLsizei n, GLuint* buffers) { getGLFunc("glGenBuffers", n, buffers); }
inline void glGenFramebuffers(GLsizei n, GLuint* framebuffers) { getGLFunc("glGenFramebuffers", n, framebuffers); }
inline void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) { getGLFunc("glGenRenderbuffers", n, renderbuffers); }
inline void glGenVertexArrays(GLsizei n, GLuint* arrays) { getGLFunc("glGenVertexArrays", n, arrays); }
inline GLint glGetAttribLocation(GLuint program, const GLchar* name) { return getGLFunc<GLint>("glGetAttribLocation", program, name); }
inline void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) { getGLFunc("glGetProgramInfoLog", program, bufSize, length, infoLog); }
inline void glGetProgramiv(GLuint program, GLenum pname, GLint* params) { getGLFunc("glGetProgramiv", program, pname, params); }
inline void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) { getGLFunc("glGetShaderInfoLog", shader, bufSize, length, infoLog); }
inline void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) { getGLFunc("glGetShaderiv", shader, pname, params); }
inline GLint glGetUniformLocation(GLuint program, const GLchar* name) { return getGLFunc<GLint>("glGetUniformLocation", program, name); }
inline void glLinkProgram(GLuint program) { getGLFunc("glLinkProgram", program); }
inline void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) { getGLFunc("glRenderbufferStorage", target, internalformat, width, height); }
inline void glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) { getGLFunc("glShaderSource", shader, count, string, length); }
inline void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { getGLFunc("glUniformMatrix4fv", location, count, transpose, value); }
inline void glUniform1f(GLint location, GLfloat v0) { getGLFunc("glUniform1f", location, v0); }
inline void glUseProgram(GLuint program) { getGLFunc("glUseProgram", program); }
inline void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) { getGLFunc("glVertexAttribPointer", index, size, type, normalized, stride, pointer); }

// WGL
inline const GLubyte* wglGetExtensionsStringEXT() { return getGLFunc<const GLubyte*>("wglGetExtensionsStringEXT"); }
inline HANDLE wglDXOpenDeviceNV(void* dxDevice) { return getGLFunc<HANDLE>("wglDXOpenDeviceNV", dxDevice); }
inline BOOL wglDXCloseDeviceNV(HANDLE hDevice) { return getGLFunc<BOOL>("wglDXCloseDeviceNV", hDevice); }
inline HANDLE wglDXRegisterObjectNV(HANDLE hDevice, void* dxObject, GLuint name, GLenum type, GLenum access) { return getGLFunc<HANDLE>("wglDXRegisterObjectNV", hDevice, dxObject, name, type, access); }
inline BOOL wglDXUnregisterObjectNV(HANDLE hDevice, HANDLE hObject) { return getGLFunc<BOOL>("wglDXUnregisterObjectNV", hDevice, hObject); }
inline BOOL wglDXLockObjectsNV(HANDLE hDevice, GLint count, HANDLE* hObjects) { return getGLFunc<BOOL>("wglDXLockObjectsNV", hDevice, count, hObjects); }
inline BOOL wglDXUnlockObjectsNV(HANDLE hDevice, GLint count, HANDLE* hObjects) { return getGLFunc<BOOL>("wglDXUnlockObjectsNV", hDevice, count, hObjects); }
#define WGL_ACCESS_READ_WRITE_NV 0x0001
#endif

// Returns a c-string to the name of the GL function whos pointer is passed in
// This is used for error display purposes only
const char* glFuncToString(const void* func);

// Checks the current GL error, and throws an exception if it's not GL_NO_ERROR
// Every call into a GL function should be followed by this, so as to easily locate causes of errors
// In production code, this should be a no-op in the release configuration, but for our purposes, we always validate
void glCheckError(const char* function);

// Invokes a GL function, then calls glCheckError (thus throwing if there's an error)
template <typename GlFunc, typename... Args>
auto glCall(const GlFunc func, Args&&... args)
{
	// Use RAII to invoke glCheckError after the func() is invoked below
	// Otherwise we must assign the result of func() to a temporary, check, then return the temporary
	// Doing so would not work for return types, necessatating a void-return specialization of this function
	struct Helper
	{
		Helper(GlFunc func)
			: f(func)
		{
		}
		~Helper() noexcept(false)
		{
			// It's slightly wasteful to look the function name in the success case,
			// but we're more concerned with useful error messages than performance in this example
			glCheckError(glFuncToString((const void*)f));
		}
		const GlFunc f;
	} helper(func);

	return func(std::forward<Args>(args)...);
}

// Creates an opengl contex with the associated window
struct NativeOpenGLContext;
NativeOpenGLContext createOpenGLContext(NativeWindow&);

// Returns the required viewport size to fill the given window
void applyWindowViewport(NativeWindow&, NativeOpenGLContext&);

// Swaps buffers on the given window
void swapBuffers(NativeWindow&, NativeOpenGLContext&);

// Resource type used by the below GlResource class
enum class GlResourceType
{
	Buffer,
	Vao,
	Fbo,
	Texture,
	Shader,
	Program,
	RenderBuffer,
};

// Simple RAII wrapper for any GL resource
template <GlResourceType ResourceType>
struct GlResource
{
public:
	GlResource() {}

	GlResource(GlResource&& other)
		: name_(std::move(other.name_))
	{
	}

	~GlResource()
	{
		clear();
	}

	explicit operator bool() const
	{
		return (bool)name_;
	}

	operator GLuint() const
	{
		if (!name_)
			throw std::runtime_error(std::string("null opengl id"));
		return *name_;
	}

	template <typename... CreateArgs>
	void create(CreateArgs&&... createArgs)
	{
		if (!name_)
		{
			const GLuint name = glCall(GlResourceInfo::GenFunc, std::forward<CreateArgs>(createArgs)...);
			name_ = std::make_unique<GLuint>(name);
		}
	}

	template <typename... BindArgs>
	void createAndBind(BindArgs&&... bindArgs)
	{
		if (!name_)
		{
			const GLuint name = glCall(GlResourceInfo::GenFunc);
			name_ = std::make_unique<GLuint>(name);
		}

		glCall(GlResourceInfo::BindFunc, std::forward<BindArgs>(bindArgs)..., *name_);
	}

	template <typename... BindArgs>
	void bind(BindArgs&&... bindArgs) const
	{
		if (!name_)
			throw std::runtime_error(std::string("null id"));

		glCall(GlResourceInfo::BindFunc, std::forward<BindArgs>(bindArgs)..., *name_);
	}

	void clear()
	try
	{
		if (!name_)
			return;

		const GLuint name = *name_;
		name_.reset();

		glCall(GlResourceInfo::DelFunc, name);
	}
	catch (...)
	{
		// If failing to clear a resource, log an error but consider it non-fatal
		std::cerr << "GlResource error: " << currentExceptionMessage() << std::endl;
	}

	void operator=(GlResource&& resource)
	{
		if (&resource != this)
		{
			clear();
			name_ = std::move(resource.name_);
		}
	}

private:
	std::unique_ptr<GLuint> name_; // Can be an std::optional if using C++17

	struct GlResourceInfo;
};

// Converts a void(GLsizei, GLuint) function (like most of the GL generate functions)
// to a GLuint() style signature that generates just one name instead of an array of them
template <void genFunc(GLsizei, GLuint*)>
GLuint genAdapter()
{
	GLuint name = 0;
	genFunc(1, &name);
	return name;
}

// Similar to genAdapter but for converting a delete function that takes an array
template <void delFunc(GLsizei, const GLuint*)>
void delAdapter(const GLuint name)
{
	delFunc(1, &name);
}

template <>
struct GlResource<GlResourceType::Buffer>::GlResourceInfo
{
	static constexpr auto GenFunc = &genAdapter<&glGenBuffers>;
	static constexpr auto DelFunc = &delAdapter<&glDeleteBuffers>;
	static constexpr auto BindFunc = &glBindBuffer;
};

template <>
struct GlResource<GlResourceType::Vao>::GlResourceInfo
{
	static constexpr auto GenFunc = &genAdapter<&glGenVertexArrays>;
	static constexpr auto DelFunc = &delAdapter<&glDeleteVertexArrays>;
	static constexpr auto BindFunc = &glBindVertexArray;
};

template <>
struct GlResource<GlResourceType::Fbo>::GlResourceInfo
{
	static constexpr auto GenFunc = &genAdapter<&glGenFramebuffers>;
	static constexpr auto DelFunc = &delAdapter<&glDeleteFramebuffers>;
	static constexpr auto BindFunc = &glBindFramebuffer;
};

#ifdef _WIN32

// For some reason MSVC 2017 sometimes gives "cannot deduce type for 'auto' from 'overloaded-function'"
// when taking the address of functions declared with APIENTRY, so we have a quick adapter here

inline void glGenTextures2(GLsizei n, GLuint* textures)
{
	glGenTextures(n, textures);
}

inline void glDeleteTextures2(GLsizei n, const GLuint* textures)
{
	glDeleteTextures(n, textures);
}
#define glGenTextures glGenTextures2
#define glDeleteTextures glDeleteTextures2

#endif

template <>
struct GlResource<GlResourceType::Texture>::GlResourceInfo
{
	static constexpr auto GenFunc = &genAdapter<&glGenTextures>;
	static constexpr auto DelFunc = &delAdapter<&glDeleteTextures>;

	// glBindTexture wrapped to avoid "Error C2131 expression did not evaluate to a constant" on MSVC2017 when using &glBindTexture
	static void glBindWrapper(const GLenum target, const GLuint texture) { glBindTexture(target, texture); }
	static constexpr auto BindFunc = &glBindWrapper;
};

template <>
struct GlResource<GlResourceType::Shader>::GlResourceInfo
{
	static constexpr auto GenFunc = &glCreateShader;
	static constexpr auto DelFunc = &glDeleteShader;
};

template <>
struct GlResource<GlResourceType::Program>::GlResourceInfo
{
	static constexpr auto GenFunc = &glCreateProgram;
	static constexpr auto DelFunc = &glDeleteProgram;
	static constexpr auto BindFunc = &glUseProgram;
};

template <>
struct GlResource<GlResourceType::RenderBuffer>::GlResourceInfo
{
	static constexpr auto GenFunc = &genAdapter<&glGenRenderbuffers>;
	static constexpr auto DelFunc = &delAdapter<&glDeleteRenderbuffers>;
	static constexpr auto BindFunc = &glBindRenderbuffer;
};

// Implementation of NativeOpenGLContext struct
#ifdef _WIN32
struct NativeOpenGLContext
{
};
#elif defined(__APPLE__)
#else
struct NativeOpenGLContext
{
};
#endif
