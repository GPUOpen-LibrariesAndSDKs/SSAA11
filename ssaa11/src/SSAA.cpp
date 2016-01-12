//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "SSAA.h"
#include "../../DXUT/Optional/DXUTcamera.h"
#include "../../DXUT/Optional/SDKMesh.h"
#include "../../DXUT/Optional/SDKmisc.h"
#include "../../DXUT/Core/DDSTextureLoader.h"
#include "../../AMD_SDK/inc/AMD_SDK.h"


DirectX::XMVECTOR gSunDir = DirectX::XMVectorSet( -0.5f, -0.2f, 0.5f, 0.0f );
DirectX::XMVECTOR gSpotLookAt = DirectX::XMVectorSet( 2.4f, 430.0f, 336.0f, 1.0f );
DirectX::XMVECTOR gSpotPos[] = 
{
	DirectX::XMVectorSet( -386, 176.0f, -166.0f, 1.0f ),
	DirectX::XMVectorSet( 191.0f, 55.0f, -356.0f, 1.0f ),
	DirectX::XMVectorSet( 459.0f, 24.0f, 187.0f, 1.0f )
};
DirectX::XMVECTOR gSpotColor[] = 
{
	DirectX::XMVectorSet( 0.6f, 0.6f, 2.0f, 1.0f ),
	DirectX::XMVectorSet( 1.5f, 0.3f, 0.3f, 1.0f ),
	DirectX::XMVectorSet( 0.2f, 1.7f, 0.2f, 1.0f )
};

// Global scene constant buffer - updated once per frame
struct SceneConstantBuffer
{
	DirectX::XMMATRIX	m_WorldViewProj;
	DirectX::XMMATRIX	m_World;
	DirectX::XMVECTOR	m_EyePosition;

	DirectX::XMVECTOR	m_SunDirection;
	DirectX::XMVECTOR	m_SunColor;
	DirectX::XMVECTOR	m_AmbientColor;

	DirectX::XMVECTOR	m_SpotPositionAndRadius[ 3 ];
	DirectX::XMVECTOR	m_SpotDirectionAndAngle[ 3 ];
	DirectX::XMVECTOR	m_SpotColor[ 3 ];
};

// Quad blit constant buffer
struct QuadConstantBuffer
{
	DirectX::XMVECTOR	m_Dimensions;
};

// Quad blit vertex
struct QuadVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
};


SSAA::SSAA() :
	m_AntiAliasingType( None ),
	m_ResolutionMultiplierX( 1.0f ),
	m_ResolutionMultiplierY( 1.0f ),
	m_Format( Fmt8x4 ),
	m_Scene( TypicalScene ),
	m_Camera( 0 ),
	m_Device( 0 ),
	m_ImmediateContext( 0 ),
	m_SceneConstantBuffer( 0 ),
	m_QuadConstantBuffer( 0 ),
	m_SceneDepthStencilState( 0 ),
	m_QuadDepthStencilState( 0 ),
	m_OpaqueState( 0 ),
	m_RasterStateCullBack( 0 ),
	m_RasterStateCullFront( 0 ),
	m_SceneMesh( 0 ),
	m_SceneInputLayout( 0 ),
	m_SceneVS( 0 ),
	m_ScenePS( 0 ),
	m_SceneSampleFrequencyPS( 0 ),
	m_StressTestInputLayout( 0 ),
	m_StressTestVB( 0 ),
	m_StressTestIB( 0 ),
	m_StressTestVS( 0 ),
	m_StressTestPS( 0 ),
	m_StressTestSampleFrequencyPS( 0 ),
	m_StressTestTexture( 0 ),
	m_QuadInputLayout( 0 ),
	m_QuadVS( 0 ),
	m_QuadNormalPS( 0 ),
	m_Quad2x2RGPS( 0 ),
	m_QuadVB( 0 ),
	m_QuadSampler( 0 ),
	m_DestinationTexture( 0 ),
	m_MultisampledSurface( 0 ),
	m_RenderTargetView( 0 ),
	m_DestinationTextureSRV( 0 ),
	m_DepthTexture( 0 ),
	m_DepthView( 0 )
{
	ZeroMemory( m_SceneSamplers, sizeof( m_SceneSamplers ) );
}


SSAA::~SSAA()
{

}


