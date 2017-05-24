// FOVE DirextX11 Example
// This shows how to display content in a FOVE HMD via the FOVE SDK & DirectX 11

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <stdexcept>
#include <windows.h>
#include <atlbase.h>
#include <d3d11_1.h>
#include "IFVRHeadset.h"
#include "IFVRCompositor.h"
#include "Util.h"
#include "DXUtil.h"
#include "Model.h"

// Include the compiled shaders
// The .hlsl shader source files are added to the project from CMakeList.txt
// The build process will then compile them into C arrays and generate these header files
#include "Shader.vert_compiled.h"
#include "Shader.frag_compiled.h"

// Use std namespace for convenience
using namespace std;

// Default window size
constexpr int windowSizeX = 1066;
constexpr int windowSizeY = 600;

// Vertex format size
constexpr int floatsPerVert = 7;

// Link the needed DirectX libraries
#pragma comment(lib, "d3d11.lib")

// Handles window messages
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(window, message, wParam, lParam);
}

// Flushes the event queue for the main window
bool FlushWindowEvents()
{
	while (true)
	{
		MSG msg = { 0 };
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				return false;
			}
		}
		else
			return true;
	}
}

// Creates the main window
// Throws if there is any error
HWND InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX windowClass;
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = L"FoveWindowClass";
	windowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	if (!RegisterClassEx(&windowClass))
		throw runtime_error("Unable to register window class: " + GetLastErrorAsString());

	// Create window
	RECT r = { 0, 0, windowSizeX, windowSizeY };
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
	HWND window = CreateWindow(L"FoveWindowClass", L"Fove DirectX11 Example",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr, nullptr, hInstance, nullptr);
	if (!window)
		throw runtime_error("Unable to create window: " + GetLastErrorAsString());

	// Display the window on screen
	ShowWindow(window, nCmdShow);

	return window;
}

CComPtr<ID3D11Device> CreateDevice(CComPtr<ID3D11DeviceContext>& deviceContext)
{
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	CComPtr<ID3D11Device> device;
	const HRESULT err = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0/*D3D11_CREATE_DEVICE_DEBUG*/, &featureLevel, 1, D3D11_SDK_VERSION, &device, nullptr, &deviceContext);
	if (FAILED(err) || !device || !deviceContext)
		throw runtime_error("Unable to create device: " + HResultToString(err));
	return device;
}

CComPtr<IDXGISwapChain> CreateSwapChain(const HWND window, ID3D11Device& device, ID3D11DeviceContext& deviceContext, const Fove::SFVR_Vec2i singleEyeResolution)
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
	HRESULT err = factory->CreateSwapChainForHwnd(&device, window, &swapChainDesc, nullptr, nullptr, &swapChain1);
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
// Currently this draws a ring of triangles around the user
void RenderScene(ID3D11DeviceContext& deviceContext, ID3D11Buffer& constantsBuffer, const Fove::SFVR_Matrix44& projection, const Fove::SFVR_Matrix44& modelview)
{
	// Update clip matrix
	Fove::SFVR_Matrix44 mvp = projection * modelview;
	deviceContext.UpdateSubresource(&constantsBuffer, 0, nullptr, &mvp, 0, 0);

	// Issue draw command
	static constexpr size_t numVerts = sizeof(verts) / (sizeof(float) * floatsPerVert);
	deviceContext.Draw(numVerts, 0);
}

