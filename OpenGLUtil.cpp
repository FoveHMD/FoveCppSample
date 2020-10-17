#include "OpenGLUtil.h"
#include "NativeUtil.h"
#include "Util.h"
#include <iostream>
#include <map>

using namespace std;

void GlCheckError(const char* function)
{
	string errorStr;
	const auto append = [&errorStr](string str) {
		if (!errorStr.empty())
			errorStr += " + ";
		errorStr += str;
	};

	// glGetError should be called in a loop until cleared, incase there are multiple errors
	for (bool done = false; !done;) {
		const GLenum error = glGetError();
		switch (error) {
		case GL_NO_ERROR:
			done = true;
			break;
		case GL_INVALID_ENUM:
			append("GL_INVALID_ENUM");
			break;
		case GL_INVALID_VALUE:
			append("GL_INVALID_VALUE");
			break;
		case GL_INVALID_OPERATION:
			append("GL_INVALID_OPERATION");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			append("GL_INVALID_FRAMEBUFFER_OPERATION");
			break;
		case GL_OUT_OF_MEMORY:
			append("GL_OUT_OF_MEMORY");
			break;
		default:
			append(to_string(error));
			break;
		};
	}

	if (!errorStr.empty())
		throw "error in "s + function + ": " + errorStr;
}

const char* GlFuncToString(const void* const func)
{
	static const map<const void*, const char*> functions = {
		{ (const void*)&DelAdapter<&glDeleteBuffers>, "glDeleteBuffers" },
		{ (const void*)&DelAdapter<&glDeleteFramebuffers>, "glDeleteFramebuffers" },
		{ (const void*)&DelAdapter<&glDeleteRenderbuffers>, "glDeleteRenderbuffers" },
		{ (const void*)&DelAdapter<&glDeleteTextures>, "glDeleteTextures" },
		{ (const void*)&DelAdapter<&glDeleteVertexArrays>, "glDeleteVertexArrays" },
		{ (const void*)&GenAdapter<&glGenBuffers>, "glGenBuffers" },
		{ (const void*)&GenAdapter<&glGenFramebuffers>, "glGenFramebuffers" },
		{ (const void*)&GenAdapter<&glGenRenderbuffers>, "glGenRenderbuffers" },
		{ (const void*)&GenAdapter<&glGenTextures>, "glGenTextures" },
		{ (const void*)&GenAdapter<&glGenVertexArrays>, "glGenVertexArrays" },
		{ (const void*)&glAttachShader, "glAttachShader" },
		{ (const void*)&glBindBuffer, "glBindBuffer" },
		{ (const void*)&glBindFramebuffer, "glBindFramebuffer" },
		{ (const void*)&glBindRenderbuffer, "glBindRenderbuffer" },
		{ (const void*)&glBindTexture, "glBindTexture" },
		{ (const void*)&glBindVertexArray, "glBindVertexArray" },
		{ (const void*)&glBufferData, "glBufferData" },
		{ (const void*)&glCheckFramebufferStatus, "glCheckFramebufferStatus" },
		{ (const void*)&glClear, "glClear" },
		{ (const void*)&glClearColor, "glClearColor" },
		{ (const void*)&glCompileShader, "glCompileShader" },
		{ (const void*)&glCreateProgram, "glCreateProgram" },
		{ (const void*)&glCreateShader, "glCreateShader" },
		{ (const void*)&glDeleteProgram, "glDeleteProgram" },
		{ (const void*)&glDeleteShader, "glDeleteShader" },
		{ (const void*)&glDetachShader, "glDetachShader" },
		{ (const void*)&glDisable, "glDisable" },
		{ (const void*)&glDrawArrays, "glDrawArrays" },
		{ (const void*)&glEnable, "glEnable" },
		{ (const void*)&glEnableVertexAttribArray, "glEnableVertexAttribArray" },
		{ (const void*)&glFramebufferRenderbuffer, "glFramebufferRenderbuffer" },
		{ (const void*)&glFramebufferTexture, "glFramebufferTexture" },
		{ (const void*)&glGenVertexArrays, "glGenVertexArrays" },
		{ (const void*)&glGetAttribLocation, "glGetAttribLocation" },
		{ (const void*)&glGetProgramInfoLog, "glGetProgramInfoLog" },
		{ (const void*)&glGetProgramiv, "glGetProgramiv" },
		{ (const void*)&glGetShaderInfoLog, "glGetShaderInfoLog" },
		{ (const void*)&glGetShaderiv, "glGetShaderiv" },
		{ (const void*)&glGetString, "glGetString" },
		{ (const void*)&glGetUniformLocation, "glGetUniformLocation" },
		{ (const void*)&glLinkProgram, "glLinkProgram" },
		{ (const void*)&glShaderSource, "glShaderSource" },
		{ (const void*)&glTexImage2D, "glTexImage2D" },
		{ (const void*)&glTexParameteri, "glTexParameteri" },
		{ (const void*)&glUniform1f, "glUniform1f" },
		{ (const void*)&glUniformMatrix4fv, "glUniformMatrix4fv" },
		{ (const void*)&glUseProgram, "glUseProgram" },
		{ (const void*)&glVertexAttribPointer, "glVertexAttribPointer" },
		{ (const void*)&glViewport, "glViewport" },
	};

	const auto it = functions.find(func);
	return it != functions.end() ? it->second : "(unknown gl function)";
}

