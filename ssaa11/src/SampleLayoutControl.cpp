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
#include "SampleLayoutControl.h"
#include "SSAA.h"


static const DirectX::XMVECTOR gPixelBackgroundColor = DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 1.0f );
static const DirectX::XMVECTOR gPixelForgroundColor = DirectX::XMVectorSet( 0.0f, 0.0f, 1.0f, 1.0f );
static const DirectX::XMVECTOR gColorSampleColor = DirectX::XMVectorSet( 1.0f, 0.0f, 0.0f, 1.0f );
static const DirectX::XMVECTOR gCoverageSampleColor = DirectX::XMVectorSet( 1.0f, 1.0f, 0.0f, 1.0f );
static const DirectX::XMVECTOR gResolveLocationColor = DirectX::XMVectorSet( 1.0f, 1.0f, 0.1f, 1.0f );
static const DirectX::XMVECTOR gDropShadowColor = DirectX::XMVectorSet( 0.0f, 0.0f, 0.0f, 0.8f );
static const float	gDropShadowOffset = 0.015f;


SampleLayoutControl::SampleLayoutControl( CDXUTDialog* pDialog, const SSAA& ssaa, AMD::Sprite& sprite ) :
	CDXUTControl( pDialog ),
	m_SSAA( ssaa ),
	m_Sprite( sprite ),
	m_CircleTexture( 0 ),
	m_CrossTexture( 0 ),
	m_Scale( 1.0f ),
	m_Timer( 0.0f )
{ 
	m_pDialog = pDialog; // DXUT brokeness!
}


void SampleLayoutControl::RenderSprite( float cx, float cy, float size, DirectX::XMVECTOR color, Primitive primitive )
{
	POINT pt;
	m_pDialog->GetLocation( pt );
	int baseX = pt.x + m_x; 
	int baseY = pt.y + m_y;
	float w = (float)m_width;
	float ws = w * m_Scale;

	m_Sprite.SetSpriteColor( color );
	m_Sprite.SetBorderColor( DirectX::XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ) );

	ID3D11ShaderResourceView* texture = 0;
	switch ( primitive )
	{
		case Circle: texture = m_CircleTexture; break;
		case Cross: texture = m_CrossTexture; break;
	}

	int hs = (int)(0.5f * ws * size);
	int s = hs*2;
	m_Sprite.RenderSprite( texture, baseX + (int)(w * cx) - hs, baseY - m_width + hs + (int)(w * (1.0f - cy)), s, s, true, false );
}


void SampleLayoutControl::RenderPixel( float cx, float cy )
{
	RenderSprite( cx, cy, 0.5f, gPixelBackgroundColor, Square );
	RenderSprite( cx, cy, 0.45f, gPixelForgroundColor, Square );
}


void SampleLayoutControl::RenderPoint( float cx, float cy, int flags )
{
	cx -= 0.5f;
	cy -= 0.5f;
	cx *= m_Scale;
	cy *= m_Scale;
	cx += 0.5f;
	cy += 0.5f;

	if ( flags & ColorSample )
	{
		if ( flags & SampleDropShadow )
			RenderSprite(cx + gDropShadowOffset, cy - gDropShadowOffset, 0.12f, gDropShadowColor, Circle );
		RenderSprite( cx, cy, 0.12f, gColorSampleColor, Circle );
	}
	
	if ( flags & CoverageSample )
	{
		if ( flags & SampleDropShadow )
			RenderSprite( cx + gDropShadowOffset, cy - gDropShadowOffset, 0.04f, gDropShadowColor, Circle );
		RenderSprite( cx, cy, 0.04f, gCoverageSampleColor, Circle );
	}

	if ( flags & ResolveLocation )
	{
		const float size = 0.12f;
		
		RenderSprite( cx + gDropShadowOffset, cy - gDropShadowOffset, size, gDropShadowColor, Cross );
		RenderSprite( cx, cy, size, gResolveLocationColor, Cross );
	}
}


static float ConvertToNormalisedCoord( int value )
{
	return 0.5f + ( 0.5f * ((float)value / 8.0f ) );
}


