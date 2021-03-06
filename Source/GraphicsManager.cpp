#include "stdafx.h"
#include "GraphicsManager.h"
#include "WindowManager.h"

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

#include <iostream>
#include <fstream>
#include "LogManager.h"

LogManager *logger = new LogManager();

IDXGISwapChain *swapchain;  // the pointer to the swap chain interface
ID3D11Device *dev;          // the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;// the pointer to our Direct3D device context

ID3D11RenderTargetView *backbuffer;

int width;
int height;
float deltaTime;
float timeElapsed;

ID3D11VertexShader *vertShader;
ID3D11PixelShader *pixelShader;

struct VERTEX
{
	FLOAT X, Y, Z;      // position
	D3DXCOLOR Color;    // color
};

ID3D11Buffer *vertBuffer;
ID3D11Buffer *constBuffer;
ID3D11InputLayout *pLayout;

ID3D10Blob *vertLog = nullptr;
ID3D10Blob *pixelLog = nullptr;


D3D11_INPUT_ELEMENT_DESC ied[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD0", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

__declspec(align(16))
struct CameraConstants {
	float dTime;
	float time;
};

CameraConstants constants;


GraphicsManager::GraphicsManager(float x, float y)
{
	width = x;
	height = y;
}


GraphicsManager::~GraphicsManager()
{
}

HRESULT GraphicsManager::InitializeGraphics(HWND hWnd)
{
	HRESULT action = InitD3D(hWnd);
	if (FAILED(action))
	{
		return action;
	}

	action = InitShaders(0);
	if (FAILED(action))
	{
		return action;
	}

	action = CreateVertBuffer();
	if (FAILED(action))
	{
		return action;
	}
	
	action = CreateConstBuffer();
	if (FAILED(action))
	{
		return action;
	}
	return action;
}

// this function initializes and prepares Direct3D for use
HRESULT GraphicsManager::InitD3D(HWND hWnd)
{
	logger->Open();
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferDesc.Width = width;
	scd.BufferDesc.Height = height;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = hWnd;                                // the window to be used
	scd.SampleDesc.Count = 4;                               // how many multisamples
	scd.Windowed = 1;                                    // windowed/full-screen mode
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

															// create a device, device context and swap chain using the information in the scd struct
	HRESULT createDevice = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&swapchain,
		&dev,
		NULL,
		&devcon);

	if (FAILED(createDevice))
	{
		return createDevice;
	}

	// get the address of the back buffer
	ID3D11Texture2D *pBackBuffer;
	HRESULT bufferCreated = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	if (FAILED(bufferCreated))
	{
		return bufferCreated;
	}

	// use the back buffer address to create the render target
	HRESULT createTarget = dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	pBackBuffer->Release();

	if (FAILED(createTarget))
	{
		return createTarget;
	}

	// set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = 1920;
	viewport.Height = 1080;

	devcon->RSSetViewports(1, &viewport);

	return S_OK;
}

void GraphicsManager::EndD3D()
{
	swapchain->SetFullscreenState(FALSE, NULL);
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
	vertShader->Release();
	pixelShader->Release();
	//constBuffer->Release();
	//vertBuffer->Release();
}

void GraphicsManager::RenderFrame()
{
	UpdateConstBuffer();
	devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0, 0, 0, 0));

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->IASetVertexBuffers(0, 1, &vertBuffer, &stride, &offset);
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	devcon->Draw(6, 0);

	swapchain->Present(0, 0);
}

HRESULT GraphicsManager::InitShaders(bool clearLog)
{
	if (clearLog)
		logger->Open();
	logger->Append("Compiling shaders...\n");

	ID3D10Blob *VS, *PS;
	D3DX11CompileFromFile(L"shaders.shader", 0, 0, "VShader", "vs_4_0", 0, 0, 0, &VS, &vertLog, 0);
	D3DX11CompileFromFile(L"shaders.shader", 0, 0, "PShader", "ps_4_0", 0, 0, 0, &PS, &pixelLog, 0);
	
	if (vertLog != nullptr || pixelLog != nullptr)
	{
		logger->Append("\nShader compilation failed: \n");
		LPVOID vertPtr = vertLog->GetBufferPointer();

		logger->Append((char*)vertPtr);
		return E_ABORT;
	}
	else if (vertLog == nullptr && pixelLog == nullptr)
	{
		logger->Append("Shader compilation successful.");
	}


	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &vertShader);
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pixelShader);
	devcon->VSSetShader(vertShader, 0, 0);
	devcon->PSSetShader(pixelShader, 0, 0);
	dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
	devcon->IASetInputLayout(pLayout);

	return S_OK;
}

HRESULT GraphicsManager::CreateConstBuffer()
{
	D3D11_BUFFER_DESC cbDesc;

	cbDesc.ByteWidth = sizeof(CameraConstants);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &constants;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	HRESULT res = dev->CreateBuffer(&cbDesc, &InitData, &constBuffer);
	if (FAILED(res))
	{
		logger->Append("\nERROR: Creating constant buffer failed." );
	}
	devcon->VSSetConstantBuffers(0, 1, &constBuffer);
	devcon->PSSetConstantBuffers(0, 1, &constBuffer);

	return res;
}

void GraphicsManager::UpdateConstBuffer()
{
	constants.dTime = deltaTime;
	constants.time = timeElapsed;

	D3D11_MAPPED_SUBRESOURCE resource;
	devcon->Map(constBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, &constants, sizeof(constants));
	devcon->Unmap(constBuffer, 0);
}

HRESULT GraphicsManager::CreateVertBuffer()
{
	VERTEX triVerts[] =
	{
		{ -1, 1, 0.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f) },
		{ 1, 1, 0.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f) },
		{ -1, -1, 0.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f) },
		{ 1, -1, 0.0f, D3DXCOLOR(0.0f, 1.0f, 1.0f, 1.0f) }
	};


	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
	bd.ByteWidth = sizeof(VERTEX) * 4;             // size is the VERTEX struct * 3
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer

	HRESULT action = dev->CreateBuffer(&bd, NULL, &vertBuffer);       // create the buffer

	if (FAILED(action))
	{
		logger->Append("\nERROR: Creating vertex buffer failed.");
		return action;
	}

	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(vertBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);   // map the buffer
	memcpy(ms.pData, triVerts, sizeof(triVerts));                // copy the data
	devcon->Unmap(vertBuffer, NULL);                                     // unmap the buffer

	return action;
}
