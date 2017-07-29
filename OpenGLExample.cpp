// FOVE DirextX11 Example
// This shows how to display content in a FOVE HMD via the FOVE SDK & OpenGL

#include "IFVRCompositor.h"
#include "IFVRHeadset.h"
#include "Model.h"
#include "NativeUtil.h"
#include "OpenGLUtil.h"
#include "Util.h"
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

// The FOVE compositor currently only accepts DX11 buffers
// So we will use a tiny bit of DX11 code to generate a buffer we can submit,
// then use OpenGL to render to it using the NV_DX_interop/NV_DX_interop2 extensions
#ifdef _WIN32
#define USE_DX_RENDER_BUFFER
#endif
#ifdef USE_DX_RENDER_BUFFER
#include "DXUtil.h"
#include <atlbase.h>
#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")
#endif

// Use std namespace for convenience
using namespace std;

// Vertex format size
constexpr int floatsPerVert = 7;

// Players height above the ground (in meters)
constexpr float playerHeight = 1.6f;

// Main vertex shader source
const char* const vertSrc = "#version 140\n"                                                 // Declare GLSL version
                            "uniform mat4 mvp;\n"                                            // Modelview matrix (updated per-frame)
                            "uniform float selection;\n"                                     // Currently selected object
                            "in vec4 pos;\n"                                                 // Position of the vertex (from the model), 4th element is the object
                            "in vec3 color;\n"                                               // Color of the vertex (from the model)
                            "out vec3 fragColor;\n"                                          // The output color we will pass to the shader
                            "void main(void)\n"                                              // Entry point of the shader
                            "{\n"                                                            //
                            "	gl_Position = mvp * vec4(pos.xyz, 1.0);\n"                   // Transform the position by the modelview matrix
#ifdef USE_DX_RENDER_BUFFER                                                                  //
                            "	FLIP_Y;\n"                                                   // Gets replaced with code to flip the output, see CreateShaderProgram()
#endif                                                                                       //
                            "	float selection = max(0.0, 0.5 - abs(selection - pos.w));\n" // Compute whether this is part of a selected object
                            "	fragColor = color + vec3(selection);\n"                      // Color is simply passed through to frag shader
                            "}";

// Main fragment shader source
const char* const fragSrc = "#version 140\n"                          // Declare GLSL version
                            "in vec3 fragColor;\n"                    // The incoming color from the vertex shader
                            "out vec4 finalColor;\n"                  // The output color
                            "void main(void)\n"                       // Entry point of the shader
                            "{\n"                                     //
                            "	finalColor = vec4(fragColor, 1.0);\n" // Pass the color straight through
                            "}";

// Texture copy vertex shader source
const char* const texCopyVertSrc = "#version 140\n"                           // Declare GLSL version
                                   "in vec2 pos;\n"                           // Position of the vertex
                                   "out vec2 uv;\n"                           // Output texture coordinate of this vertex
                                   "void main(void)\n"                        // Entry point of the shader
                                   "{\n"                                      //
                                   "	gl_Position = vec4(pos, 0.0, 1.0);\n" // Transform the position
#ifdef USE_DX_RENDER_BUFFER                                                   //
                                   "	FLIP_Y;\n"                            // Gets replaced with code to flip the output, see CreateShaderProgram()
#endif                                                                        //
                                   "	uv = pos * 0.5 + 0.5;\n"              // Position goes from -1 to 1, but uvs go from 0 to 1
                                   "}";

// Texture copy fragment shader source
const char* const texCopyFragSrc = "#version 140\n"                       // Declare GLSL version
                                   "uniform sampler2D tex;\n"             // The texture we will read
                                   "in vec2 uv;\n"                        // Texture coordinate from the vertex shader
                                   "out vec4 finalColor;\n"               // The output color
                                   "void main(void)\n"                    // Entry point of the shader
                                   "{\n"                                  //
                                   "	finalColor = texture(tex, uv);\n" // Texture sample is the final color, no transformations
                                   "}";