void SampleLayoutControl::Render()
{
	// The MSAA sample locations are specified by the DX spec.
	// The EQAA sample locations are vendor specific and could change on future hardware.
	const int MSAA2TapsX[ 2 ] = { 4, -4 };
	const int MSAA2TapsY[ 2 ] = { 4, -4 };

	const int MSAA4TapsX[ 4 ] = { -2, 6, -6, 2 };
	const int MSAA4TapsY[ 4 ] = { -6, -2, 2, 6 };
	
	const int MSAA8TapsX[ 8 ] = { 1, -1, 5, -3, -5, -7, 3, 7 };
	const int MSAA8TapsY[ 8 ] = { -3, 3, 1, -5, 5, -1, 7, -7 };

	const int EQAA4CoverageTapsX[ 4 ] = { -6, 6, -2, 2 };
	const int EQAA4CoverageTapsY[ 4 ] = { -6, 6, 2, -2 };

	const int EQAA8CoverageTapsX[ 8 ] = { 7, -7, -5, 1, 3, -3, -1, 5 };
	const int EQAA8CoverageTapsY[ 8 ] = { 6, -8, 5, -5, 7, -7, 1, -1 };

	const int EQAA16CoverageTapsX[ 16 ] = { 7, -7, -5, 1, 3, -3, -1, 5, 4, -8, -2, 2, 0, -4, -6, 6 };
	const int EQAA16CoverageTapsY[ 16 ] = { 6, -8, 5, -5, 7, -7, 1, -1, 2, -6, 3, -3, 4, -2, 0, -4 };

	const float SSAARGTapsX[ 4 ] = { 0.4f, 0.9f, -0.4f, -0.9f };
	const float SSAARGTapsY[ 4 ] = { 0.9f, -0.4f, -0.9f, 0.4f };
	
	m_Sprite.SetPointSample( false ); // bilinear sprite rendering

	switch ( m_SSAA.GetAAType() )
	{
		case SSAA::None:
			m_Scale = 1.5f;
			RenderPixel( 0.5f, 0.5f );
			RenderPoint( 0.5f, 0.5f, ColorSample | CoverageSample | ResolveLocation );
			m_Scale = 1.0f;
			break;

		case SSAA::MSAAx2:
		case SSAA::SSAAx2SF:
		case SSAA::EQAA2f4x:
			m_Scale = 1.5f;
			RenderPixel( 0.5f, 0.5f );
			
			if ( m_SSAA.GetAAType() == SSAA::EQAA2f4x )
			{
				int sampleflags = ColorSample | CoverageSample;
				for ( int i = 0; i < 4; i++ )
				{
					if ( i == 2 )
						sampleflags &= ~ColorSample;
					RenderPoint( 0.25f + (0.5f*ConvertToNormalisedCoord( EQAA4CoverageTapsX[ i ]) ), 0.25f + (0.5f*(1.0f-ConvertToNormalisedCoord( EQAA4CoverageTapsY[ i ]) ) ), sampleflags );
				}
			}
			else
			{
				for ( int i = 0; i < 2; i++ )
				{
					RenderPoint( 0.25f + (0.5f*ConvertToNormalisedCoord( MSAA2TapsX[ i ]) ), 0.25f + (0.5f*(1.0f-ConvertToNormalisedCoord( MSAA2TapsY[ i ]) ) ), ColorSample | CoverageSample );
				}
			}

			RenderPoint( 0.5f, 0.5f, ResolveLocation );
			m_Scale = 1.0f;
			break;
			
		case SSAA::SSAAx2H:
			RenderPixel( 0.25f, 0.5f );
			RenderPixel( 0.75f, 0.5f );
			RenderPoint( 0.25f, 0.5f, ColorSample | CoverageSample );
			RenderPoint( 0.75f, 0.5f, ColorSample | CoverageSample );
			RenderPoint( 0.5f, 0.5f, ResolveLocation );
			break;

		case SSAA::SSAAx2V:
			RenderPixel( 0.5f, 0.25f );
			RenderPixel( 0.5f, 0.75f );
			RenderPoint( 0.5f, 0.25f, ColorSample | CoverageSample );
			RenderPoint( 0.5f, 0.75f, ColorSample | CoverageSample );
			RenderPoint( 0.5f, 0.5f, ResolveLocation );
			break;

		case SSAA::MSAAx4:
		case SSAA::EQAA4f8x:
		case SSAA::SSAAx4SF:
			m_Scale = 1.5f;
			RenderPixel( 0.5f, 0.5f );
			
			if ( m_SSAA.GetAAType() == SSAA::EQAA4f8x )
			{
				int sampleflags = ColorSample | CoverageSample;
				for ( int i = 0; i < 8; i++ )
				{
					if ( i == 4 )
						sampleflags &= ~ColorSample;
					RenderPoint( 0.25f + (0.5f*ConvertToNormalisedCoord( EQAA8CoverageTapsX[ i ]) ), 0.25f + (0.5f*(1.0f-ConvertToNormalisedCoord( EQAA8CoverageTapsY[ i ]) ) ), sampleflags );
				}
			}
			else
			{
				for ( int i = 0; i < 4; i++ )
				{
					RenderPoint( 0.25f + (0.5f*ConvertToNormalisedCoord( MSAA4TapsX[ i ] )), 0.25f + (0.5f*(1.0f-ConvertToNormalisedCoord( MSAA4TapsY[ i ] ))), ColorSample | CoverageSample );
				}
			}

			RenderPoint( 0.5f, 0.5f, ResolveLocation );
			m_Scale = 1.0f;
			break;

		case SSAA::MSAAx8:
		case SSAA::SSAAx8SF:
		case SSAA::EQAA8f16x:
			m_Scale = 1.5f;
			RenderPixel( 0.5f, 0.5f );
			
			if ( m_SSAA.GetAAType() == SSAA::EQAA8f16x )
			{
				int sampleflags = ColorSample | CoverageSample;
				for ( int i = 0; i < 16; i++ )
				{
					if ( i == 8 )
						sampleflags &= ~ColorSample;
					RenderPoint( 0.25f + (0.5f*ConvertToNormalisedCoord( EQAA16CoverageTapsX[ i ]) ), 0.25f + (0.5f*(1.0f-ConvertToNormalisedCoord( EQAA16CoverageTapsY[ i ]) ) ), sampleflags );
				}
			}
			else
			{
				for ( int i = 0; i < 8; i++ )
				{
					RenderPoint( 0.25f + (0.5f*ConvertToNormalisedCoord( MSAA8TapsX[ i ] )), 0.25f + (0.5f*(1.0f-ConvertToNormalisedCoord( MSAA8TapsY[ i ]) )), ColorSample | CoverageSample );
				}
			}

			RenderPoint( 0.5f, 0.5f, ResolveLocation );
			m_Scale = 1.0f;
			break;

		case SSAA::SSAAx4:
		case SSAA::SSAAx4RG:
			RenderPixel( 0.25f, 0.25f );
			RenderPixel( 0.25f, 0.75f );
			RenderPixel( 0.75f, 0.25f );
			RenderPixel( 0.75f, 0.75f );

			RenderPoint( 0.25f, 0.25f, ColorSample | CoverageSample );
			RenderPoint( 0.25f, 0.75f, ColorSample | CoverageSample );
			RenderPoint( 0.75f, 0.25f, ColorSample | CoverageSample );
			RenderPoint( 0.75f, 0.75f, ColorSample | CoverageSample );

			if ( m_SSAA.GetAAType() == SSAA::SSAAx4RG )
			{
				for ( int i = 0; i < 4; i++ )
				{
					RenderPoint( 0.5f + (0.5f*SSAARGTapsX[ i ]), 0.5f + (0.5f*SSAARGTapsY[ i ]), ResolveLocation );
				}
			}
			else
			{
				RenderPoint( 0.5f, 0.5f, ResolveLocation );
			}
			break;

		case SSAA::SSAAx15:
		{
			ID3D11DeviceContext* pd3dDeviceContext = m_pDialog->GetManager()->GetD3D11DeviceContext();
			
			POINT pt;
			m_pDialog->GetLocation( pt );
			int baseX = pt.x + m_x; 
			int baseY = pt.y + m_y;
			
			RECT rc;
			int w8 = m_width / 8;
			int w4 = m_width / 4;
			rc.left = baseX + w8;
			rc.top = baseY - m_width + w8;
			rc.bottom = rc.top + m_width - w4;
			rc.right = rc.left + m_width - w4;
			
			pd3dDeviceContext->RSSetScissorRects( 1, &rc );

			m_Sprite.EnableScissorTest( true );
		
			float s = 0.125f;

			RenderPixel( 0.25f + s, 0.25f - s );
			RenderPixel( 0.25f + s, 0.75f - s );
			RenderPixel( 0.75f + s, 0.25f - s );
			RenderPixel( 0.75f + s, 0.75f - s );

			RenderPoint( 0.25f + s, 0.25f - s, ColorSample | CoverageSample );
			RenderPoint( 0.25f + s, 0.75f - s, ColorSample | CoverageSample );
			RenderPoint( 0.75f + s, 0.25f - s, ColorSample | CoverageSample );
			RenderPoint( 0.75f + s, 0.75f - s, ColorSample | CoverageSample );

			RenderPoint( 0.5f, 0.5f, ResolveLocation );

			m_Sprite.EnableScissorTest( false );
			
			break;
		}
	}

	RenderSprite( 0.0f + gDropShadowOffset, -0.1f - gDropShadowOffset, 0.1f, gDropShadowColor, Square );
	RenderSprite( 0.0f, -0.1f, 0.1f, gPixelBackgroundColor, Square );
	RenderSprite( 0.0f, -0.1f, 0.09f, gPixelForgroundColor, Square );

	RenderPoint( 0.0f, -0.25f, ColorSample | SampleDropShadow | IgnoreTemporalAA );
	RenderPoint( 0.0f, -0.4f, CoverageSample | SampleDropShadow | IgnoreTemporalAA);
	RenderPoint( 0.0f, -0.55f, ResolveLocation | IgnoreTemporalAA );

	m_Sprite.SetSpriteColor( DirectX::XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f ) );
	m_Sprite.SetPointSample( true );
}



void SampleLayoutControl::Render( float fElapsedTime )
{
	ID3D11Device* pd3dDevice = m_pDialog->GetManager()->GetD3D11Device();
	ID3D11DeviceContext* pd3dDeviceContext = m_pDialog->GetManager()->GetD3D11DeviceContext();

	m_pDialog->GetManager()->EndSprites11( pd3dDevice, pd3dDeviceContext );
	EndText11( pd3dDevice, pd3dDeviceContext );

	m_Timer += fElapsedTime;
	if ( m_Timer > 4.0f )
		m_Timer = 0.0f;

	Render();

	m_pDialog->GetManager()->BeginSprites11();
	BeginText11();
	m_pDialog->GetManager()->ApplyRenderUI11( pd3dDeviceContext );
		
	DXUTTextureNode* pTextureNode = m_pDialog->GetTexture( 0 );
	pd3dDeviceContext->PSSetShaderResources( 0, 1, &pTextureNode->pTexResView11 );
}