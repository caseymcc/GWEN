
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Gwen/Gwen.h"
#include "Gwen/Skins/Simple.h"
#include "Gwen/Skins/TexturedBase.h"
#include "Gwen/UnitTest/UnitTest.h"
#include "Gwen/Input/Windows.h"

#ifdef USE_DEBUG_FONT
#include "Gwen/Renderers/OpenGL3_DebugFont.h"
#else
#include "Gwen/Renderers/OpenGL3.h"
#endif

#include "gl/glew.h"
#include "gl/wglew.h"
#pragma comment(lib, "glew32.lib")  
#pragma comment(lib, "opengl32.lib") 

HWND CreateGameWindow( void )
{
	LPCTSTR lpWindowName;
	WNDCLASSW	wc;
	ZeroMemory( &wc, sizeof( wc ) );
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= DefWindowProc;
	wc.hInstance		= GetModuleHandle( NULL );
	wc.lpszClassName	= L"GWENWindow";
	wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
	RegisterClassW( &wc );
#ifdef USE_DEBUG_FONT
	lpWindowName = L"GWEN - OpenGL3 Sample (Using embedded debug font renderer)";
#else
	lpWindowName = L"GWEN - OpenGL3 Sample (No cross platform way to render fonts in OpenGL)";
#endif
	HWND hWindow = CreateWindowExW( ( WS_EX_APPWINDOW | WS_EX_WINDOWEDGE ) , wc.lpszClassName, lpWindowName, ( WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN ) & ~( WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME ), -1, -1, 1004, 650, NULL, NULL, GetModuleHandle( NULL ), NULL );
	ShowWindow( hWindow, SW_SHOW );
	SetForegroundWindow( hWindow );
	SetFocus( hWindow );
	return hWindow;
}

HWND						g_pHWND = NULL;

int windowWidth  = 0;
int windowHeight = 0;
HGLRC CreateOpenGLDeviceContext()
{
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );    // just its size
	pfd.nVersion = 1;   // always 1
	pfd.dwFlags = PFD_SUPPORT_OPENGL |  // OpenGL support - not DirectDraw
				  PFD_DOUBLEBUFFER   |  // double buffering support
				  PFD_DRAW_TO_WINDOW;   // draw to the app window, not to a bitmap image
	pfd.iPixelType = PFD_TYPE_RGBA ;    // red, green, blue, alpha for each pixel
	pfd.iLayerType = PFD_MAIN_PLANE;    // Set the layer of the PFD  
	pfd.cColorBits = 24;                // 24 bit == 8 bits for red, 8 for green, 8 for blue.
	// This count of color bits EXCLUDES alpha.
	pfd.cDepthBits = 32;                // 32 bits to measure pixel depth.
	int pixelFormat = ChoosePixelFormat( GetDC( g_pHWND ), &pfd );

	if ( pixelFormat == 0 )
	{
		FatalAppExit( NULL, TEXT( "ChoosePixelFormat() failed!" ) );
	}

	//Set pixel format
	bool bResult = SetPixelFormat(GetDC(g_pHWND), pixelFormat, &pfd);
	if (!bResult)
	{
		FatalAppExit(NULL, TEXT("SetPixelFormat() failed!"));
	}

	// Create an OpenGL 2.1 context for our device context  
	HGLRC OpenGLContext = wglCreateContext(GetDC(g_pHWND));
	wglMakeCurrent(GetDC(g_pHWND), OpenGLContext);
	
	GLenum error = glewInit(); // Enable GLEW  
	if (error != GLEW_OK) // If GLEW fails  
	{
		FatalAppExit(NULL, TEXT("glewInit() failed!"));
	}
	
	int attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3, // Set the MAJOR version of OpenGL to 3  
		WGL_CONTEXT_MINOR_VERSION_ARB, 2, // Set the MINOR version of OpenGL to 2  
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, // Set our OpenGL context to be forward compatible  
		0
	};

	if (wglewIsSupported("WGL_ARB_create_context") == 1)  // If the OpenGL 3.x context creation extension is available
	{ 
		wglDeleteContext(OpenGLContext); // Delete the temporary OpenGL 2.1 context
		wglMakeCurrent(NULL, NULL); // Remove the temporary context from being active  

		OpenGLContext = wglCreateContextAttribsARB(GetDC(g_pHWND), NULL, attributes); // Create and OpenGL 3.x context based on the given attributes    
		wglMakeCurrent(GetDC(g_pHWND), OpenGLContext); // Make our OpenGL 3.0 context current  
	}
	else 
	{
		// If we didn't have support for OpenGL 3.x and up, use the OpenGL 2.1 context  
	}

	int glVersion[2] = { -1, -1 }; // Set some default values for the version  
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]); // Get back the OpenGL MAJOR version we are using  
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]); // Get back the OpenGL MAJOR version we are using  
	printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);

	/*
	RECT r;
	if ( GetClientRect( g_pHWND, &r ) )
	{
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( r.left, r.right, r.bottom, r.top, -1.0, 1.0 );
		glMatrixMode( GL_MODELVIEW );
		glViewport(0, 0, r.right - r.left, r.bottom - r.top);
	}
	*/

	RECT r;
	if (GetClientRect(g_pHWND, &r))
	{
		glViewport(0, 0, r.right - r.left, r.bottom - r.top);
	}

	windowWidth  = r.right - r.left;
	windowHeight = r.bottom - r.top;

	return OpenGLContext;
}


