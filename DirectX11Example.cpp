// FOVE DirextX11 Example
// This shows how to display content in a FOVE HMD via the FOVE SDK & DirectX 11

#include "DXUtil.h"
#include "IFVRCompositor.h"
#include "IFVRHeadset.h"
#include "Model.h"
#include "NativeUtil.h"
#include "Util.h"
#include <atlbase.h>
#include <chrono>
#include <d3d11_1.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

// Include the compiled shaders
// The .hlsl shader source files are added to the project from CMakeList.txt
// The build process will then compile them into C arrays and generate these header files
#include "Shader.frag_compiled.h"
#include "Shader.vert_compiled.h"

// Use std namespace for convenience
using namespace std;

// Vertex format size
constexpr int floatsPerVert = 7;

// Players height above the ground (in meters)
constexpr float playerHeight = 1.6f;

// Constants buffer for our shader
struct ConstantsBuffer {
	Fove::SFVR_Matrix44 mvp; // Modelview & projection - the complete transform from world coordinates to normalized device coords
	float selection = -1;    // The currently selected object. Each vertex know whats object it's part of and will "light up" if this is equal
};
static_assert(sizeof(ConstantsBuffer) == sizeof(float) * 17, "invalid constant buffer size");

// Link the needed DirectX libraries
#pragma comment(lib, "d3d11.lib")

CComPtr<ID3D11Device> CreateDevice(CComPtr<ID3D11DeviceContext>& deviceContext)
{
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	CComPtr<ID3D11Device> device;
	const HRESULT err = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0 /*D3D11_CREATE_DEVICE_DEBUG*/, &featureLevel, 1, D3D11_SDK_VERSION, &device, nullptr, &deviceContext);
	if (FAILED(err) || !device || !deviceContext)
		throw runtime_error("Unable to create device: " + HResultToString(err));
	return device;
}

CComPtr<IDXGISwapChain> CreateSwapChain(const NativeWindow nativeWindow, ID3D11Device& device, ID3D11DeviceContext& deviceContext, const Fove::SFVR_Vec2i singleEyeResolution)
{
	// Obtain DXGI factory from device
	CComPtr<IDXGIFactory2> factory;
	{
		// Get IDXGIDevice from ID3D11Device
		CComPtr<IDXGIDevice> dxgiDevice;
		HRESULT err = device.QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (FAILED(err) || !dxgiDevice)
			throw runtime_error("Unable to get IDXGIDevice from ID3D11Device: " + HResultToString(err));

		// Get IDXGIAdapter from IDXGIDevice
		CComPtr<IDXGIAdapter> adapter;
		err = dxgiDevice->GetAdapter(&adapter);
		if (FAILED(err) || !adapter)
			throw runtime_error("Unable to get IDXGIAdapter from IDXGIDevice: " + HResultToString(err));

		// Get IDXGIFactory2 from IDXGIAdapter
		err = adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory));
		if (FAILED(err) || !factory)
			throw runtime_error("Unable to get IDXGIFactory2: " + HResultToString(err));
	}

	// Create swap chain description
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.Width = singleEyeResolution.x * 2;
	swapChainDesc.Height = singleEyeResolution.y;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; //D3D11_BIND_RENDER_TARGET/ D3D11_BIND_SHADER_RESOURCE?
	swapChainDesc.BufferCount = 1;

	// Create IDXGISwapChain1 with factory
	CComPtr<IDXGISwapChain1> swapChain1;
	HRESULT err = factory->CreateSwapChainForHwnd(&device, nativeWindow.window, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(err) || !swapChain1)
		throw runtime_error("Unable to create swap chain: " + HResultToString(err));

	// Get IDXGISwapChain from IDXGISwapChain1
	CComPtr<IDXGISwapChain> swapChain;
	err = swapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&swapChain));
	if (FAILED(err) || !swapChain)
		throw runtime_error("Unable to get IDXGISwapChain from IDXGISwapChain1: " + HResultToString(err));

	return swapChain;
}