NativeOpenGLContext CreateOpenGLContext(NativeWindow& window) try {
	NativeOpenGLContext ret;

#ifdef _WIN32
	// Get the device context
	// This is the GDI+ level context associated with the window for generic windows drawing
	const HDC deviceContext = GetDC(window.window);
	if (!deviceContext)
		throw "Unable to get device context from window";

	// Attribute list
	PIXELFORMATDESCRIPTOR descriptor;
	memset(&descriptor, 0, sizeof(descriptor));
	descriptor.nSize = sizeof(descriptor);
	descriptor.nVersion = 1;
	descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER | PFD_SWAP_LAYER_BUFFERS;
	descriptor.iPixelType = PFD_TYPE_RGBA;
	descriptor.cColorBits = 32;
	descriptor.cRedBits = 8;
	descriptor.cGreenBits = 8;
	descriptor.cBlueBits = 8;
	descriptor.cAlphaBits = 8;

	// Query for a pixel format that fits the attributes we want.
	const int pixelFormat = ChoosePixelFormat(deviceContext, &descriptor);
	if (0 == pixelFormat)
		throw "ChoosePixelFormat: " + GetLastErrorAsString();

	// Set the pixel format on the device context
	if (!SetPixelFormat(deviceContext, pixelFormat, &descriptor))
		throw "SetPixelFormat: " + GetLastErrorAsString();

	// Create the rendering context
	// Typically drivers will give us the latest GL compatibility profile that they support
	// If we want a specific GL version or a core context, we could instead check for and use wglCreateContextAttribsARB extension
	const HGLRC renderContext = wglCreateContext(deviceContext);
	if (!renderContext)
		throw "wglCreateContext: " + GetLastErrorAsString();

	// Make the context current for this thread
	if (!wglMakeCurrent(deviceContext, renderContext))
		throw "wglMakeCurrent: " + GetLastErrorAsString();
#endif

	// Check that our initial OpenGL state has no error
	GlCheckError("InitialGLState");

	// Log the GL version
	const char* const version = (const char*)GlCall(glGetString, GL_VERSION);
	cout << "GL Version is: " << (version ? version : "unknown") << endl;
	return ret;
} catch (...) {
	throw "Unable to create GL context: " + currentExceptionMessage();
}

void ApplyWindowViewport(NativeWindow& window, NativeOpenGLContext& context)
{
#ifdef _WIN32
	// Todo: update this as the window resizes
	glViewport(0, 0, windowSizeX, windowSizeY);
#endif
}

void SwapBuffers(NativeWindow& window, NativeOpenGLContext& context)
{
#ifdef _WIN32
	// Get the device context
	const HDC deviceContext = GetDC(window.window);
	if (!deviceContext)
		throw "Unable to get device context from window";

	// Tell windows to swap buffers
	if (!::SwapBuffers(deviceContext))
		throw "SwapBuffers: " + GetLastErrorAsString();
#endif
}