GlResource<GlResourceType::Program> CreateShaderProgram(const char* vertSrc, const char* const fragSrc)
{
	// Helper function to compile and verify a shader
	const auto Compile = [](const char* source, const GLenum type) {
		// Create the shader object
		GlResource<GlResourceType::Shader> shader;
		shader.Create(type);

		// Attach the shader source
		GlCall(glShaderSource, shader, 1, &source, nullptr);

		// Compile the shader
		GlCall(glCompileShader, shader);

		// Check compilation status
		GLint isCompiled = GL_FALSE;
		GlCall(glGetShaderiv, shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE) {
			// Get the length of the error log
			GLint length = 0;
			GlCall(glGetShaderiv, shader, GL_INFO_LOG_LENGTH, &length);

			// Read the error log
			string log;
			if (length > 0) {
				log.resize(length);
				GlCall(glGetShaderInfoLog, shader, length, &length, &log[0]);

				log.resize(length);
			}

			// Throw an error including the log message
			throw runtime_error("Failed to compile shader: " + log);
		}

		return shader;
	};

#ifdef USE_DX_RENDER_BUFFER
	// Replace code in the vertex shader to flip the image vertically
	// This is because GL/DirectX textures use opposite y directions
	string vertSrcStr = vertSrc;
	const size_t pos = vertSrcStr.find("FLIP_Y");
	if (pos != string::npos)
		vertSrcStr.replace(pos, 6, "gl_Position.y *= -1");
	vertSrc = vertSrcStr.c_str();
#endif

	// Create the vertex and fragment shader via the above helper function
	const GlResource<GlResourceType::Shader> vertShader = Compile(vertSrc, GL_VERTEX_SHADER);
	const GlResource<GlResourceType::Shader> fragShader = Compile(fragSrc, GL_FRAGMENT_SHADER);

	// Link the vertex and fragment shaders into a program
	GlResource<GlResourceType::Program> program;
	program.Create();
	GlCall(glAttachShader, program, vertShader);
	GlCall(glAttachShader, program, fragShader);
	GlCall(glLinkProgram, program);

	// Check if the program linked correctly
	GLint isLinked = GL_FALSE;
	GlCall(glGetProgramiv, program, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE) {
		// Get the length of the error log
		GLint length = 0;
		GlCall(glGetProgramiv, program, GL_INFO_LOG_LENGTH, &length);

		// Read the error log
		string log;
		if (length > 0) {
			log.resize(length);
			GlCall(glGetProgramInfoLog, program, length, &length, &log[0]);

			log.resize(length);
		}

		// Throw an error including the log message
		throw runtime_error("Failed to link shader: " + log);
	}

	// Detatch the shaders are a succesful link (optional)
	GlCall(glDetachShader, program, vertShader);
	GlCall(glDetachShader, program, fragShader);

	// Bind the program to the global gl state
	program.Bind();

	return program;
}

struct RenderSurface {
#ifdef USE_DX_RENDER_BUFFER
	using GLDXHandle = unique_ptr<void, function<void(HANDLE)>>;

	CComPtr<ID3D11Device> device;
	CComPtr<ID3D11DeviceContext> deviceContext;
	CComPtr<ID3D11Texture2D> fboTextureD3D;
	GLDXHandle glDevice;
	GLDXHandle glRenderBuffer;

	// RAII helper to lock and unlock the DX render texture for use w/ OpenGL
	struct D3DSurfaceLock {
		D3DSurfaceLock(const HANDLE d, const HANDLE o)
		    : device(d)
		    , object(o)
		{
			if (device && object && !wglDXLockObjectsNV(device, 1, &object))
				throw runtime_error("Unable to lock DX surface");
		}

		~D3DSurfaceLock() noexcept(false)
		{
			if (device && object && !wglDXUnlockObjectsNV(device, 1, &object))
				throw runtime_error("Unable to unlock DX surface");
		}

	private:
		const HANDLE device;
		HANDLE object;
	};
#endif

	GlResource<GlResourceType::RenderBuffer> depthBuffer; // Z-buffer
	GlResource<GlResourceType::Texture> fboTexture;       // Color buffer that can be used as a texture
	GlResource<GlResourceType::Fbo> fbo;                  // Framebuffer associated with the above two textures
};

