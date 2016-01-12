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

// Global constant buffer set once per frame
cbuffer Constants : register( b0 )
{
	matrix	WorldViewProjection;
	matrix	World;
	float4	EyePosition;
	float4	SunDirection;
	float4	SunColor;
	float4	AmbientColor;

	float4	SpotPositionAndRadius[ 3 ];
	float4	SpotDirectionAndAngle[ 3 ];
	float4	SpotColor[ 3 ];
};


// The two texture slots
Texture2D		g_txAlbedo : register( t0 );
Texture2D		g_txNormal : register( t1 );

// The sampler states defined by the demo
SamplerState	g_samPoint : register( s0 );
SamplerState	g_samAniso : register( s1 );

// Input from vertex buffer
struct VS_INPUT
{
	float4 position		: POSITION;
	float3 normal		: NORMAL;
	float2 texcoord		: TEXCOORD0;
	float3 tangent		: TANGENT;
};

// Output from vertex shader
struct VS_OUTPUT
{
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
	float2 texcoord		: TEXCOORD0;
	float3 worldPos		: TEXCOORD1;
	float4 position		: SV_POSITION;
};


// Input to pixel shader. 
// NB. no need to pass through SV_POSITION - an unnecessary expense. 
// "sample" modifier used to call shader on a per sample basis rather than per pixel.
#if defined (PER_SAMPLE_FREQUENCY)
struct PS_INPUT
{
	centroid sample float3		normal		: NORMAL;
	centroid sample float3		tangent		: TANGENT;
	sample float2				texcoord	: TEXCOORD0;
	centroid sample float3		worldPos	: TEXCOORD1;
};
#else
struct PS_INPUT
{
	centroid float3 normal		: NORMAL;
	centroid float3 tangent		: TANGENT;
	float2			texcoord	: TEXCOORD0;
	centroid float3 worldPos	: TEXCOORD1;
};
#endif

// Vertex normal decompression function
float3 R10G10B10A2_UNORM_TO_R32G32B32_FLOAT( in float3 vVec )
{
    vVec *= 2.0f;
    return vVec >= 1.0f ? ( vVec - 2.0f ) : vVec;
}

// Vertex shader entry point
VS_OUTPUT VSMain( in VS_INPUT input )
{
	VS_OUTPUT output;

	output.position = mul( input.position, WorldViewProjection );
	output.normal = normalize( mul( R10G10B10A2_UNORM_TO_R32G32B32_FLOAT( input.normal ), (float3x3)World ) );
	output.tangent = normalize( mul( R10G10B10A2_UNORM_TO_R32G32B32_FLOAT( input.tangent ), (float3x3)World ) );
	output.texcoord = input.texcoord;
	output.worldPos = mul( input.position, World ).xyz;

	return output;
}


// Lighting function to calculate lighting contribution per pixel for main scene.
// Includes calculations for 3 spot lights to better represent a real game scenario.
void LightingFunction( float3 position, float3 normal, float specularMask, out float3 lightIntensity, out float3 specularHilight )
{
	float ndotl = saturate( dot( normal, SunDirection.xyz ) );
	lightIntensity = 0;
	specularHilight = 0;
	
	lightIntensity += ndotl * SunColor.rgb;
	lightIntensity += AmbientColor.rgb;
	
	float3 viewDir = normalize( EyePosition.xyz - position );

	float3 halfAngle = normalize( viewDir + SunDirection.xyz );

	float specPower = pow( saturate( dot( halfAngle, normal ) ), 64 );
	
	float3 specularColor = float3( 0.5, 0.5, 0.5 );
	specularHilight += SunColor.rgb * specPower * specularMask * specularColor;

	// Spot light contributions
	for ( int i = 0; i < 3; i++ )
	{
		float radius = SpotPositionAndRadius[ i ].w;
		float coneAngle = SpotDirectionAndAngle[ i ].w;
			
		float3 vecToLight = SpotPositionAndRadius[ i ].xyz - position;
		float vecLength = length( vecToLight );
		float3 vecToLightNormalised = normalize( vecToLight );
		float coneDot = dot( vecToLightNormalised, SpotDirectionAndAngle[ i ].xyz );
		if (coneDot > coneAngle && vecLength < radius )
		{
			float distanceAtten = 1.0 - (vecLength / radius);
			float radialAttenuation = (coneDot - coneAngle) / (1.0 - coneAngle);
				
			ndotl = saturate( dot( normal, vecToLightNormalised ) );
			
			float3 contrib = SpotColor[ i ].rgb * radialAttenuation * distanceAtten;
			
			lightIntensity += contrib * ndotl;
			
			float3 halfAngle = normalize( viewDir + SpotDirectionAndAngle[ i ].xyz );

			float specPower = pow( saturate( dot( halfAngle, normal ) ), 64 );
			
			specularHilight += contrib * specPower * specularMask * specularColor;
		}
	}
}


