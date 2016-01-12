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
#include "../../DXUT/Core/DXUT.h"
#include "../../DXUT/Core/DXUTmisc.h"
#include "../../DXUT/Optional/DXUTgui.h"
#include "../../DXUT/Optional/DXUTCamera.h"
#include "../../DXUT/Optional/DXUTSettingsDlg.h"
#include "../../DXUT/Optional/SDKmisc.h"
#include "../../DXUT/Optional/SDKmesh.h"
#include "../../DXUT/Core/DDSTextureLoader.h"
#include "../../AMD_SDK/inc/AMD_SDK.h"
#include "resource.h"
#include "SSAA.h"
#include "SampleLayoutControl.h"

#pragma warning( disable : 4100 ) // disable unreference formal parameter warnings for /W4 builds

#define USE_MAGNIFY		// Magnify Tool doesn't currently work with PIX for Windows -so disable this for PIX debugging


CFirstPersonCamera          g_Camera;                   // A model viewing camera
CDXUTDialogResourceManager  g_DialogResourceManager;    // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;              // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTSDKMesh				g_SceneMesh;

enum
{
	IDC_DIRECT3D_STATIC_CAPTION,
	IDC_TOGGLEFULLSCREEN,
	IDC_CHANGEDEVICE,
	IDC_SETTINGS_LABEL,
	IDC_SCENE_TITLE,
	IDC_SCENE_COMBO,
	IDC_SSAA_TYPE_LABEL,
	IDC_SSAA_TYPE,
	IDC_RENDER_TARGET_LABEL,
	IDC_RENDER_TARGET,
	IDC_LAYOUT_TITLE,
	IDC_LAYOUT_CONTROL,
	IDC_LAYOUT_KEY_1,
	IDC_LAYOUT_KEY_2,
	IDC_LAYOUT_KEY_3,
	IDC_LAYOUT_KEY_4,
	IDC_MAGNIFY_TITLE,
};

static CDXUTComboBox*				g_SceneSelectCombo = 0;
static CDXUTComboBox*				g_SSAATypeCombo = 0;
static CDXUTCheckBox*				g_TemporalAACheckBox = 0;
static CDXUTComboBox*				g_RenderTargetCombo = 0;

#if defined (USE_MAGNIFY)
static AMD::MagnifyTool				g_MagnifyTool;
#endif
static AMD::HUD						g_HUD;
static AMD::Sprite					g_Sprite;
static bool							g_bRenderHUD = true;
static SSAA							g_SSAA;
static ID3D11ShaderResourceView*	g_CircleTexture = 0;
static ID3D11ShaderResourceView*	g_CrossTexture = 0;
static SampleLayoutControl*			g_SampleLayoutControl = 0;
static bool							g_EQAASupported = false;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Puts the camera in the default location depending on which scene we are showing
//--------------------------------------------------------------------------------------
void SetUpCameraForScene()
{
	// Camera set up for main scene
	if ( g_SSAA.GetSceneType() == SSAA::TypicalScene )
	{
		g_Camera.SetViewParams( DirectX::XMVectorSet( 210.0f, 134.3f, -240.2f, 1.0f ), DirectX::XMVectorSet( 209.5f,134.6f,-239.3f, 1.0f ) );

		g_Camera.SetScalers( 0.01f, 125.0f );
	}
	else
	{
		// Camera set up for stress test
		g_Camera.SetViewParams( DirectX::XMVectorSet( 10.0f, 3.0f, 10.0f, 1.0f ), DirectX::XMVectorSet( -3.0f, 4.0f, 4.0f ,1.0f ) );

		g_Camera.SetScalers( 0.01f, 10.0f );
	}

	DirectX::XMFLOAT3 vMin = DirectX::XMFLOAT3( -1000.0f, -1000.0f, -1000.0f );
    DirectX::XMFLOAT3 vMax = DirectX::XMFLOAT3( 1000.0f, 1000.0f, 1000.0f );
    
    g_Camera.SetRotateButtons(TRUE, FALSE, FALSE);
    g_Camera.SetDrag( true );
    g_Camera.SetEnableYAxisMovement( true );
    g_Camera.SetClipToBoundary( TRUE, &vMin, &vMax );
    g_Camera.FrameMove( 0 );
}