// Simple function to render some stuff around the player
void RenderScene(ID3D11DeviceContext& deviceContext, ID3D11Buffer& constantsBuffer, const Fove::SFVR_Matrix44& projection, const Fove::SFVR_Matrix44& modelview, const float selection)
{
	// Update clip matrix
	ConstantsBuffer c;
	c.selection = selection;
	c.mvp = projection * modelview;
	deviceContext.UpdateSubresource(&constantsBuffer, 0, nullptr, &c, 0, 0);

	// Issue draw command
	static constexpr size_t numVerts = sizeof(levelModelVerts) / (sizeof(float) * floatsPerVert);
	deviceContext.Draw(numVerts, 0);
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

	// Create a compositor layer, which we will use for submission
	const auto createLayer = [&]() -> unique_ptr<Fove::SFVR_CompositorLayer> {
		Fove::SFVR_CompositorLayer layer;
		const Fove::EFVR_ErrorCode error = compositor->CreateLayer(Fove::SFVR_CompositorLayerCreateInfo(), &layer);
		return error == Fove::EFVR_ErrorCode::None ? make_unique<Fove::SFVR_CompositorLayer>(Fove::SFVR_CompositorLayer(layer)) : nullptr;
	};
	unique_ptr<const Fove::SFVR_CompositorLayer> layer = createLayer();
	Fove::SFVR_Vec2i renderSurfaceSize = layer ? layer->idealResolutionPerEye : Fove::SFVR_Vec2i{ 1024, 1024 };

	// Create a window and setup an DirectX device associated with it
	NativeWindow nativeWindow = CreateNativeWindow(nativeLaunchInfo, "FOVE DirectX11 Example");
	CComPtr<ID3D11DeviceContext> deviceContext;
	const CComPtr<ID3D11Device> device = CreateDevice(deviceContext);
	const CComPtr<IDXGISwapChain> swapChain = CreateSwapChain(nativeWindow, *device, *deviceContext, renderSurfaceSize);

	// Get back buffer
	CComPtr<ID3D11Texture2D> backBuffer;
	HRESULT err = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	if (FAILED(err) || !backBuffer)
		throw runtime_error("Unable to create render target view: " + HResultToString(err));

	// Create a render target view
	CComPtr<ID3D11RenderTargetView> renderTargetView;
	err = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
	if (FAILED(err) || !renderTargetView)
		throw runtime_error("Unable to create render target view: " + HResultToString(err));

	// Create the depth buffer
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = renderSurfaceSize.x * 2;
	descDepth.Height = renderSurfaceSize.y;
	descDepth.MipLevels = 0;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	CComPtr<ID3D11Texture2D> depthBuffer;
	err = device->CreateTexture2D(&descDepth, nullptr, &depthBuffer);
	if (FAILED(err) || !depthBuffer)
		throw runtime_error("Unable to create depth buffer: " + HResultToString(err));

	// Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(dsDesc));
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	CComPtr<ID3D11DepthStencilState> depthStencilState;
	err = device->CreateDepthStencilState(&dsDesc, &depthStencilState);
	if (FAILED(err) || !depthStencilState)
		throw runtime_error("Unable to create depth stencil state: " + HResultToString(err));

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	CComPtr<ID3D11DepthStencilView> depthStencilView;
	err = device->CreateDepthStencilView(depthBuffer, &depthStencilViewDesc, &depthStencilView);
	if (FAILED(err) || !depthStencilView)
		throw runtime_error("Unable to create depth stencil view " + HResultToString(err));

	// Bind render targets to output-merger stage
	deviceContext->OMSetRenderTargets(1, BindInputArray(renderTargetView), depthStencilView);
	deviceContext->OMSetDepthStencilState(depthStencilState, 1);

	// Setup the right and left viewport
	D3D11_VIEWPORT leftViewport;
	leftViewport.Width = (FLOAT)renderSurfaceSize.x;
	leftViewport.Height = (FLOAT)renderSurfaceSize.y;
	leftViewport.MinDepth = 0.0f;
	leftViewport.MaxDepth = 1.0f;
	leftViewport.TopLeftX = 0;
	leftViewport.TopLeftY = 0;
	D3D11_VIEWPORT rightViewport = leftViewport;
	rightViewport.TopLeftX = (FLOAT)renderSurfaceSize.x;

	// Create the vertex shader
	CComPtr<ID3D11VertexShader> vertexShader;
	err = device->CreateVertexShader(g_vert, sizeof(g_vert), nullptr, &vertexShader);
	if (FAILED(err) || !vertexShader)
		throw runtime_error("Unable to create vertex shader: " + HResultToString(err));
	deviceContext->VSSetShader(vertexShader, nullptr, 0);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	CComPtr<ID3D11InputLayout> vertexLayout;
	err = device->CreateInputLayout(layout, ARRAYSIZE(layout), g_vert, sizeof(g_vert), &vertexLayout);
	if (FAILED(err) || !vertexLayout)
		throw runtime_error("Unable to create vertex layout: " + HResultToString(err));
	deviceContext->IASetInputLayout(vertexLayout);

	// Create and set the pixel shader
	CComPtr<ID3D11PixelShader> pixelShader;
	err = device->CreatePixelShader(g_frag, sizeof(g_frag), nullptr, &pixelShader);
	if (FAILED(err) || !pixelShader)
		throw runtime_error("Unable to create pixel shader: " + HResultToString(err));
	deviceContext->PSSetShader(pixelShader, nullptr, 0);

	// Create vertex buffer
	static_assert(sizeof(levelModelVerts) % 3 == 0, "Verts array size should be a multiple of 3 (triangles)");
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(levelModelVerts);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = levelModelVerts;
	CComPtr<ID3D11Buffer> vertexBuffer;
	err = device->CreateBuffer(&vertexBufferDesc, &InitData, &vertexBuffer);
	if (FAILED(err) || !vertexBuffer)
		throw runtime_error("Unable to create vertex buffer: " + HResultToString(err));
	deviceContext->IASetVertexBuffers(0, 1, BindInputArray(vertexBuffer), BindInputArray<UINT>(sizeof(float) * floatsPerVert), BindInputArray<UINT>(0));
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Setup constants buffer
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(float) * 32; // This must be a multiple of 16, as required by DX11
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	CComPtr<ID3D11Buffer> constantBuffer;
	err = device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(err) || !constantBuffer)
		throw runtime_error("Unable to create constant buffer: " + HResultToString(err));
	deviceContext->VSSetConstantBuffers(0, 1, BindInputArray(constantBuffer));

	// Main loop
	Fove::SFVR_Matrix44 cameraMatrix; // Stores the camera translation used each frame
	while (true) {
		// Update
		float selection = -1; // Selected model that will be computed each time in the update phase
		{
			if (!FlushWindowEvents(nativeWindow))
				break;

			// Create layer if we have none
			// This allows us to connect to the compositor once it launches
			if (!layer) {
				// Check if the compositor is ready first. Othewise we will hang for a while when trying to create a layer
				bool isReady = false;
				compositor->IsReady(&isReady); // Error is ignored here - in the case of an error our initial value of false will be kept
				if (isReady) {
					if ((layer = createLayer())) {
						// Todo: resize rendering surface
					}
				}
			}

			// Compute selection based on eye gaze
			Fove::SFVR_GazeConvergenceData convergence;
			const Fove::EFVR_ErrorCode gazeError = headset->GetGazeConvergence(&convergence);
			if (Fove::EFVR_ErrorCode::None == gazeError) {
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
		Fove::SFVR_Pose pose;
		const Fove::EFVR_ErrorCode poseError = compositor->WaitForRenderPose(&pose);
		if (Fove::EFVR_ErrorCode::None != poseError) {
			// If there was an error waiting, it's possible that WaitForRenderPose returned immediately
			// Sleep a little bit to prevent us from rendering at maximum framerate and eating massive resources/battery
			this_thread::sleep_for(10ms);
		}

		// Render the scene
		{
			// Clear the back buffer
			const float color[] = { 0.3f, 0.3f, 0.8f, 0.3f };
			deviceContext->ClearRenderTargetView(renderTargetView, color);
			deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1, 0);

			// Compute the modelview matrix
			// Everything here is reverse since we are moving the world we are going to draw, not the camera
			const Fove::SFVR_Matrix44 modelview = QuatToMatrix(pose.orientation.Conjugate()) * // Apply the HMD orientation
			    TranslationMatrix(-pose.position.x, -pose.position.y, -pose.position.z) *      // Apply the position tracking offset
			    TranslationMatrix(0, -playerHeight, 0);                                        // Move ground downwards to compensate for player height

			// Compute the camera matrix which is the opposite of the modelview
			// This is used for selection in the update cycle
			// We could simply invert the modelview but in this case it's easy enough to create the inverse
			cameraMatrix = QuatToMatrix(pose.orientation) * TranslationMatrix(pose.position.x, pose.position.y, pose.position.z) * TranslationMatrix(0, playerHeight, 0);

			// Get distance between eyes to shift camera for stereo effect
			float halfIOD = 0.064f;
			headset->GetIOD(&halfIOD); // Error is ignored, it will use the default value if there's an error
			halfIOD *= 0.5f;

			// Fetch the projection matrices
			Fove::SFVR_Matrix44 lProjection, rProjection;
			if (Fove::EFVR_ErrorCode::None == headset->GetProjectionMatricesLH(0.01f, 1000.0f, &lProjection, &rProjection)) {
				// Render left eye
				deviceContext->RSSetViewports(1, &leftViewport);
				RenderScene(*deviceContext, *constantBuffer, Transpose(lProjection), TranslationMatrix(halfIOD, 0, 0) * modelview, selection);

				// Render right eye
				deviceContext->RSSetViewports(1, &rightViewport);
				RenderScene(*deviceContext, *constantBuffer, Transpose(rProjection), TranslationMatrix(-halfIOD, 0, 0) * modelview, selection);
			}
		}

		// Present rendered results to compositor
		if (layer) {
			Fove::SFVR_DX11Texture tex{ backBuffer };

			Fove::SFVR_CompositorLayerSubmitInfo submitInfo;
			submitInfo.layerId = layer->layerId;
			submitInfo.pose = pose;
			submitInfo.left.texInfo = &tex;
			submitInfo.right.texInfo = &tex;

			Fove::SFVR_TextureBounds bounds;
			bounds.top = 0;
			bounds.bottom = 1;
			bounds.left = 0;
			bounds.right = 0.5f;
			submitInfo.left.bounds = bounds;
			bounds.left = 0.5f;
			bounds.right = 1;
			submitInfo.right.bounds = bounds;

			compositor->Submit(submitInfo); // Error ignored, just continue rendering to the window when we're disconnected
		}

		// Present the rendered image to the screen
		const HRESULT err = swapChain->Present(0, 0);
		if (FAILED(err))
			throw runtime_error("Unable to present: " + HResultToString(err));
	}
} catch (const exception& e) {
	// Display any error as a popup box then exit the program
	ShowErrorBox(e.what());
}