// Main scene pixel shader entry point
float4 PSMainBump( in PS_INPUT input ) : SV_TARGET
{
	// Sample textures
	float4 albedo = g_txAlbedo.Sample( g_samAniso, input.texcoord );
	float3 normal = 2.0 * g_txNormal.Sample( g_samAniso, input.texcoord ).xyz - 1.0;
	float specMask = albedo.a;
	
	// Generate binormal from normal and tangent to save interpolators
	float3 binormal = normalize( cross( input.normal, input.tangent ) );
	
	// Calculate basis matrix and transform our texture space normal (from normal map) into world space
	float3x3 basisMatrix = float3x3( binormal, input.tangent, input.normal );
	normal = normalize( mul( normal, basisMatrix ) );
	
	// Calculate lighting contribution
	float3 lighting;
	float3 specularContrib;
	LightingFunction( input.worldPos, normal, specMask, lighting, specularContrib );

	float4 color = 1;
	
	// Multiply albedo by lighting and add on specular
	color.rgb = (lighting * albedo.rgb) + specularContrib;

	return color;
}


// Stress test vertex input from vertex buffer
struct VS_INPUT2
{
	float3 position		: POSITION;
	float3 normal		: NORMAL;
	float2 texcoord		: TEXCOORD0;
};

// Stress test vertex shader output
struct VS_OUTPUT2
{
	float3 normal		: NORMAL;
	float2 texcoord		: TEXCOORD0;
	float4 position		: SV_POSITION;
};

// Stress test vertex shader input
#if defined (PER_SAMPLE_FREQUENCY)
struct PS_INPUT2
{
	sample centroid float3	normal		: NORMAL;
	sample float2			texcoord	: TEXCOORD0;
};
#else
struct PS_INPUT2
{
	centroid float3 normal		: NORMAL;
	float2			texcoord	: TEXCOORD0;
};
#endif


// Stress test vertex shader entry point
VS_OUTPUT2 VSMain2( in VS_INPUT2 input )
{
	VS_OUTPUT2 output;

	output.position = mul( float4( input.position, 1 ), WorldViewProjection );
	output.normal = normalize( mul( input.normal, (float3x3)World ) );
	output.texcoord = input.texcoord;
	
	return output;
}


// Stress test pixel shader entry point
float4 PSMain2( in PS_INPUT2 input ) : SV_TARGET
{
	// Read albedo texture
	float4 albedo = g_txAlbedo.Sample( g_samPoint, input.texcoord );
	
	// Alpha test done using clip intruction
	clip( albedo.a - 0.1 );

	// Simple n.l lighting calculation
	float3 lighting = saturate( dot( SunDirection.xyz, input.normal ) );

	float4 color = 1;
	
	// Add little bit of ambient light
	lighting += 0.05;

	// Final lighting
	color.rgb = lighting * albedo.rgb;
	
	return color;
}