RenderSurface GenerateRenderSurface(const Fove::SFVR_Vec2i singleEyeResolution)
{
	RenderSurface ret;

#ifdef USE_DX_RENDER_BUFFER
	// Attempt to create a DX surface to render to
	try {
		// Check for the interop extension
		// We check WGL_NV_DX_interop2 instead of WGL_NV_DX_interop, since the latter is only for D3D9
		const string extensionName = "WGL_NV_DX_interop2";
		const string extensionList = (const char*)GlCall(wglGetExtensionsStringEXT);
		for (size_t pos = 0;; ++pos) {
			pos = extensionList.find(extensionName, pos);
			if (pos == string::npos)
				throw runtime_error(extensionName + " extension not supported");

			// Check that the extension name is not a subset of another name. It must be surrounded by spaces or end of string
			const size_t endChar = pos + extensionName.size();
			if ((pos == 0 || extensionList[pos - 1] == ' ') && (endChar >= extensionList.size() || extensionList[endChar] == ' '))
				break;
		}

		// Create the device and context
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		HRESULT err = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0 /*D3D11_CREATE_DEVICE_DEBUG*/, &featureLevel, 1, D3D11_SDK_VERSION, &ret.device, nullptr, &ret.deviceContext);
		if (FAILED(err) || !ret.device || !ret.deviceContext)
			throw runtime_error("Unable to create device: " + HResultToString(err));

		// Setup description for a D3D texture
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = singleEyeResolution.x * 2;
		desc.Height = singleEyeResolution.y;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		// Create a D3D texture
		err = ret.device->CreateTexture2D(&desc, nullptr, &ret.fboTextureD3D);
		if (FAILED(err))
			throw runtime_error("Unable to create device: " + HResultToString(err));

		// Open the D3D device from the OpenGL side
		ret.glDevice = RenderSurface::GLDXHandle{
			// Open device
			wglDXOpenDeviceNV(ret.device),
			// Close device
			[](HANDLE device) {
			    if (!wglDXCloseDeviceNV(device))
				    cerr << "wglDXCloseDeviceNV:" << GetLastErrorAsString() << endl;
			}
		};
		if (!ret.glDevice)
			throw runtime_error("wglDXOpenDeviceNV failed: " + GetLastErrorAsString());

		// Get the color buffer
		const auto deviceCPtr = ret.glDevice.get(); // For use in the lambda below
		ret.fboTexture.CreateAndBind(GL_TEXTURE_2D);
		ret.glRenderBuffer = RenderSurface::GLDXHandle{
			// Register
			wglDXRegisterObjectNV(ret.glDevice.get(), ret.fboTextureD3D, ret.fboTexture, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV),
			// Deregister
			[deviceCPtr](HANDLE object) {
			    if (!wglDXUnregisterObjectNV(deviceCPtr, object))
				    cerr << "wglDXUnregisterObjectNV:" << GetLastErrorAsString() << endl;
			}
		};
		if (!ret.glRenderBuffer)
			throw runtime_error("wglDXOpenDeviceNV failed: " + GetLastErrorAsString());
	} catch (const exception& e) {
		// If we fail to create a DX surface, we will fall back to rendering to a normal GL surface
		// This will prevent us from submitting to the compositor, but we can still render the scene and take input from the headset
		ShowErrorBox("Unable to create a DX surface: "s + e.what() + "\n\nCompositor submission will be disabled.");

		ret = {}; // Clear out any already initialized D3D resources
	}

	RenderSurface::D3DSurfaceLock lock(ret.glDevice.get(), ret.glRenderBuffer.get());
#endif

	// Create the texture we will render to
	// The left side of this texture will be for the left eye, the right side for the right eye
	// We use a nearest filter since when we submit this texture to the compositor, it will be copied to a texture of equivalent size
	// If you were to draw to a smaller buffer (for performance reasons), linear would be a better choice
	if (!ret.fboTexture) {
		ret.fboTexture.CreateAndBind(GL_TEXTURE_2D);
		GlCall(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, singleEyeResolution.x * 2, singleEyeResolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}
	GlCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GlCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Create the depth buffer
	ret.depthBuffer.CreateAndBind(GL_RENDERBUFFER);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, singleEyeResolution.x * 2, singleEyeResolution.y);

	// Bind the texture & depth buffer to the framebuffer
	ret.fbo.CreateAndBind(GL_FRAMEBUFFER);
	GlCall(glFramebufferTexture, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ret.fboTexture, 0);
	GlCall(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ret.depthBuffer);

	// Check that the framebuffer we created is complete
	// If this fails, it means we've not set up the frame buffers correctly and can't render to it
	if (GL_FRAMEBUFFER_COMPLETE != GlCall(glCheckFramebufferStatus, GL_FRAMEBUFFER))
		throw runtime_error("Framebuffer is incomplete");

	return ret;
}