// Main program entry point and loop
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow) try
{
	// Connect to headset
	unique_ptr<Fove::IFVRHeadset> headset{ Fove::GetFVRHeadset() };
	if (!headset)
		throw runtime_error("Unable to create headset connection");
	headset->Initialise(Fove::EFVR_ClientCapabilities::Orientation | Fove::EFVR_ClientCapabilities::Position);

	// Connect to compositor
	unique_ptr<Fove::IFVRCompositor> compositor{ Fove::GetFVRCompositor() };
	if (!compositor)
		throw runtime_error("Unable to create compositor connection");

	// Get the rendering resolution from the compositor
	const Fove::SFVR_Vec2i singleEyeResolution = GetSingleEyeResolutionWithTimeout(*compositor);

	// Setup
	const HWND window = InitWindow(hInstance, nCmdShow);
	CComPtr<ID3D11DeviceContext> deviceContext;
	const CComPtr<ID3D11Device> device = CreateDevice(deviceContext);
	const CComPtr<IDXGISwapChain> swapChain = CreateSwapChain(window, *device, *deviceContext, singleEyeResolution);

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
	descDepth.Width = singleEyeResolution.x * 2;
	descDepth.Height = singleEyeResolution.y;
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
	leftViewport.Width = (FLOAT)singleEyeResolution.x;
	leftViewport.Height = (FLOAT)singleEyeResolution.y;
	leftViewport.MinDepth = 0.0f;
	leftViewport.MaxDepth = 1.0f;
	leftViewport.TopLeftX = 0;
	leftViewport.TopLeftY = 0;
	D3D11_VIEWPORT rightViewport = leftViewport;
	rightViewport.TopLeftX = (FLOAT)singleEyeResolution.x;

	// Create the vertex shader
	CComPtr<ID3D11VertexShader> vertexShader;
	err = device->CreateVertexShader(g_vert, sizeof(g_vert), nullptr, &vertexShader);
	if (FAILED(err) || !vertexShader)
		throw runtime_error("Unable to create vertex shader: " + HResultToString(err));
	deviceContext->VSSetShader(vertexShader, nullptr, 0);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	static_assert(sizeof(verts) % 3 == 0, "Verts array size should be a multiple of 3 (triangles)");
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(verts);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = verts;
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
	constantBufferDesc.ByteWidth = sizeof(float) * 16; // One 4x4 matrix
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	CComPtr<ID3D11Buffer> constantBuffer;
	err = device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(err) || !constantBuffer)
		throw runtime_error("Unable to create constant buffer: " + HResultToString(err));
	deviceContext->VSSetConstantBuffers(0, 1, BindInputArray(constantBuffer));

	// Main loop
	while (true)
	{
		// Update
		if (!FlushWindowEvents())
			break;

		// Wait for the compositor to tell us to render
		// This allows the compositor to limit our frame rate to what's appropriate for the HMD display
		Fove::SFVR_Pose pose = CheckError(compositor->WaitForRenderPose(), "pose");

		// Render
		{
			// Clear the back buffer 
			const float color[] = { 0.3f, 0.3f, 0.8f, 0.3f };
			deviceContext->ClearRenderTargetView(renderTargetView, color);
			deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1, 0);

			// Compute matrices
			Fove::SFVR_Matrix44 displace = TranslationMatrix(0, -1.6f, 0); // Move ground downwards to compensate for player height
			Fove::SFVR_Matrix44 camPosition = TranslationMatrix(-pose.position.x, -pose.position.y, -pose.position.z);
			Fove::SFVR_Matrix44 poseMatrix = QuatToMatrix(pose.orientation.Conjugate());
			Fove::SFVR_Matrix44 modelview = poseMatrix * camPosition * displace;

			// Get distance between eyes to shift camera for stereo effect
			float halfIOD = 0.064f;
			headset->GetIOD(halfIOD); // Error is ignored, it will use the default value if there's an error

			// Render left eye
			Fove::SFVR_Matrix44 leftEyeProj;
			CheckError(headset->GetProjectionMatrixLH(Fove::EFVR_Eye::Left, 0.01f, 1000.0f, &leftEyeProj), "left eye projection matrix");
			deviceContext->RSSetViewports(1, &leftViewport);
			RenderScene(*deviceContext, *constantBuffer, Transpose(leftEyeProj), TranslationMatrix(halfIOD, 0, 0) * modelview);

			// Render right eye
			Fove::SFVR_Matrix44 rightEyeProj;
			CheckError(headset->GetProjectionMatrixLH(Fove::EFVR_Eye::Right, 0.01f, 1000.0f, &rightEyeProj), "right eye projection matrix");
			deviceContext->RSSetViewports(1, &rightViewport);
			RenderScene(*deviceContext, *constantBuffer, Transpose(rightEyeProj), TranslationMatrix(-halfIOD, 0, 0) * modelview);
		}

		// Present rendered results to compositor
		{
			Fove::SFVR_CompositorTexture tex(backBuffer);
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

		// Present the rendered image to the screen
		const HRESULT err = swapChain->Present(0, 0);
		if (FAILED(err))
			throw runtime_error("Unable to present: " + HResultToString(err));
	}

	return 0;
}
catch (const exception &e)
{
	// Display any error as a popup box then exit the program
	MessageBox(0, ToUtf16(e.what()).c_str(), L"Error", MB_OK);
	return -1;
}