// Setting the AA type will recreate the render target with the appropriate size and sample level
void SSAA::SetAAType( Type type )
{
	if ( m_AntiAliasingType != type && type >= None && type < Max )
	{
		m_AntiAliasingType = type;
	
		switch ( m_AntiAliasingType )
		{
			case SSAAx2H: m_ResolutionMultiplierX = 2.0f; m_ResolutionMultiplierY = 1.0f; break;
			case SSAAx2V: m_ResolutionMultiplierX = 1.0f; m_ResolutionMultiplierY = 2.0f; break;
			case SSAAx15: m_ResolutionMultiplierX = 1.5f; m_ResolutionMultiplierY = 1.5f; break;
			case SSAAx4: case SSAAx4RG: m_ResolutionMultiplierX = 2.0f; m_ResolutionMultiplierY = 2.0f; break;
			default: m_ResolutionMultiplierX = 1.0f; m_ResolutionMultiplierY = 1.0f; break;
		}

		// Re-alloc render target
		CreateRenderTargets();

		// Update the description string
		UpdateDescription();
	}
}


// Setting the render target format will recreate the render target
void SSAA::SetRenderTargetFormat( RenderTargetFormat format )
{
	m_Format = format;

	// Re-alloc render target
	CreateRenderTargets();

	// Update the description string
	UpdateDescription();
}


// Set which scene to view
void SSAA::SetScene( SceneType scene )
{
	m_Scene = scene;
}



