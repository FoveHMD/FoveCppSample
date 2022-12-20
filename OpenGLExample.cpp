// FOVE OpenGL Example
// This shows how to display content in a FOVE HMD via the FOVE SDK & OpenGL

#include "FoveAPI.h"
#include "Model.h"
#include "NativeUtil.h"
#include "OpenGLUtil.h"
#include "Util.h"
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

// Use std namespace for convenience
using namespace std;

// The client must use the same GPU as the compositor, which will be the GPU wired to the headset.
// In the DirectX example, we query the compositor for the right GPU, and launch our context on it explicitely.
// However, in OpenGL, this must be done with WGL extensions like WGL_NV_gpu_affinity, and is out of the scope of this example.
// Instead, at least for Optimus systems with both Intel and NVIDIA GPUs, the following export will cause
// the Optimus driver to select the NVIDIA GPU by default, which will always be the correct one for FOVE.
// See https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm for more info.
#ifdef _WIN32
extern "C" _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
#endif

// Vertex format size
constexpr int floatsPerVert = 7;

// Players height above the ground (in meters)
constexpr float playerHeight = 1.6f;

// Main vertex shader source
const char* const demoSceneVertSrc = "#version 140\n"                                                // Declare GLSL version
                                     "uniform mat4 mvp;\n"                                           // Modelview matrix (updated per-frame)
                                     "uniform float selection;\n"                                    // Currently selected object
                                     "in vec4 pos;\n"                                                // Position of the vertex (from the model), 4th element is the object
                                     "in vec3 color;\n"                                              // Color of the vertex (from the model)
                                     "out vec3 fragColor;\n"                                         // The output color we will pass to the shader
                                     "void main(void)\n"                                             // Entry point of the shader
                                     "{\n"                                                           //
                                     "	gl_Position = mvp * vec4(pos.xyz, 1.0);\n"                   // Transform the position by the modelview matrix
                                     "	float selection = max(0.0, 0.5 - abs(selection - pos.w));\n" // Compute whether this is part of a selected object
                                     "	fragColor = color + vec3(selection);\n"                      // Color is simply passed through to frag shader
                                     "}";

// Main fragment shader source
const char* const demoSceneFragSrc = "#version 140\n"                         // Declare GLSL version
                                     "in vec3 fragColor;\n"                   // The incoming color from the vertex shader
                                     "out vec4 finalColor;\n"                 // The output color
                                     "void main(void)\n"                      // Entry point of the shader
                                     "{\n"                                    //
                                     "	finalColor = vec4(fragColor, 1.0);\n" // Pass the color straight through
                                     "}";

// Texture copy vertex shader source
const char* const texCopyVertSrc = "#version 140\n"                           // Declare GLSL version
                                   "in vec2 pos;\n"                           // Position of the vertex
                                   "out vec2 uv;\n"                           // Output texture coordinate of this vertex
                                   "void main(void)\n"                        // Entry point of the shader
                                   "{\n"                                      //
                                   "	gl_Position = vec4(pos, 0.0, 1.0);\n" // Transform the position
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
			throw "Failed to compile shader: " + log;
		}

		return shader;
	};

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
		throw "Failed to link shader: " + log;
	}

	// Detach the shaders after a successful link (optional)
	GlCall(glDetachShader, program, vertShader);
	GlCall(glDetachShader, program, fragShader);

	// Bind the program to the global gl state
	program.Bind();

	return program;
}

struct RenderSurface {
	GlResource<GlResourceType::RenderBuffer> depthBuffer; // Z-buffer
	GlResource<GlResourceType::Texture> fboTexture;       // Color buffer that can be used as a texture
	GlResource<GlResourceType::Fbo> fbo;                  // Framebuffer associated with the above two textures
};

RenderSurface GenerateRenderSurface(const Fove::Vec2i singleEyeResolution)
{
	RenderSurface ret;

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
		throw "Framebuffer is incomplete";

	return ret;
}

