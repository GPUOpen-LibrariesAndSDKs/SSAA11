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
#ifndef __SAMPLE_LAYOUT_CONTROL_H__
#define __SAMPLE_LAYOUT_CONTROL_H__


#include "../../DXUT/Core/DXUT.h"
#include "../../DXUT/Optional/DXUTgui.h"
#include "../../AMD_SDK/inc/AMD_SDK.h"


class SSAA;

// Class declaration for Sample Layout Control - Custom DXUT GUI control for rendering sample locations for MSAA/SSAA
class SampleLayoutControl : public CDXUTControl
{
public:

	SampleLayoutControl( CDXUTDialog* pDialog, const SSAA& ssaa, AMD::Sprite& sprite );
	virtual ~SampleLayoutControl() {}
	virtual void Render( float fElapsedTime );

	void SetTextures( ID3D11ShaderResourceView* circleTexture, ID3D11ShaderResourceView* crossTexture ) { m_CircleTexture = circleTexture; m_CrossTexture = crossTexture; }
	
private:

	SampleLayoutControl& operator=( const SampleLayoutControl& );
	
	enum Primitive
	{
		Square,
		Circle,
		Cross
	};

	enum
	{
		ResolveLocation = 1 << 0,
		ColorSample = 1 << 1,
		CoverageSample = 1 << 2,
		SampleDropShadow = 1 << 3,
		IgnoreTemporalAA = 1 << 4
	};

	void Render();
	void RenderSprite( float cx, float cy, float size, DirectX::XMVECTOR color, Primitive primitive );
	void RenderPixel( float cx, float cy );
	void RenderPoint( float cx, float cy, int flags );

	const SSAA&					m_SSAA;
	AMD::Sprite&				m_Sprite;
	ID3D11ShaderResourceView*	m_CircleTexture;
	ID3D11ShaderResourceView*	m_CrossTexture;
	float						m_Scale;
	float						m_Timer;
};


#endif