// Init to be called when new D3D device is created
void SSAA::Init( ID3D11Device* device, ID3D11DeviceContext* immediateContext, CDXUTSDKMesh* sceneMesh, const CFirstPersonCamera& camera )
{
	m_Device = device;
	m_ImmediateContext = immediateContext;
	m_SceneMesh = sceneMesh;
	m_Camera = &camera;

	ID3DBlob* Blob = 0;
	HRESULT hr = S_OK;

	// Create depth stencil states
	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
	DepthStencilDesc.DepthEnable = TRUE; 
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; 
	DepthStencilDesc.StencilEnable = FALSE; 
	DepthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK; 
	DepthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK; 
	V( m_Device->CreateDepthStencilState( &DepthStencilDesc, &m_SceneDepthStencilState ) );

	DepthStencilDesc.DepthEnable = FALSE; 
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	V( m_Device->CreateDepthStencilState( &DepthStencilDesc, &m_QuadDepthStencilState ) );
	
	// Create shaders
	V( AMD::CompileShaderFromFile( L"../src/Shaders/Scene.hlsl", "PSMainBump", "ps_5_0", &Blob, 0 ) );
	V( m_Device->CreatePixelShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_ScenePS ) );
	SAFE_RELEASE( Blob );

	D3D10_SHADER_MACRO defines[] = 
	{
		"PER_SAMPLE_FREQUENCY", "",
		0, 0
	};
	V( AMD::CompileShaderFromFile( L"../src/Shaders/Scene.hlsl", "PSMainBump", "ps_5_0", &Blob, defines ) );
	V( m_Device->CreatePixelShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_SceneSampleFrequencyPS ) );
	SAFE_RELEASE( Blob );

	V( AMD::CompileShaderFromFile( L"../src/Shaders/Scene.hlsl", "VSMain", "vs_5_0", &Blob, 0 ) );
	V( m_Device->CreateVertexShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_SceneVS ) );

	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,		0,  0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R10G10B10A2_UNORM,	0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R16G16_FLOAT,			0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",   0, DXGI_FORMAT_R10G10B10A2_UNORM,	0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
    
	V( m_Device->CreateInputLayout( layout, ARRAYSIZE( layout ), Blob->GetBufferPointer(), Blob->GetBufferSize(), &m_SceneInputLayout ) );
	
	SAFE_RELEASE( Blob );

	V( AMD::CompileShaderFromFile( L"../src/Shaders/Scene.hlsl", "PSMain2", "ps_5_0", &Blob, 0 ) );
	V( m_Device->CreatePixelShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_StressTestPS ) );
	SAFE_RELEASE( Blob );

	V( AMD::CompileShaderFromFile( L"../src/Shaders/Scene.hlsl", "PSMain2", "ps_5_0", &Blob, defines ) );
	V( m_Device->CreatePixelShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_StressTestSampleFrequencyPS ) );
	SAFE_RELEASE( Blob );

	V( AMD::CompileShaderFromFile( L"../src/Shaders/Scene.hlsl", "VSMain2", "vs_5_0", &Blob, 0 ) );
	V( m_Device->CreateVertexShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_StressTestVS ) );

	const D3D11_INPUT_ELEMENT_DESC layout2[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R16G16_FLOAT,			0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	V( m_Device->CreateInputLayout( layout2, ARRAYSIZE( layout2 ), Blob->GetBufferPointer(), Blob->GetBufferSize(), &m_StressTestInputLayout ) );
	
	SAFE_RELEASE( Blob );
	
	V( AMD::CompileShaderFromFile( L"../src/Shaders/Quad.hlsl", "PSMain", "ps_5_0", &Blob, 0 ) );
	V( m_Device->CreatePixelShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_QuadNormalPS ) );
	SAFE_RELEASE( Blob );

	V( AMD::CompileShaderFromFile( L"../src/Shaders/Quad.hlsl", "PSMain2x2RG", "ps_5_0", &Blob, 0 ) );
	V( m_Device->CreatePixelShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_Quad2x2RGPS ) );
	SAFE_RELEASE( Blob );

	V( AMD::CompileShaderFromFile( L"../src/Shaders/Quad.hlsl", "VSMain", "vs_5_0", &Blob, 0 ) );
	V( m_Device->CreateVertexShader( Blob->GetBufferPointer(), Blob->GetBufferSize(), 0, &m_QuadVS ) );

	const D3D11_INPUT_ELEMENT_DESC QuadLayout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
    
	V( m_Device->CreateInputLayout( QuadLayout, ARRAYSIZE( QuadLayout ), Blob->GetBufferPointer(), Blob->GetBufferSize(), &m_QuadInputLayout ) );
	
	SAFE_RELEASE( Blob );

	// Create cube geometry for stress test scene
	AMD::CreateCube( 1.0f, &m_StressTestVB, &m_StressTestIB );

	// Create Quad blit vertex buffer
	QuadVertex Verts[ 12 ];

	Verts[ 0 ].pos = DirectX::XMFLOAT3( -1.0f, -1.0f, 0.5f );
	Verts[ 0 ].uv = DirectX::XMFLOAT2( 0.0f, 1.0f );
	Verts[ 1 ].pos = DirectX::XMFLOAT3( -1.0f, 1.0f, 0.5f );
	Verts[ 1 ].uv = DirectX::XMFLOAT2( 0.0f, 0.0f );
	Verts[ 2 ].pos = DirectX::XMFLOAT3( 1.0f, -1.0f, 0.5f );
	Verts[ 2 ].uv = DirectX::XMFLOAT2( 1.0f, 1.0f );
	Verts[ 3 ].pos = DirectX::XMFLOAT3( -1.0f, 1.0f, 0.5f );
	Verts[ 3 ].uv = DirectX::XMFLOAT2( 0.0f, 0.0f );
	Verts[ 4 ].pos = DirectX::XMFLOAT3( 1.0f, 1.0f, 0.5f );
	Verts[ 4 ].uv = DirectX::XMFLOAT2( 1.0f, 0.0f );
	Verts[ 5 ].pos = DirectX::XMFLOAT3( 1.0f, -1.0f, 0.5f );
	Verts[ 5 ].uv = DirectX::XMFLOAT2( 1.0f, 1.0f );

	// Verts set up to blit the scene upside down
	Verts[ 6 ].pos = DirectX::XMFLOAT3( -1.0f, -1.0f, 0.5f );
	Verts[ 6 ].uv = DirectX::XMFLOAT2( 0.0f, 0.0f );
	Verts[ 7 ].pos = DirectX::XMFLOAT3( -1.0f, 1.0f, 0.5f );
	Verts[ 7 ].uv = DirectX::XMFLOAT2( 0.0f, 1.0f );
	Verts[ 8 ].pos = DirectX::XMFLOAT3( 1.0f, -1.0f, 0.5f );
	Verts[ 8 ].uv = DirectX::XMFLOAT2( 1.0f, 0.0f );
	Verts[ 9 ].pos = DirectX::XMFLOAT3( -1.0f, 1.0f, 0.5f );
	Verts[ 9 ].uv = DirectX::XMFLOAT2( 0.0f, 1.0f );
	Verts[ 10 ].pos = DirectX::XMFLOAT3( 1.0f, 1.0f, 0.5f );
	Verts[ 10 ].uv = DirectX::XMFLOAT2( 1.0f, 1.0f );
	Verts[ 11 ].pos = DirectX::XMFLOAT3( 1.0f, -1.0f, 0.5f );
	Verts[ 11 ].uv = DirectX::XMFLOAT2( 1.0f, 0.0f );

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.ByteWidth = sizeof( Verts );
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = Verts;
	V( m_Device->CreateBuffer( &BufferDesc, &InitData, &m_QuadVB ) );

	// Create the scene's global constant buffer
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.ByteWidth = sizeof( SceneConstantBuffer );
	V( m_Device->CreateBuffer( &BufferDesc, 0, &m_SceneConstantBuffer ) );
	
	// Create the quad blit constant buffer
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.ByteWidth = sizeof( QuadConstantBuffer );
	V( m_Device->CreateBuffer( &BufferDesc, 0, &m_QuadConstantBuffer ) );
	
	// Create the samplers
	D3D11_SAMPLER_DESC SamplerDesc;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.MipLODBias = 0.0f;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 0;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	V( m_Device->CreateSamplerState( &SamplerDesc, &m_QuadSampler ) );

	for ( int i = 0; i < NumBiasLevels; i++ )
	{
		if ( i == MinusOne )
			SamplerDesc.MipLODBias = -1.0f;
		else if ( i == MinusOneAndAHalf )
			SamplerDesc.MipLODBias = -1.5f;

		SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		SamplerDesc.MaxAnisotropy = 1;
		V( m_Device->CreateSamplerState( &SamplerDesc, &m_SceneSamplers[ i ].m_PointSampler ) );

		SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		SamplerDesc.MaxAnisotropy = 2;
		V( m_Device->CreateSamplerState( &SamplerDesc, &m_SceneSamplers[ i ].m_AnisoSampler ) );
	}

	// Create the blend state
	D3D11_BLEND_DESC BlendStateDesc;
	BlendStateDesc.AlphaToCoverageEnable = FALSE;
	BlendStateDesc.IndependentBlendEnable = FALSE;
	BlendStateDesc.RenderTarget[0].BlendEnable = FALSE;
	BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; 
	BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; 
	BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA; 
	BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA; 
	BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	V( m_Device->CreateBlendState( &BlendStateDesc, &m_OpaqueState ) );

	// Create the front and back cull states
	D3D11_RASTERIZER_DESC RasterDesc;
	RasterDesc.FillMode = D3D11_FILL_SOLID;
	RasterDesc.CullMode = D3D11_CULL_BACK;
    RasterDesc.FrontCounterClockwise = FALSE;
    RasterDesc.DepthBias = 0;
    RasterDesc.DepthBiasClamp = 0.0f;
    RasterDesc.SlopeScaledDepthBias = 0.0f;
    RasterDesc.DepthClipEnable = TRUE;
    RasterDesc.ScissorEnable = FALSE;
    RasterDesc.MultisampleEnable = TRUE;
    RasterDesc.AntialiasedLineEnable = FALSE;        
	V( m_Device->CreateRasterizerState( &RasterDesc, &m_RasterStateCullBack ) );

	RasterDesc.CullMode = D3D11_CULL_FRONT;
	V( m_Device->CreateRasterizerState( &RasterDesc, &m_RasterStateCullFront ) );

	// Load the texture for the alpha tested stress test scene
	V( DirectX::CreateDDSTextureFromFile( m_Device, L"..\\Media\\StressTest.dds", nullptr, &m_StressTestTexture ) );
}