int main()
{
	//
	// Create a new window
	//
	g_pHWND = CreateGameWindow();
	//
	// Create OpenGL Device
	//
	HGLRC OpenGLContext = CreateOpenGLDeviceContext();
	//
	// Create a GWEN OpenGL Renderer
	//
#ifdef USE_DEBUG_FONT
	Gwen::Renderer::OpenGL3* pRenderer = new Gwen::Renderer::OpenGL3_DebugFont();
#else
	Gwen::Renderer::OpenGL3* pRenderer = new Gwen::Renderer::OpenGL3();
#endif
	pRenderer->Init(windowWidth, windowHeight);
	//
	// Create a GWEN skin
	//
	Gwen::Skin::TexturedBase* pSkin = new Gwen::Skin::TexturedBase( pRenderer );
	pSkin->Init( "DefaultSkin.png" );
	//
	// Create a Canvas (it's root, on which all other GWEN panels are created)
	//
	Gwen::Controls::Canvas* pCanvas = new Gwen::Controls::Canvas( pSkin );
	pCanvas->SetSize(998, 650 - 24);
	pCanvas->SetDrawBackground( true );
	pCanvas->SetBackgroundColor( Gwen::Color( 150, 170, 170, 255 ) );
	//
	// Create our unittest control (which is a Window with controls in it)
	//
	UnitTest* pUnit = new UnitTest( pCanvas );
	pUnit->SetPos( 10, 10 );
	//
	// Create a Windows Control helper
	// (Processes Windows MSG's and fires input at GWEN)
	//
	Gwen::Input::Windows GwenInput;
	GwenInput.Initialize( pCanvas );
	//
	// Begin the main game loop
	//

	//Gwen::Controls::WindowControl* pWindow = new Gwen::Controls::WindowControl(pCanvas);
	//pWindow->SetTitle(Gwen::Utility::Format(L"Hellow GWEN!", 1));
	//pWindow->SetSize(400, 400);
	//pWindow->SetPos(10, 10);
	//pWindow->SetDeleteOnClose(true);

	MSG msg;

	while ( true )
	{
		// Skip out if the window is closed
		if ( !IsWindowVisible( g_pHWND ) )
		{ break; }

		// If we have a message from windows..
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			// .. give it to the input handler to process
			GwenInput.ProcessMessage( msg );

			// if it's QUIT then quit..
			if ( msg.message == WM_QUIT )
			{ break; }

			// Handle the regular window stuff..
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		// Main OpenGL Render Loop
		{
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			pCanvas->RenderCanvas();
			SwapBuffers( GetDC( g_pHWND ) );
		}
	}

	// Clean up OpenGL
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( OpenGLContext );
	ReleaseDC(g_pHWND, GetDC(g_pHWND)); // Release the device context from our window  

	delete pCanvas;
	delete pSkin;
	delete pRenderer;
}