// Platform-independent main program entry point and loop
// This is invoked from WinMain in WindowsUtil.cpp
void Main(NativeLaunchInfo nativeLaunchInfo) try {
	// Connect to headset, specifying the capabilities we will use
	Fove::Headset headset = Fove::Headset::create(Fove::ClientCapabilities::OrientationTracking | Fove::ClientCapabilities::PositionTracking | Fove::ClientCapabilities::EyeTracking | Fove::ClientCapabilities::GazedObjectDetection).getValue();

	// Connect to compositor
	Fove::Compositor compositor = headset.createCompositor().getValue();

	// Create a compositor layer, which we will use for submission
	const Fove::CompositorLayerCreateInfo layerCreateInfo; // Using all default values
	Fove::Result<Fove::CompositorLayer> layerOrError = compositor.createLayer(layerCreateInfo);
	Fove::Vec2i renderSurfaceSize = layerOrError ? layerOrError->idealResolutionPerEye : Fove::Vec2i { 1024, 1024 };

	// Create a window and setup an OpenGL associated with it
	NativeWindow nativeWindow = CreateNativeWindow(nativeLaunchInfo, "FOVE OpenGL Example");
	NativeOpenGLContext nativeOpenGLContext = CreateOpenGLContext(nativeWindow);

	// Set up a framebuffer which we will render to
	// If we were unable to create a layer now (eg. compositor not running), use a default size while we wait for the compositor
	RenderSurface renderSurface = GenerateRenderSurface(renderSurfaceSize);

	// Create the shaders
	const GlResource<GlResourceType::Program> mainShader = CreateShaderProgram(demoSceneVertSrc, demoSceneFragSrc);
	const GlResource<GlResourceType::Program> texCopyShader = CreateShaderProgram(texCopyVertSrc, texCopyFragSrc);

	// Helper function for getting uniform/attrib locations
	// The gl functions glGetUniformLocation/glGetAttribLocation return a signed integer
	// Native indicates that the attribute/uniform name doesn't exist
	// Positive indicates the index of the item, but it must be converted to unsigned before use
	// This function checks for error and then converts to unsigned
	const auto Check = [](const GLint location, const char* const name) {
		if (0 > location)
			throw "Unable to find location of "s + name;
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
		constexpr GLsizei stride = static_cast<GLsizei>(sizeof(float) * floatsPerVert);
		GlCall(glVertexAttribPointer, posLoc, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
		GlCall(glVertexAttribPointer, colorLoc, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 4));

		return vao;
	}();

	// Setup the vertex buffer, uploading our model data to OpenGL (and the GPU)
	const GlResource<GlResourceType::Buffer> fullscreenQuadVbo = [] {
		GlResource<GlResourceType::Buffer> vbo;
		vbo.CreateAndBind(GL_ARRAY_BUFFER);
		float verts[] = {
			-1.0f,
			-1.0f,
			1.0f,
			1.0f,
			-1.0f,
			1.0f,

			-1.0f,
			-1.0f,
			1.0f,
			-1.0f,
			1.0f,
			1.0f,
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

	// Register all objects with FOVE SceneAware
	// This allows FOVE to handle all the detection of which object you're looking at
	// Object picking can be done manually if needed, using the gaze vectors
	// However, we recommending using the FOVE API, as the additional scene info can increase the accuracy of ET
	constexpr int cameraId = 9999; // Any arbitrary int not used by the objects
	{
		// Setup camera
		// Posiiton will be updated each frame in the main loop
		Fove::CameraObject cam;
		cam.id = 9999;
		CheckError(headset.registerCameraObject(cam), "registerCameraObject");

		// This can also be done manually if needed, using the gaze vectors,
		// but we recommend using the FOVE API, as the additional scene info can increase the accuracy of ET
		constexpr size_t numSphereFloats = sizeof(collisionSpheres) / sizeof(float);
		static_assert(numSphereFloats % 5 == 0, "Invalid collision sphere format");
		constexpr size_t numSpheres = numSphereFloats / 5;
		for (size_t i = 0; i < numSpheres; ++i) {
			const float selectionid = collisionSpheres[i * 5 + 0];

			Fove::ObjectCollider collider;
			collider.center = Fove::Vec3 { collisionSpheres[i * 5 + 2], collisionSpheres[i * 5 + 3], collisionSpheres[i * 5 + 4] };
			collider.shapeType = Fove::ColliderType::Sphere;
			collider.shapeDefinition.sphere.radius = collisionSpheres[i * 5 + 1];

			Fove::GazableObject object;
			object.colliderCount = 1;
			object.colliders = &collider;
			object.group = Fove::ObjectGroup::Group0; // Groups allows masking of different objects to difference cameras (not needed here)
			object.id = static_cast<int>(collisionSpheres[i * 5 + 0]);
			CheckError(headset.registerGazableObject(object), "registerGazableObject");
		}
	}

	// Main loop
	while (true) {
		// Update
		float selection = -1; // Selected model that will be computed each time in the update phase
		{
			if (!FlushWindowEvents(nativeWindow))
				break;

			// Create layer if we have none
			// This allows us to connect to the compositor once it launches
			if (!layerOrError) {
				// Check if the compositor is ready first. Otherwise we will hang for a while when trying to create a layer
				Fove::Result<bool> isReadyOrError = compositor.isReady();
				if (isReadyOrError.isValid() && isReadyOrError.getValue()) {
					if ((layerOrError = compositor.createLayer(layerCreateInfo)).isValid()) {
						// Todo: resize rendering surface
					}
				}
			}

			// Determine the selection object based on what's being gazed at
			headset.fetchEyeTrackingData();
			if (const Fove::Result<int> gazeOrError = headset.getGazedObjectId())
				if (gazeOrError.getValue() != fove_ObjectIdInvalid)
					selection = static_cast<float>(gazeOrError.getValue());
		}

		// Wait for the compositor to tell us to render
		// This allows the compositor to limit our frame rate to what's appropriate for the HMD display
		// We move directly on to rendering after this, the update phase happens before hand
		// This is to ensure the quickest possible turnaround time from being signaled to presenting a frame,
		// such that we reduce the risk of missing a frame due to time spent during update
		const Fove::Result<Fove::Pose> poseOrError = compositor.waitForRenderPose();
		const Fove::Pose pose = poseOrError.isValid() ? poseOrError.getValue() : Fove::Pose();
		if (!poseOrError.isValid()) {
			// If there was an error waiting, it's possible that WaitForRenderPose returned immediately
			// Sleep a little bit to prevent us from rendering at maximum framerate and eating massive resources/battery
			this_thread::sleep_for(10ms);
		}

		// Render the scene
		{
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
			const Fove::Matrix44 modelview = QuatToMatrix(Conjugate(pose.orientation))    // Apply the HMD orientation
			    * TranslationMatrix(-pose.position.x, -pose.position.y, -pose.position.z) // Apply the position tracking offset
			    * TranslationMatrix(0, -playerHeight, 0);                                 // Move ground downwards to compensate for player height

			// Get distance between eyes to shift camera for stereo effect
			const Fove::Result<float> iodOrError = headset.getRenderIOD();
			const float halfIOD = 0.5f * (iodOrError.isValid() ? iodOrError.getValue() : 0.064f);

			// Fetch the projection matrices
			Fove::Result<Fove::Stereo<Fove::Matrix44>> projectionsOrError = headset.getProjectionMatricesLH(0.01f, 1000.0f);
			if (projectionsOrError.isValid()) {
				// Helper function to render the scene
				const auto RenderScene = [&](bool isLeft) {
					// Setup the viewport such that we only render to the right/left half of the texture
					glViewport(isLeft ? 0 : renderSurfaceSize.x, 0, renderSurfaceSize.x, renderSurfaceSize.y);

					// Update clip matrix
					Fove::Matrix44 mvp = Transpose(isLeft ? projectionsOrError->l : projectionsOrError->r) * (TranslationMatrix(isLeft ? halfIOD : -halfIOD, 0, 0) * modelview);
					GlCall(glUniformMatrix4fv, mvpLoc, 1, true, (const float*)mvp.mat);

					// Issue draw command
					static constexpr size_t numVerts = sizeof(levelModelVerts) / (sizeof(float) * floatsPerVert);
					GlCall(glDrawArrays, GL_TRIANGLES, 0, (GLsizei)numVerts);
				};

				// Render the scene twice, once for the left, once for the right
				RenderScene(true);
				RenderScene(false);
			}
		}

		// Present rendered results to compositor
		if (layerOrError && renderSurface.fboTexture) {
			const Fove::GLTexture tex { (GLuint)renderSurface.fboTexture };

			Fove::CompositorLayerSubmitInfo submitInfo;
			submitInfo.layerId = layerOrError->layerId;
			submitInfo.pose = pose;
			submitInfo.left.texInfo = &tex;
			submitInfo.right.texInfo = &tex;

			Fove::TextureBounds bounds;
			bounds.top = 0;
			bounds.bottom = 1;
			bounds.left = 0;
			bounds.right = 0.5f;
			submitInfo.left.bounds = bounds;
			bounds.left = 0.5f;
			bounds.right = 1;
			submitInfo.right.bounds = bounds;

			compositor.submit(submitInfo); // Error ignored, just continue rendering to the window when we're disconnected
		}

		// Present the rendered image to the screen
		{
			// Bind the default framebuffer (index 0, that of the window)
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

		// Update camera position used by FOVE gaze detection
		Fove::ObjectPose camPose;
		camPose.position = pose.position;
		camPose.position.y += playerHeight;
		camPose.velocity = pose.velocity;
		camPose.rotation = pose.orientation;
		CheckError(headset.updateCameraObject(cameraId, camPose), "updateCameraObject");
	}
} catch (...) {
	// Display any error as a popup box then exit the program
	ShowErrorBox("Error: " + currentExceptionMessage());
}