void SSAA::DeInit()
{
	DestroyRenderTargets();

	for ( int i = 0; i < NumBiasLevels; i++ )
	{
		SAFE_RELEASE( m_SceneSamplers[ i ].m_PointSampler );
		SAFE_RELEASE( m_SceneSamplers[ i ].m_AnisoSampler );
	}
	
	SAFE_RELEASE( m_SceneConstantBuffer );
	SAFE_RELEASE( m_QuadConstantBuffer );

	SAFE_RELEASE( m_SceneInputLayout );
	SAFE_RELEASE( m_SceneVS );
	SAFE_RELEASE( m_ScenePS );
	SAFE_RELEASE( m_SceneSampleFrequencyPS );

	SAFE_RELEASE( m_StressTestInputLayout );
	SAFE_RELEASE( m_StressTestVB );
	SAFE_RELEASE( m_StressTestIB );
	SAFE_RELEASE( m_StressTestVS );
	SAFE_RELEASE( m_StressTestPS );
	SAFE_RELEASE( m_StressTestSampleFrequencyPS );
	SAFE_RELEASE( m_StressTestTexture );

	SAFE_RELEASE( m_QuadSampler );
	SAFE_RELEASE( m_QuadVB );
	SAFE_RELEASE( m_QuadInputLayout );
	SAFE_RELEASE( m_QuadVS );
	SAFE_RELEASE( m_QuadNormalPS );
	SAFE_RELEASE( m_Quad2x2RGPS );
	
	SAFE_RELEASE( m_QuadDepthStencilState );
	SAFE_RELEASE( m_SceneDepthStencilState );

	SAFE_RELEASE( m_RasterStateCullBack );
	SAFE_RELEASE( m_RasterStateCullFront );
	SAFE_RELEASE( m_OpaqueState );
}


// Handle resizing the scene. The Intermediate target needs resizing
void SSAA::OnResize( int width, int height )
{
	m_Width = width;
	m_Height = height;
	CreateRenderTargets();
	UpdateDescription();
}


