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
#ifndef __SSAA_H__
#define __SSAA_H__


#include "../../DXUT/Core/DXUT.h"


class CFirstPersonCamera;
class CDXUTSDKMesh;


class SSAA
{
public:

	enum Type
	{
		None,
		
		MSAAx2,
		SSAAx2H,
		SSAAx2V,
		SSAAx2SF,
		
		SSAAx15,
		
		MSAAx4,
		SSAAx4,
		SSAAx4SF,

		SSAAx4RG,

		MSAAx8,
		SSAAx8SF,
		
		EQAA2f4x,
		EQAA4f8x,
		EQAA8f16x,
		
		Max
	};

	enum RenderTargetFormat
	{
		Fmt8x4,
		Fmt1010102,
		FmtFP16x4,
		FmtMax
	};

	enum SceneType
	{
		TypicalScene,
		StressTest
	};

	// Construction/destruction
	SSAA();
	~SSAA();

	// Settings for SSAA
	void SetAAType( Type type );
	void SetRenderTargetFormat( RenderTargetFormat format );
	void SetScene( SceneType scene );
	
	// Init/deinit called on create/destroy D3D device
	void Init( ID3D11Device* device, ID3D11DeviceContext* immediateContext, CDXUTSDKMesh* sceneMesh, const CFirstPersonCamera& camera );
	void DeInit();
	
	// Called when resolution/window size changed
	void OnResize( int width, int height );

	// Renders the scene to an intermediate render target and resolves to backbuffer
	void Render( ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv );

	Type GetAAType() const { return m_AntiAliasingType; }
	RenderTargetFormat GetRenderTargetType() const { return m_Format; }
	SceneType GetSceneType() const { return m_Scene; }
	const wchar_t* GetDescription() const { return m_Description; }
	const wchar_t* GetAADescription() const;
	
private:

	// Create/destroy intermediate render targets
	void CreateRenderTargets();
	void DestroyRenderTargets();

	DXGI_FORMAT GetRenderTargetFormat() const;
	int GetRenderTargetFormatSizeInBytes() const;
	UINT GetMultisampleLevel() const;
	UINT GetMultisampleQuality() const;
	ID3D11PixelShader* GetQuadPixelShader();
	ID3D11PixelShader* GetScenePixelShader();

	void UpdateDescription();
	void RenderStressTestScene( const DirectX::XMMATRIX& ViewProj );

	struct SceneSamplers
	{
		ID3D11SamplerState*		m_PointSampler;
		ID3D11SamplerState*		m_AnisoSampler;
	};

	enum BiasLevels
	{
		NoBias,
		MinusOne,
		MinusOneAndAHalf,
		NumBiasLevels
	};

	Type								m_AntiAliasingType;
	float								m_ResolutionMultiplierX;
	float								m_ResolutionMultiplierY;
	RenderTargetFormat					m_Format;
	SceneType							m_Scene;
	const CFirstPersonCamera*			m_Camera;
	int									m_Width;
	int									m_Height;
	wchar_t								m_Description[ 256 ];
	
	// DX11 state
	ID3D11Device*						m_Device;
	ID3D11DeviceContext*				m_ImmediateContext;
	ID3D11Buffer*						m_SceneConstantBuffer;
	ID3D11Buffer*						m_QuadConstantBuffer;
	ID3D11DepthStencilState*			m_SceneDepthStencilState;
	ID3D11DepthStencilState*			m_QuadDepthStencilState;
	ID3D11BlendState*					m_OpaqueState;
	ID3D11RasterizerState*				m_RasterStateCullBack;
	ID3D11RasterizerState*				m_RasterStateCullFront;
	
	// Scene samplers
	SceneSamplers						m_SceneSamplers[ NumBiasLevels ];

	// Scene D3D resources
	CDXUTSDKMesh*						m_SceneMesh;
	ID3D11InputLayout*					m_SceneInputLayout;
	ID3D11VertexShader*					m_SceneVS;
	ID3D11PixelShader*					m_ScenePS;
	ID3D11PixelShader*					m_SceneSampleFrequencyPS;
	
	// Stress Test Scene D3D resources
	ID3D11InputLayout*					m_StressTestInputLayout;
	ID3D11Buffer*						m_StressTestVB;
	ID3D11Buffer*						m_StressTestIB;
	ID3D11VertexShader*					m_StressTestVS;
	ID3D11PixelShader*					m_StressTestPS;
	ID3D11PixelShader*					m_StressTestSampleFrequencyPS;
	ID3D11ShaderResourceView*			m_StressTestTexture;

	// Full screen quad D3D resources
	ID3D11InputLayout*					m_QuadInputLayout;
	ID3D11VertexShader*					m_QuadVS;
	ID3D11PixelShader*					m_QuadNormalPS;
	ID3D11PixelShader*					m_Quad2x2RGPS;
	ID3D11Buffer*						m_QuadVB;
	ID3D11SamplerState*					m_QuadSampler;
	
	// Render targets
	ID3D11Texture2D*					m_DestinationTexture;
	ID3D11Texture2D*					m_MultisampledSurface;
	ID3D11RenderTargetView*				m_RenderTargetView;
	ID3D11ShaderResourceView*			m_DestinationTextureSRV;
	ID3D11Texture2D*					m_DepthTexture;
	ID3D11DepthStencilView*				m_DepthView;
};

#endif