//--------------------------------------------------------------------------------------
// Make sure we check for hardware support of EQAA modes before adding to the options
//--------------------------------------------------------------------------------------
void CheckForEQAASupport()
{
	// Get DXGI device from D3D11 device
	IDXGIDevice* pDXGIDevice = 0;
	DXUTGetD3D11Device()->QueryInterface( __uuidof(IDXGIDevice), (void **)&pDXGIDevice );
	if ( pDXGIDevice )
	{
		// Get DXGI adpater from DXGI device
		IDXGIAdapter* pDXGIAdapter = 0;
		pDXGIDevice->GetParent( __uuidof(IDXGIAdapter), (void **)&pDXGIAdapter );
		if ( pDXGIAdapter )
		{
			// Query DXGI adapter for vendor id
			DXGI_ADAPTER_DESC desc;
			pDXGIAdapter->GetDesc( &desc );
			if ( desc.VendorId == VENDOR_ID_AMD )
			{
				const DXGI_FORMAT formats[] = 
				{
					DXGI_FORMAT_R8G8B8A8_UNORM,
					DXGI_FORMAT_R10G10B10A2_UNORM,
					DXGI_FORMAT_R16G16B16A16_FLOAT
				};

				struct SampleAndQuality
				{
					UINT samples; 
					UINT qualityLevels;
				};

				const SampleAndQuality sampleAndQualities[] = 
				{
					{ 2, 4 },
					{ 4, 8 },
					{ 8, 16 }
				};

				// By default assume we support EQAA unless CheckMultisampleQualityLevels tells us otherwise
				bool supported = true;

				// Check all formats that the demo supports
				for ( int i = 0; i < ARRAYSIZE( formats ); i++ )
				{
					// Check all sample/quality combos
					for ( int j = 0; j < ARRAYSIZE( sampleAndQualities ); j++ )
					{
						UINT qualityLevels = 0;
						if ( DXUTGetD3D11Device()->CheckMultisampleQualityLevels( formats[ i ], sampleAndQualities[ j ].samples, &qualityLevels ) == S_OK )
						{
							// EQAA supported if returned quality level is higher. eg for EQAA 2f4x quality must be > 4
							if ( qualityLevels <= sampleAndQualities[ j ].qualityLevels )
							{
								supported = false;
								break;
							}
						}
						else
						{
							supported = false;
							break;
						}
					}
				}
				
				// If all modes are supported, then add them to the drop down combo box
				if ( supported ) 
				{
					g_SSAATypeCombo->AddItem( L"2f4x EQAA", NULL );
					g_SSAATypeCombo->AddItem( L"4f8x EQAA", NULL );
					g_SSAATypeCombo->AddItem( L"8f16x EQAA", NULL );
					g_EQAASupported = true;
				}
			}

			// Dont forget to free up references to DXGI objects
			pDXGIAdapter->Release();
		}

		pDXGIDevice->Release();
	}
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );

	InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"SSAA11 v1.2" );

    // Require D3D_FEATURE_LEVEL_11_0
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1920, 1080 );

    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    D3DCOLOR DlgColor = 0x88888888; // Semi-transparent background for the dialog
   
	// Register resource managers with UI
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.m_GUI.Init( &g_DialogResourceManager );
    g_HUD.m_GUI.SetBackgroundColors( DlgColor );
    g_HUD.m_GUI.SetCallback( OnGUIEvent );

	// Disable the backbuffer AA as we are rendering to an intermediate target for our 3d scene
	g_SettingsDlg.GetDialogControl()->GetControl( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_COUNT )->SetEnabled( false );
	g_SettingsDlg.GetDialogControl()->GetControl( DXUTSETTINGSDLG_D3D11_MULTISAMPLE_QUALITY )->SetEnabled( false );

    int iY = 0;

    g_HUD.m_GUI.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
    g_HUD.m_GUI.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight, VK_F2 );
	iY += AMD::HUD::iGroupDelta / 2;

	// Options UI
	g_HUD.m_GUI.AddStatic( IDC_SETTINGS_LABEL, L"-- Options --", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddStatic( IDC_SCENE_TITLE, L"Scene Selection", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddComboBox( IDC_SCENE_COMBO, AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth + 20, AMD::HUD::iElementHeight, 0, true, &g_SceneSelectCombo );
	if( g_SceneSelectCombo )
	{
		g_SceneSelectCombo->SetDropHeight( 40 );
		g_SceneSelectCombo->AddItem( L"Typical Scene", NULL );
		g_SceneSelectCombo->AddItem( L"Alpha Stress Test", NULL );
		g_SceneSelectCombo->SetSelectedByIndex( g_SSAA.GetSceneType() );
	}
	
	g_HUD.m_GUI.AddStatic( IDC_SSAA_TYPE_LABEL, L"AA Type", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth + 20, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddComboBox( IDC_SSAA_TYPE, AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth + 20, AMD::HUD::iElementHeight, 0, true, &g_SSAATypeCombo );
	if( g_SSAATypeCombo )
	{
		g_SSAATypeCombo->SetDropHeight( 100 );
		g_SSAATypeCombo->AddItem( L"None", NULL );
		g_SSAATypeCombo->AddItem( L"2x MSAA", NULL );
		g_SSAATypeCombo->AddItem( L"2x SSAA (Horiz.)", NULL );
		g_SSAATypeCombo->AddItem( L"2x SSAA (Vert.)", NULL );
		g_SSAATypeCombo->AddItem( L"2x SSAA SF", NULL );
		g_SSAATypeCombo->AddItem( L"1.5x1.5 SSAA", NULL );
		g_SSAATypeCombo->AddItem( L"4x MSAA", NULL );
		g_SSAATypeCombo->AddItem( L"4x SSAA", NULL );
		g_SSAATypeCombo->AddItem( L"4x SSAA SF", NULL );
		g_SSAATypeCombo->AddItem( L"4x SSAA RG", NULL );
		g_SSAATypeCombo->AddItem( L"8x MSAA", NULL );
		g_SSAATypeCombo->AddItem( L"8x SSAA SF", NULL );
		
		g_SSAATypeCombo->SetSelectedByIndex( g_SSAA.GetAAType() );
	}

	g_HUD.m_GUI.AddStatic( IDC_RENDER_TARGET_LABEL, L"Render Target Format", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth + 20, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddComboBox( IDC_RENDER_TARGET, AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth + 20, AMD::HUD::iElementHeight, 0, true, &g_RenderTargetCombo );
	if( g_RenderTargetCombo )
	{
		g_RenderTargetCombo->SetDropHeight( 50 );
		g_RenderTargetCombo->AddItem( L"R8G8B8A8", NULL );
		g_RenderTargetCombo->AddItem( L"R10G10B10A2", NULL );
		g_RenderTargetCombo->AddItem( L"R16G16B16A16F", NULL );
		g_SSAATypeCombo->SetSelectedByIndex( g_SSAA.GetRenderTargetType() );
	}

	iY += 15;
	
	// Sample pattern layout visualization
	g_HUD.m_GUI.AddStatic( IDC_LAYOUT_TITLE, L"-- Sample Layout --", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	iY += 20;

	g_SampleLayoutControl = new SampleLayoutControl( &g_HUD.m_GUI, g_SSAA, g_Sprite );
	g_HUD.m_GUI.AddControl( g_SampleLayoutControl );
	g_SampleLayoutControl->SetLocation( AMD::HUD::iElementOffset, iY += AMD::HUD::iElementWidth );
	g_SampleLayoutControl->SetSize( AMD::HUD::iElementWidth, AMD::HUD::iElementWidth );
	g_SampleLayoutControl->SetID( IDC_LAYOUT_CONTROL );

	iY -= 18;
	g_HUD.m_GUI.AddStatic( IDC_LAYOUT_KEY_1, L"= Pixel boundary", 70, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddStatic( IDC_LAYOUT_KEY_2, L"= Color sample", 70, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddStatic( IDC_LAYOUT_KEY_3, L"= Coverage sample", 70, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	g_HUD.m_GUI.AddStatic( IDC_LAYOUT_KEY_4, L"= Resolve location", 70, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	
	iY += AMD::HUD::iGroupDelta / 2;

	// Magnifying glass tool
	g_HUD.m_GUI.AddStatic( IDC_MAGNIFY_TITLE, L"-- Magnifying Glass --", AMD::HUD::iElementOffset, iY += AMD::HUD::iElementDelta, AMD::HUD::iElementWidth, AMD::HUD::iElementHeight );
	iY += AMD::HUD::iElementDelta;

#if defined (USE_MAGNIFY)
	// Add the magnify tool UI to our HUD
	g_MagnifyTool.InitApp( &g_HUD.m_GUI, iY, true );
#endif

	SetUpCameraForScene();

	// Polling the gamepad is expensive and we want to be GPU bound for this sample
	//g_Camera.EnableGamePad( false ); 
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( DirectX::XMVectorSet( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
	g_pTxtHelper->DrawTextLine( g_SSAA.GetDescription() );

	static int		s_counter = 0;
	static float	s_SceneCostAcc = 0.0f;
	static float	s_ResolveCostAcc = 0.0f;
	static float	s_SceneCost = 0.0f;
	static float	s_ResolveCost = 0.0f;

	s_SceneCostAcc += (float)TIMER_GetTime( Gpu, L"Scene" ) * 1000.0f;
	s_ResolveCostAcc += (float)TIMER_GetTime( Gpu, L"AA Resolve" ) * 1000.0f;
	s_counter++;
	if ( s_counter > 10 )
	{
		s_SceneCost = s_SceneCostAcc / (float)s_counter;
		s_ResolveCost = s_ResolveCostAcc / (float)s_counter;
		s_SceneCostAcc = 0.0f;
		s_ResolveCostAcc = 0.0f;
		s_counter = 0;
	}
	
	WCHAR wcbuf[256];
	swprintf_s( wcbuf, 256, L"Cost in milliseconds( Scene = %.2f, Resolve = %.2f )", s_SceneCost, s_ResolveCost );
	g_pTxtHelper->DrawTextLine( wcbuf );
	g_pTxtHelper->DrawTextLine( L"" );
	g_pTxtHelper->DrawTextLine( g_SSAA.GetAADescription() );

    g_pTxtHelper->SetInsertionPos( 5, DXUTGetDXGIBackBufferSurfaceDesc()->Height - 2*AMD::HUD::iElementDelta );
	g_pTxtHelper->DrawTextLine( L"Toggle GUI             : F1" );
	g_pTxtHelper->DrawTextLine( L"Cycle through AA Modes : +/-" );

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;
	
    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_SettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	// Load the textures for the sample layout visualization
	V( DirectX::CreateDDSTextureFromFile( pd3dDevice, L"..\\Media\\circle.dds", nullptr, &g_CircleTexture ) );
	V( DirectX::CreateDDSTextureFromFile( pd3dDevice, L"..\\Media\\cross.dds", nullptr, &g_CrossTexture ) );

	g_SampleLayoutControl->SetTextures( g_CircleTexture, g_CrossTexture );
   
	// Load the main scene
	V( g_SceneMesh.Create( pd3dDevice, L"SquidRoom\\SquidRoom.sdkmesh" ) );

	// Init the SSAA class
	g_SSAA.Init( pd3dDevice, pd3dImmediateContext, &g_SceneMesh, g_Camera );
	g_SSAA.OnResize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    
    // Create AMD_SDK resources here
    g_HUD.OnCreateDevice( pd3dDevice );
#if defined (USE_MAGNIFY)    
	g_MagnifyTool.OnCreateDevice( pd3dDevice );
#endif
    TIMER_Init( pd3dDevice )
    g_Sprite.OnCreateDevice( pd3dDevice );
   	
	// Only need to check for EQAA support once
	static bool bEQAAChecked = false;
	if ( !bEQAAChecked ) 
	{
		CheckForEQAASupport();
		bEQAAChecked = true;
	}

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_SettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( DirectX::XM_PI / 4, fAspectRatio, 1.0f, 3000.0f );
    
    // Set the location and size of the AMD standard HUD
    g_HUD.m_GUI.SetLocation( pBackBufferSurfaceDesc->Width - AMD::HUD::iDialogWidth, 0 );
    g_HUD.m_GUI.SetSize( AMD::HUD::iDialogWidth, pBackBufferSurfaceDesc->Height );
    g_HUD.OnResizedSwapChain( pBackBufferSurfaceDesc );

    g_Sprite.OnResizedSwapChain( pBackBufferSurfaceDesc );

#if defined (USE_MAGNIFY)
    // Magnify tool will capture from the color buffer
    g_MagnifyTool.OnResizedSwapChain( pd3dDevice, pSwapChain, pBackBufferSurfaceDesc, pUserContext, 
        pBackBufferSurfaceDesc->Width - AMD::HUD::iDialogWidth, 0 );
    D3D11_RENDER_TARGET_VIEW_DESC RTDesc;
    ID3D11Resource* pTempRTResource;
    DXUTGetD3D11RenderTargetView()->GetResource( &pTempRTResource );
    DXUTGetD3D11RenderTargetView()->GetDesc( &RTDesc );
    g_MagnifyTool.SetSourceResources( pTempRTResource, RTDesc.Format, 
                DXUTGetDXGIBackBufferSurfaceDesc()->Width, DXUTGetDXGIBackBufferSurfaceDesc()->Height,
                DXUTGetDXGIBackBufferSurfaceDesc()->SampleDesc.Count );
    g_MagnifyTool.SetPixelRegion( 128 );
    g_MagnifyTool.SetScale( 5 );
    SAFE_RELEASE( pTempRTResource );
#endif
	
	g_SSAA.OnResize( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    // Reset the timer at start of frame
    TIMER_Reset()

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }       

	// Render the scene through the SSAA class. This sets the render target, renders the appropriate scene and resolves the scene to the backbuffer.
	g_SSAA.Render( DXUTGetD3D11RenderTargetView(), DXUTGetD3D11DepthStencilView() );

	// Render HUD and other text to the backbuffer
	TIMER_Begin( 0, L"HUD" );
	if( g_bRenderHUD )
    {
#if defined (USE_MAGNIFY)
        g_MagnifyTool.Render();
#endif
        g_HUD.OnRender( fElapsedTime );
		RenderText();
    }
	TIMER_End();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_SettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

	// Free up main scene mesh
    g_SceneMesh.Destroy();
    
	// Free up sample layout textures
	SAFE_RELEASE( g_CircleTexture );
	SAFE_RELEASE( g_CrossTexture );

	// Free up any resources allocated by the SSAA class
	g_SSAA.DeInit();

    // Destroy AMD_SDK resources here
	g_HUD.OnDestroyDevice();
#if defined (USE_MAGNIFY)
    g_MagnifyTool.OnDestroyDevice();
#endif
    TIMER_Destroy()
    g_Sprite.OnDestroyDevice();
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // Disable vsync for profiling purposes
    pDeviceSettings->d3d11.SyncInterval = 0;

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.m_GUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    
    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
			case VK_F1:
				g_bRenderHUD = !g_bRenderHUD;
				break;

			case VK_ADD:
				if ( ( g_EQAASupported && g_SSAA.GetAAType() < SSAA::Max ) || g_SSAA.GetAAType() < SSAA::SSAAx8SF )
				{
					g_SSAA.SetAAType( (SSAA::Type)( g_SSAA.GetAAType() + 1 ) );
					g_SSAATypeCombo->SetSelectedByIndex( g_SSAA.GetAAType() );
				}
				break;

			case VK_SUBTRACT:
				g_SSAA.SetAAType( (SSAA::Type)( g_SSAA.GetAAType() - 1 ) );
				g_SSAATypeCombo->SetSelectedByIndex( g_SSAA.GetAAType() );
				break;
		}
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() );
            break;
		
		case IDC_SCENE_COMBO:
			g_SSAA.SetScene( (SSAA::SceneType)g_SceneSelectCombo->GetSelectedIndex() );
			SetUpCameraForScene();
			break;

		case IDC_SSAA_TYPE:
			g_SSAA.SetAAType( (SSAA::Type)g_SSAATypeCombo->GetSelectedIndex() );
			break;

		case IDC_RENDER_TARGET:
			g_SSAA.SetRenderTargetFormat( (SSAA::RenderTargetFormat)g_RenderTargetCombo->GetSelectedIndex() );
			break;
    }

#if defined (USE_MAGNIFY)
    // Call the MagnifyTool gui event handler
    g_MagnifyTool.OnGUIEvent( nEvent, nControlID, pControl, pUserContext );
#endif
}