// Render the scene.
// Back buffer and backbuffer depth stencil views are passed in from the application.
void SSAA::Render( ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv )
{
	TIMER_Begin( 0, L"Scene" );

	// Set the render target to be our intermediate render target - NOT the back buffer.
	m_ImmediateContext->OMSetRenderTargets( 1, &m_RenderTargetView, m_DepthView );

	// Switch off alpha blending
	float BlendFactor[1] = { 0.0f };
	m_ImmediateContext->OMSetBlendState( m_OpaqueState, BlendFactor, 0xffffffff );

	// Clear the color target and depth stencil buffer
	FLOAT clearColor[ 4 ] = { 0.1f, 0.1f, 0.2f, 1.0f };
	if ( m_Scene == StressTest )
	{
		clearColor[ 0 ] = clearColor[ 1 ] = clearColor[ 2 ] = 0.0f;
	}
	m_ImmediateContext->ClearRenderTargetView( m_RenderTargetView, clearColor );
	m_ImmediateContext->ClearDepthStencilView( m_DepthView, D3D11_CLEAR_DEPTH, 1.0, 0 );
	
	// Set the depth stecnil state
	m_ImmediateContext->OMSetDepthStencilState( m_SceneDepthStencilState, 0 );
	
	// Set the viewport to cover the whole render target
	D3D11_VIEWPORT vp;
	vp.Width = (float)m_Width * m_ResolutionMultiplierX;
	vp.Height = (float)m_Height * m_ResolutionMultiplierY;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	m_ImmediateContext->RSSetViewports( 1, &vp );
	
	m_ImmediateContext->RSSetState( m_RasterStateCullBack );
	
	// Set the samplers biased on sample level if we are doing per sample SSAA
	ID3D11SamplerState* samplers[ 2 ] = { m_SceneSamplers[ NoBias ].m_PointSampler, m_SceneSamplers[ NoBias ].m_AnisoSampler };
	if ( m_AntiAliasingType == SSAAx4SF )
	{
		samplers[ 0 ] = m_SceneSamplers[ MinusOne ].m_PointSampler;
		samplers[ 1 ] = m_SceneSamplers[ MinusOne ].m_AnisoSampler;
	}
	else if ( m_AntiAliasingType == SSAAx8SF )
	{
		samplers[ 0 ] = m_SceneSamplers[ MinusOneAndAHalf ].m_PointSampler;
		samplers[ 1 ] = m_SceneSamplers[ MinusOneAndAHalf ].m_AnisoSampler;
	}
	m_ImmediateContext->PSSetSamplers( 0, 2, samplers );
	
	DirectX::XMMATRIX view = m_Camera->GetViewMatrix();

	DirectX::XMMATRIX ViewProj = view * m_Camera->GetProjMatrix();

	// Render the scene to our MSAA/SSAA target
	if ( m_Scene == TypicalScene )
	{
		// Update the scene constant buffer
		D3D11_MAPPED_SUBRESOURCE resource;
		if ( m_ImmediateContext->Map( m_SceneConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource ) == S_OK )
		{
			SceneConstantBuffer* constants = (SceneConstantBuffer*)resource.pData;
		
			// ViewProj matrix from camera
			constants->m_WorldViewProj = DirectX::XMMatrixTranspose( ViewProj );
		
			// No transformation of this scene
			DirectX::XMMATRIX dxmatIdentity = DirectX::XMMatrixIdentity();
			
			constants->m_World = DirectX::XMMatrixTranspose( dxmatIdentity );
		
			// Eye position
			constants->m_EyePosition = m_Camera->GetEyePt();

			// Invert the sun direction to avoid doing it in pixel shader
			constants->m_SunDirection = DirectX::XMVectorNegate( DirectX::XMVector3Normalize( gSunDir ) );

			// Lighting conditions
			constants->m_SunColor = DirectX::XMVectorSet( 0.3f, 0.3f, 0.25f, 0.0f );
			constants->m_AmbientColor = DirectX::XMVectorSet( 0.02f, 0.02f, 0.05f, 0.0f );

			// Copy across the spot light params
			for ( int i = 0; i < 3; i++ )
			{
				DirectX::XMVECTOR spotLightDir = DirectX::XMVectorSubtract( gSpotPos[ i ], gSpotLookAt );
				spotLightDir = DirectX::XMVector3Normalize( spotLightDir );
				
				constants->m_SpotPositionAndRadius[ i ] = DirectX::XMVectorSetW( gSpotPos[ i ], 2000.0f );
				constants->m_SpotDirectionAndAngle[ i ] = DirectX::XMVectorSetW( spotLightDir, 0.86f );;
				constants->m_SpotColor[ i ] = gSpotColor[ i ];
			}
		
			m_ImmediateContext->Unmap( m_SceneConstantBuffer, 0 );
		}

		// Both vertex and pixel shaders use this constant buffer
		m_ImmediateContext->VSSetConstantBuffers( 0, 1, &m_SceneConstantBuffer );
		m_ImmediateContext->PSSetConstantBuffers( 0, 1, &m_SceneConstantBuffer );

		// Set shaders
		m_ImmediateContext->IASetInputLayout( m_SceneInputLayout );
		m_ImmediateContext->VSSetShader( m_SceneVS, 0, 0 );
		m_ImmediateContext->PSSetShader( GetScenePixelShader(), 0, 0 );
		
		// Submit the draw calls
		m_SceneMesh->Render( m_ImmediateContext, 0, 1 );
	}
	else
	{
		// Set the vertex and pixel shaders
		m_ImmediateContext->IASetInputLayout( m_StressTestInputLayout );
		m_ImmediateContext->VSSetShader( m_StressTestVS, 0, 0 );
		m_ImmediateContext->PSSetShader( GetScenePixelShader(), 0, 0 );

		// Render the stress test scene
		RenderStressTestScene( ViewProj );
	}

	TIMER_End();

	TIMER_Begin( 0, L"AA Resolve" );

	// Now perform resolve
	// This is either an MSAA resolve using ResolveSubresource, or a quad blit to downsample the SSAA target. Either way,
	// a quad blit is required to write our scene to the back buffer

	if ( m_MultisampledSurface )
	{
		m_ImmediateContext->ResolveSubresource( m_DestinationTexture, 0, m_MultisampledSurface, 0, GetRenderTargetFormat() );
	}

	UINT stride = sizeof( QuadVertex );
	UINT offset = 0;

	// Set the backbuffer as the render target
	m_ImmediateContext->OMSetRenderTargets( 1, &rtv, dsv );

	// Restore the viewport to the size of the backbuffer
	vp.Width = (FLOAT)m_Width;
	vp.Height = (FLOAT)m_Height;
	m_ImmediateContext->RSSetViewports( 1, &vp );
	
	// Set the depth stencil state
	m_ImmediateContext->OMSetDepthStencilState( m_QuadDepthStencilState, 0 );

	// Set the intermediate target as the blit input with linear filtering
	m_ImmediateContext->PSSetSamplers( 0, 1, &m_QuadSampler );
	m_ImmediateContext->PSSetShaderResources( 0, 1, &m_DestinationTextureSRV );

	// Render the blit
	m_ImmediateContext->IASetInputLayout( m_QuadInputLayout );
	m_ImmediateContext->IASetVertexBuffers( 0, 1, &m_QuadVB, &stride, &offset );
	m_ImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	m_ImmediateContext->VSSetShader( m_QuadVS, 0, 0 );
	m_ImmediateContext->PSSetShader( GetQuadPixelShader(), 0, 0 );
	m_ImmediateContext->Draw( 6, 0 );

	TIMER_End();
}