// Platform-independant main program entry point and loop
// This is invoked from WinMain in WindowsUtil.cpp
void Main(NativeLaunchInfo nativeLaunchInfo) try {
	// Connect to headset
	unique_ptr<Fove::IFVRHeadset> headset{ Fove::GetFVRHeadset() };
	if (!headset)
		throw runtime_error("Unable to create headset connection");
	headset->Initialise(Fove::EFVR_ClientCapabilities::Orientation | Fove::EFVR_ClientCapabilities::Position | Fove::EFVR_ClientCapabilities::Gaze);

	// Connect to compositor
	unique_ptr<Fove::IFVRCompositor> compositor{ Fove::GetFVRCompositor() };
	if (!compositor)
		throw runtime_error("Unable to create compositor connection");

	// Get the rendering resolution from the compositor
	const Fove::SFVR_Vec2i singleEyeResolution = GetSingleEyeResolutionWithTimeout(*compositor);

	// Create a window and setup an OpenGL associated with it
	NativeWindow nativeWindow = CreateNativeWindow(nativeLaunchInfo, "Fove OpenGL Example");
	NativeOpenGLContext nativeOpenGLContext = CreateOpenGLContext(nativeWindow);

	// Set up a framebuffer which we will render to
	RenderSurface renderSurface = GenerateRenderSurface(singleEyeResolution);

	// Create the shaders
	const GlResource<GlResourceType::Program> mainShader = CreateShaderProgram(vertSrc, fragSrc);
	const GlResource<GlResourceType::Program> texCopyShader = CreateShaderProgram(texCopyVertSrc, texCopyFragSrc);

	// Helper function for getting uniform/attrib locations
	// The gl functions glGetUniformLocation/glGetAttribLocation return a signed integer
	// Native indicates that the attribute/uniform name doesn't exist
	// Positive indicates the index of the item, but it must be converted to unsigned before use
	// This function checks for error and then converts to unsigned
	const auto Check = [](const GLint location, const char* const name) {
		if (0 > location)
			throw runtime_error("Unable to find location of "s + name);
		return location;
	};

	// Get data indexes for the shader inputs
	// We will use these to bind data to the shader
	const GLint mvpLoc = Check(GlCall(glGetUniformLocation, mainShader, "mvp"), "mvp");
	const GLint selectionLoc = Check(GlCall(glGetUniformLocation, mainShader, "selection"), "selection");
	const GLuint posLoc = (GLuint)Check(GlCall(glGetAttribLocation, mainShader, "pos"), "pos");
	const GLuint colorLoc = (GLuint)Check(GlCall(glGetAttribLocation, mainShader, "color"), "color");
	const GLuint texCopyPosLoc = (GLuint)Check(GlCall(glGetAttribLocation, texCopyShader, "pos"), "pos");

	// Setup the vertex buffer, uploading our model data to OpenGL (and the GPU)
	const GlResource<GlResourceType::Buffer> vbo = [] {
		GlResource<GlResourceType::Buffer> vbo;
		vbo.CreateAndBind(GL_ARRAY_BUFFER);

		GlCall(glBufferData, GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(levelModelVerts), levelModelVerts, GL_STATIC_DRAW);

		return vbo;
	}();

	// Setup vertex array object
	// This will associate the above buffer data with semantic meaning to the shader
	const GlResource<GlResourceType::Vao> vao = [&] {
		GlResource<GlResourceType::Vao> vao;
		vao.CreateAndBind();

		// Attach the vertex buffer we created to the VAO
		vbo.Bind(GL_ARRAY_BUFFER);

		// Enable usage of the position/color attributes
		GlCall(glEnableVertexAttribArray, posLoc);
		GlCall(glEnableVertexAttribArray, colorLoc);

		// Bind the vertex array
		constexpr size_t stride = sizeof(float) * floatsPerVert;
		GlCall(glVertexAttribPointer, posLoc, 4, GL_FLOAT, GL_FALSE, (GLsizei)stride, (void*)0);
		GlCall(glVertexAttribPointer, colorLoc, 3, GL_FLOAT, GL_FALSE, GLsizei(sizeof(float) * floatsPerVert), (void*)(sizeof(float) * 4));

		return vao;
	}();

	// Setup the vertex buffer, uploading our model data to OpenGL (and the GPU)
	const GlResource<GlResourceType::Buffer> fullscreenQuadVbo = [] {
		GlResource<GlResourceType::Buffer> vbo;
		vbo.CreateAndBind(GL_ARRAY_BUFFER);
		float verts[] = {
			-1.0f, -1.0f,
			1.0f, 1.0f,
			-1.0f, 1.0f,

			-1.0f, -1.0f,
			1.0f, -1.0f,
			1.0f, 1.0f,
		};
		GlCall(glBufferData, GL_ARRAY_BUFFER, (GLsizei)sizeof(verts), verts, GL_STATIC_DRAW);
		return vbo;
	}();

	// Setup vertex array object
	// This will associate the above buffer data with semantic meaning to the shader
	const GlResource<GlResourceType::Vao> fullscreenQuadVao = [&] {
		GlResource<GlResourceType::Vao> vao;
		vao.CreateAndBind();

		// Attach the vertex buffer we created to the VAO
		fullscreenQuadVbo.Bind(GL_ARRAY_BUFFER);

		// Enable usage of the position/color attributes
		GlCall(glEnableVertexAttribArray, texCopyPosLoc);

		// Bind the vertex array
		GlCall(glVertexAttribPointer, texCopyPosLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		return vao;
	}();

	// Main loop
	Fove::SFVR_Matrix44 cameraMatrix; // Stores the camera translation used each frame
	while (true) {
		// Update
		float selection = -1; // Selected model that will be computed each time in the update phase
		{
			if (!FlushWindowEvents(nativeWindow))
				break;

			// Compute selection based on eye gaze
			const Fove::SFVR_GazeConvergenceData convergence = headset->GetGazeConvergence();
			if (Fove::EFVR_ErrorCode::None == convergence.error) {
				// Get the eye ray. The FOVE SDK will return the better of the two eye rays here
				// For now we rely exclusively on the ray. In the future, as convergence distance
				// reporting becomes more accurate, we can use it to distinguish ray-hits by distance
				Fove::SFVR_Ray ray = convergence.ray;

				// Since the convergence ray does not include the headset orienation,
				// or any other transforms we made such as the player height transform,
				// we need to adjust it by the camera matrix from last frame.
				// Since the last-frame cameraMatrix represents what is actually on screen,
				// it's a good indicator of what the user is looking at
				// It is also possible to just recompute it from the old pose, or fetch a new pose.
				//cameraMatrix = TranslationMatrix(0, playerHeight, 0);
				ray.origin = TransformPoint(cameraMatrix, ray.origin, 1);
				ray.direction = TransformPoint(cameraMatrix, ray.direction, 0); // 0 indicates we get rotation but no offset

				// Each selectable model in the scene is represented by a sphere
				// Having a more abstract collision shape than the actual polygon structure helps with accuracy
				// Each sphere is made of 5 floats: selectionid, radius, centerx, centery, centerz
				constexpr size_t numSphereFloats = sizeof(collisionSpheres) / sizeof(float);
				static_assert(numSphereFloats % 5 == 0, "Invalid collision sphere format");
				constexpr size_t numSpheres = numSphereFloats / 5;
				for (size_t i = 0; i < numSpheres; ++i) {
					// Get the parameters of this sphere
					const float selectionid = collisionSpheres[i * 5 + 0];
					const float radius = collisionSpheres[i * 5 + 1];
					const Fove::SFVR_Vec3 center{ collisionSpheres[i * 5 + 2], collisionSpheres[i * 5 + 3], collisionSpheres[i * 5 + 4] };

					// Determine if this sphere intersects
					if (RaySphereCollision(ray, center, radius)) {
						selection = selectionid;
						break; // Break upon the first hit
					}
				}
			}
		}

		// Wait for the compositor to tell us to render
		// This allows the compositor to limit our frame rate to what's appropriate for the HMD display
		// We move directly on to rendering after this, the update phase happens before hand
		// This is to ensure the quickest possible turnaround time from being signaled to presenting a frame,
		// such that we reduce the risk of missing a frame due to time spent during update
		// Note: if the comositor is not running, this will return immediately, and we will run at maximum frame rate
		Fove::SFVR_Pose pose = compositor->WaitForRenderPose();
		if (Fove::EFVR_ErrorCode::None != pose.error) {
			// Use the default pose if we can't fetch the pose
			// This allows us to keep rendering a default posee (eg. while service/compositor not running)
			pose = {};
		}

		// Render the scene
		{
#ifdef USE_DX_RENDER_BUFFER
			RenderSurface::D3DSurfaceLock lock(renderSurface.glDevice.get(), renderSurface.glRenderBuffer.get());
#endif

			// Bind our framebuffer so that we render to a texture
			renderSurface.fbo.Bind(GL_FRAMEBUFFER);

			// Clear the back buffer to a nice sky blue
			GlCall(glClearColor, 0.3f, 0.3f, 0.8f, 0.3f);
			GlCall(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Bind the various state we use for rendering the scene
			mainShader.Bind();
			vao.Bind();
			GlCall(glEnable, GL_DEPTH_TEST);

			// Update selection
			GlCall(glUniform1f, selectionLoc, (GLfloat)selection);

			// Compute the modelview matrix
			// Everything here is reverse since we are moving the world we are going to draw, not the camera
			const Fove::SFVR_Matrix44 modelview = QuatToMatrix(pose.orientation.Conjugate()) // Apply the HMD orientation
			    * TranslationMatrix(-pose.position.x, -pose.position.y, -pose.position.z)    // Apply the position tracking offset
			    * TranslationMatrix(0, -playerHeight, 0);                                    // Move ground downwards to compensate for player height

			// Compute the camera matrix which is the opposite of the modelview
			// This is used for selection in the update cycle
			// We could simply invert the modelview but in this case it's easy enough to create the inverse
			cameraMatrix = QuatToMatrix(pose.orientation) * TranslationMatrix(pose.position.x, pose.position.y, pose.position.z) * TranslationMatrix(0, playerHeight, 0);

			// Get distance between eyes to shift camera for stereo effect
			float halfIOD = 0.064f;
			headset->GetIOD(halfIOD); // Error is ignored, it will use the default value if there's an error

			// Helper function to render the scene
			const auto RenderScene = [&](bool isLeft) {
				// Get the projection matrix for this eye
				// If this errors out, we will just skip rendering (this happens when service is not running, allows us to wait for it to start)
				Fove::SFVR_Matrix44 projection;
				const auto error = headset->GetProjectionMatrixLH(isLeft ? Fove::EFVR_Eye::Left : Fove::EFVR_Eye::Right, 0.01f, 1000.0f, &projection);
				if (Fove::EFVR_ErrorCode::None == error) {
					// Setup the viewport such that we only render to the right/left half of the texture
					glViewport(isLeft ? 0 : singleEyeResolution.x, 0, singleEyeResolution.x, singleEyeResolution.y);

					// Update clip matrix
					Fove::SFVR_Matrix44 mvp = Transpose(projection) * (TranslationMatrix(-halfIOD, 0, 0) * modelview);
					GlCall(glUniformMatrix4fv, mvpLoc, 1, true, (const float*)mvp.mat);

					// Issue draw command
					static constexpr size_t numVerts = sizeof(levelModelVerts) / (sizeof(float) * floatsPerVert);
					GlCall(glDrawArrays, GL_TRIANGLES, 0, (GLsizei)numVerts);
				}
			};

			// Render the scene twice, once for the left, once for the right
			RenderScene(true);
			RenderScene(false);
		}

#ifdef USE_DX_RENDER_BUFFER
		// Present rendered results to compositor
		if (renderSurface.fboTextureD3D) {
			const Fove::SFVR_CompositorTexture tex{ renderSurface.fboTextureD3D };
			Fove::SFVR_TextureBounds bounds;
			bounds.top = 0;
			bounds.bottom = 1;
			bounds.left = 0;
			bounds.right = 0.5f;
			compositor->Submit(Fove::EFVR_Eye::Left, tex, bounds, pose);
			bounds.left = 0.5f;
			bounds.right = 1;
			compositor->Submit(Fove::EFVR_Eye::Right, tex, bounds, pose);
		}
#endif

		// Present the rendered image to the screen
		{
#ifdef USE_DX_RENDER_BUFFER
			RenderSurface::D3DSurfaceLock lock(renderSurface.glDevice.get(), renderSurface.glRenderBuffer.get());
#endif

			// Bind the default framebufer (index 0, that of the window)
			// No glClear is needed since we will fill the whole view
			GlCall(glBindFramebuffer, GL_FRAMEBUFFER, 0);

			// Bind the various state we need to
			ApplyWindowViewport(nativeWindow, nativeOpenGLContext);
			GlCall(glDisable, GL_DEPTH_TEST);
			texCopyShader.Bind();
			fullscreenQuadVao.Bind();
			renderSurface.fboTexture.Bind(GL_TEXTURE_2D);

			// Draw 2 triangles forming a full screen quad
			GlCall(glDrawArrays, GL_TRIANGLES, 0, 6);

			// Swap buffers to display our new frame to the main window
			SwapBuffers(nativeWindow, nativeOpenGLContext);
		}
	}
} catch (const exception& e) {
	// Display any error as a popup box then exit the program
	ShowErrorBox(e.what());
}
