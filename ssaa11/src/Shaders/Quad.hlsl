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
struct VsQuadInput
{
    float3 v3Pos : POSITION; 
    float2 v2Tex : TEXCOORD0; 
};

struct PsQuadInput
{
    float4 v4Pos : SV_Position; 
    float2 v2Tex : TEXCOORD0;
};


cbuffer TextureConstants : register( b1 )
{
	float4	dimensions;	// w, h, 1/w, 1/h
};


Texture2D				g_Texture         : register( t0 );


SamplerState        g_SampleLinear : register( s0 );


PsQuadInput VSMain( in VsQuadInput I )
{
    PsQuadInput O = (PsQuadInput)0;

    O.v4Pos.x = I.v3Pos.x;
    O.v4Pos.y = I.v3Pos.y;
    O.v4Pos.z = I.v3Pos.z;
    O.v4Pos.w = 1.0f;
    
    O.v2Tex = I.v2Tex;
     
    return O;
}


float4 PSMain( PsQuadInput I ) : SV_Target
{
    return g_Texture.Sample( g_SampleLinear, I.v2Tex );
}


float4 PSMain2x2RG( PsQuadInput I ) : SV_Target
{
	float2 texelSize = dimensions.zw;

	float2 offsets[ 4 ] = 
	{
		float2(  0.4,  0.9 ),
		float2(  0.9, -0.4 ),
		float2( -0.4, -0.9 ),
		float2( -0.9,  0.4 ),
	};

	float4 value01 = g_Texture.Sample( g_SampleLinear, I.v2Tex + texelSize * offsets[ 0 ] );
	float4 value11 = g_Texture.Sample( g_SampleLinear, I.v2Tex + texelSize * offsets[ 1 ] );
	float4 value00 = g_Texture.Sample( g_SampleLinear, I.v2Tex + texelSize * offsets[ 2 ] );
	float4 value10 = g_Texture.Sample( g_SampleLinear, I.v2Tex + texelSize * offsets[ 3 ] );
	
	float4 value = value01 + value11 + value00 + value10;
	value *= 0.25;
	
	return value;
}