// Allocate (or reallocate) the intermediate targets and acquire render target and shader views
void SSAA::CreateRenderTargets()
{
	// Clean up previously allocated targets
	DestroyRenderTargets();

	// Set up new destination texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory( &desc, sizeof( desc ) );
	desc.Width = (int)( (float)m_Width * m_ResolutionMultiplierX );
	desc.Height = (int)( (float)m_Height * m_ResolutionMultiplierY );
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = GetRenderTargetFormat();
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = S_OK;
	V( m_Device->CreateTexture2D( &desc, 0, &m_DestinationTexture ) );
	
	// If the multisample level is greater than one then we don't want to render directly to the destination texture but to a render target that 
	// is set up with the correct multisample level and resolve down to the destination after we have finished rendering.
	if ( GetMultisampleLevel() > 1 )
	{
		// Set the appropriate multisample level and quality level
		desc.SampleDesc.Count = GetMultisampleLevel();
		desc.SampleDesc.Quality = GetMultisampleQuality();
		
		// Create the multisampled surface
		V( m_Device->CreateTexture2D( &desc, 0, &m_MultisampledSurface ) );
		
		// Create the multisampled render target view
		D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
		viewDesc.Format = desc.Format;
		viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		viewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
		V( m_Device->CreateRenderTargetView( m_MultisampledSurface, &viewDesc, &m_RenderTargetView ) );
	}
	else
	{
		// If there is no multisampling, then we can just get a render target view to the destination texture and render directly to that
		D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
		viewDesc.Format = desc.Format;
		viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
		V( m_Device->CreateRenderTargetView( m_DestinationTexture, &viewDesc, &m_RenderTargetView ) );
	}

	// Create the depth stencil surface that matches the render target view of the color scene
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	V( m_Device->CreateShaderResourceView( m_DestinationTexture, &srvDesc, &m_DestinationTextureSRV ) );

	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	V( m_Device->CreateTexture2D( &desc, 0, &m_DepthTexture ) );

	D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc;
	depthDesc.Format = desc.Format;
	depthDesc.Flags = 0;
	if ( GetMultisampleLevel() > 1 )
	{
		depthDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		depthDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
	}
	else
	{
		depthDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthDesc.Texture2D.MipSlice = 0;
	}
	
	V( m_Device->CreateDepthStencilView( m_DepthTexture, &depthDesc, &m_DepthView ) );

	// Update the constant buffer that specifies the width and height of the destination texture
	D3D11_MAPPED_SUBRESOURCE Resource;
	if ( m_ImmediateContext->Map( m_QuadConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Resource ) == S_OK )
	{
		QuadConstantBuffer* Constants = (QuadConstantBuffer*)Resource.pData;
		
		float width = (float)desc.Width;
		float height = (float)desc.Height;

		Constants->m_Dimensions = DirectX::XMVectorSet( width, height, 1.0f / width, 1.0f / height );

		m_ImmediateContext->Unmap( m_QuadConstantBuffer, 0 );
	}

	m_ImmediateContext->PSSetConstantBuffers( 1, 1, &m_QuadConstantBuffer );
}


// Clean up previously allocated render target resources
void SSAA::DestroyRenderTargets()
{
	SAFE_RELEASE( m_DepthView );
	SAFE_RELEASE( m_DepthTexture );
	SAFE_RELEASE( m_DestinationTextureSRV );
	SAFE_RELEASE( m_RenderTargetView );
	SAFE_RELEASE( m_MultisampledSurface );
	SAFE_RELEASE( m_DestinationTexture );
}


// Get the DXGI format of the currently selected render target format
DXGI_FORMAT SSAA::GetRenderTargetFormat() const
{
	const DXGI_FORMAT formats[ FmtMax ] = 
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_R16G16B16A16_FLOAT
	};

	return formats[ m_Format ];
}


// Return the bits-per-pixel size in bytes of the render target
int SSAA::GetRenderTargetFormatSizeInBytes() const
{
	const int formatSizes[ FmtMax ] = 
	{
		4,
		4,
		8
	};

	return formatSizes[ m_Format ];
}


// Return the multisample level for the current AA type
UINT SSAA::GetMultisampleLevel() const 
{
	switch ( m_AntiAliasingType )
	{
		case MSAAx2: case SSAAx2SF: case EQAA2f4x: return 2;
		case MSAAx4: case SSAAx4SF: case EQAA4f8x:return 4;
		case MSAAx8: case SSAAx8SF: case EQAA8f16x: return 8;
	}

	return 1;
}


// Get the multisample quality level for EQAA modes
UINT SSAA::GetMultisampleQuality() const
{
	switch ( m_AntiAliasingType )
	{
		case EQAA2f4x: return 4;
		case EQAA4f8x: return 8;
		case EQAA8f16x: return 16;
	}

	return 0;
}


// Get the quad blit pixel shader
ID3D11PixelShader* SSAA::GetQuadPixelShader()
{
	// This is normally a standard copy shader except for the case of the rotated grid SSAA where the shader performs 4 texture reads
	switch ( m_AntiAliasingType )
	{
		case SSAAx4RG: return m_Quad2x2RGPS;
	}

	return m_QuadNormalPS;
}


// Get the scene pixel shader
ID3D11PixelShader* SSAA::GetScenePixelShader()
{
	// If we are doing per-sample AA then we need to use the appropriate per-sample pixel shader
	switch ( m_AntiAliasingType )
	{
		case SSAAx2SF:
		case SSAAx4SF:
		case SSAAx8SF:
			return m_Scene == StressTest ? m_StressTestSampleFrequencyPS : m_SceneSampleFrequencyPS;

		default: 
			return m_Scene == StressTest ? m_StressTestPS : m_ScenePS;
	}
}


// Updates the description string
void SSAA::UpdateDescription()
{
	float width = (float)m_Width;
	float height = (float)m_Height;

	// Calculate the color target VRAM cost
	float colorVRAMUsage = ( width * m_ResolutionMultiplierX * height * m_ResolutionMultiplierY * (float)GetRenderTargetFormatSizeInBytes() ) / ( 1024.0f * 1024.0f );

	// Calculate the depth stencil cost
	float depthVRAMUsage =  ( width * m_ResolutionMultiplierX * height * m_ResolutionMultiplierY * 4.0f * (float)GetMultisampleLevel() ) / ( 1024.0f * 1024.0f );
	
	// EDIT: ACTUALLY, DONT ADD THE COST OF THE RESOLVE TARGET AS WE COULD RESOLVE TO BACKBUFFER
	// Don't forget to also factor in the cost of the multisampled color surface if there is one
	//if ( GetMultisampleLevel() > 1 )
	//	colorVRAMUsage += ( width * height * (float)GetMultisampleLevel() * (float)GetRenderTargetFormatSizeInBytes() ) / ( 1024.0f * 1024.0f );
	//~

	// Display the number of samples if multisampling is on
	wchar_t sampleDesc[ 64 ];
	sampleDesc[ 0 ] = 0;
	if ( GetMultisampleLevel() > 1 )
	{
		_snwprintf_s( sampleDesc, ARRAYSIZE( sampleDesc ), L"(%d samples) ", GetMultisampleLevel() );
	}

	_snwprintf_s( m_Description, ARRAYSIZE( m_Description ), L"Render target %dx%d %s(%1.1fMb color, %1.1fMb Z-buffer)", (int)(width * m_ResolutionMultiplierX), (int)(height * m_ResolutionMultiplierY), sampleDesc, colorVRAMUsage, depthVRAMUsage );
}


// Render the alpha stress test scene
void SSAA::RenderStressTestScene( const DirectX::XMMATRIX& ViewProj )
{
	UINT Stride = 28;
	UINT Offset = 0;

	m_ImmediateContext->PSSetShaderResources( 0, 1, &m_StressTestTexture );

	m_ImmediateContext->IASetVertexBuffers( 0, 1, &m_StressTestVB, &Stride, &Offset );
	m_ImmediateContext->IASetIndexBuffer( m_StressTestIB, DXGI_FORMAT_R16_UINT, 0 );

	m_ImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_ImmediateContext->PSSetConstantBuffers( 0, 1, &m_SceneConstantBuffer );

	DirectX::XMVECTOR LightDir = DirectX::XMVector3Normalize( gSunDir );

	for ( int i = 0; i < 10; i++ )
	{
		for ( int j = 0; j < 10; j++ )
		{
			for ( int k = 0; k < 4; k++ )
			{
				if ( k < 1 || i == 0 || j == 0 )
				{
					DirectX::XMMATRIX World = DirectX::XMMatrixTranslation( (-5 + i) * 2.5f, k * 2.2f, (-5 + j) * 2.5f );
					
					D3D11_MAPPED_SUBRESOURCE resource;
					if ( m_ImmediateContext->Map( m_SceneConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource ) == S_OK )
					{
						SceneConstantBuffer* constants = (SceneConstantBuffer*)resource.pData;
		
						DirectX::XMMATRIX wvp = World * ViewProj;
						constants->m_WorldViewProj = DirectX::XMMatrixTranspose( wvp );
		
						constants->m_World = DirectX::XMMatrixTranspose( World );
		
						constants->m_EyePosition = m_Camera->GetEyePt();
						constants->m_SunDirection = LightDir;
	
						m_ImmediateContext->Unmap( m_SceneConstantBuffer, 0 );
					}

					m_ImmediateContext->VSSetConstantBuffers( 0, 1, &m_SceneConstantBuffer );

					m_ImmediateContext->DrawIndexed( 36, 0, 0 );
				}
			}
		}
	}
}


// Return a verbose description of the current AA mode
const wchar_t* SSAA::GetAADescription() const
{
	const wchar_t* descriptions[ Max ] = 
	{
		L"No Antialiasing",
		
		L"2x Multisample Antialiasing",
		L"2x Supersample AA Horizontal",
		L"2x Supersample AA Vertical",
		L"Per-sample 2x Supersample AA",
		
		L"1.5 x 1.5 Supersample AA",
		
		L"4x Multisample Antialiasing",
		L"4x Supersample AA",
		L"Per-sample 4x Supersample AA with -1 Mip LOD Bias",

		L"4x Supersample AA with custom rotated grid resolve",

		L"8x Multisample Antialiasing",
		L"Per-sample 8x Supersample AA with -1.5 Mip LOD Bias",
		
		L"2f4x Enhanced Quality AA",
		L"4f8x Enhanced Quality AA",
		L"8f16x Enhanced Quality AA",
	};

	return descriptions[ m_AntiAliasingType ];
}