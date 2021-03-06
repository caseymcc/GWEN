
#include "Gwen/Renderers/OpenGL3.h"
#include "Gwen/Utility.h"
#include "Gwen/Font.h"
#include "Gwen/Texture.h"
#include "Gwen/WindowProvider.h"

#include <math.h>

#include "FreeImage/FreeImage.h"


namespace Gwen
{
	namespace Renderer
	{
		OpenGL3::OpenGL3()
		{
			m_iVertNum = 0;
			m_pContext = NULL;
			::FreeImage_Initialise();

			for ( int i = 0; i < MaxVerts; i++ )
			{
				m_Vertices[ i ].z = 0.5f;
			}

			VAO = 0;
			VBO = 0;
			Program = 0;
			//ProgramProjectionLocation = 0;
			ProgramTextureLocation    = 0;

		}

		OpenGL3::~OpenGL3()
		{
			::FreeImage_DeInitialise();

			if (VAO)
			{
				glDeleteBuffers(1, &VBO);
				glDeleteVertexArrays(1, &VAO);
				VAO = 0;
			}

			if (Program)
			{
				glDeleteProgram(Program);
				Program = 0;
			}

		}

		void OpenGL3::Init(int _windowWidth, int _windowHeight)
		{
			windowWidth = _windowWidth;
			windowHeight =_windowHeight;

			getOpenGlExtensions();

			//Generate VertexArrayObject
			glGenVertexArrays(1, &VAO);
			glBindVertexArray(VAO);
			
			//Generate VertexBufferObject
			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);

			glBufferData(GL_ARRAY_BUFFER, MaxVerts * 9 * sizeof(GLfloat), 0, GL_DYNAMIC_READ);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (9 * sizeof(GLfloat)), (void*)0);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, (9 * sizeof(GLfloat)), (void*)(3 * sizeof(GLfloat)));
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (9 * sizeof(GLfloat)), (void*)(7 * sizeof(GLfloat)));

			//Unbind VAO!
			glBindVertexArray(0);

			GLint Result = GL_FALSE;
			int InfoLogLength;

			Program = glCreateProgram();

			const char * vs =
				"#version 330 core\n"
				"layout(location = 0) in vec3 VertexPosition;\n"
				"layout(location = 1) in vec4 VertexColor;\n"
				"layout(location = 2) in vec2 VertexTexCoord;\n"
				"out vec2 texCoord;\n"
				"out vec4 vertexColor;\n"
				//"uniform mat4 Projection;\n"
				"uniform vec2 Viewport;\n"
				"void main()\n"
				"{\n"
				"    vertexColor = VertexColor;\n"
				"    texCoord    = VertexTexCoord;\n"
				"    vec2 p = ( VertexPosition.xy * (2.0 / Viewport) ) - 1.0;\n"
				"    gl_Position = vec4(p, VertexPosition.z, 1.0);\n"
				"}\n";
			GLuint vso = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vso, 1, (const char **)&vs, NULL);
			glCompileShader(vso);
			
			//VERTEX SHADER ERROR
			glGetShaderiv(vso, GL_COMPILE_STATUS, &Result);
			glGetShaderiv(vso, GL_INFO_LOG_LENGTH, &InfoLogLength);
			if (InfoLogLength > 0)
			{
				std::vector<char> VertexShaderErrorMessage(InfoLogLength);
				glGetShaderInfoLog(vso, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
				fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);
			}

			const char * fs=
				"#version 330 core\n"
				"in vec2 texCoord;\n"
				"in vec4 vertexColor;\n"
				"uniform sampler2D Texture;\n"
				"uniform float TextureEnabled;\n"
				"out vec4 fragColor;\n"
				"void main()\n"
				"{\n"
				"       vec4 tex = texture2D(Texture, texCoord.xy);\n"
				"		fragColor   = mix(vertexColor, tex, TextureEnabled);\n"
				"}\n";

			GLuint fso = glCreateShader(GL_FRAGMENT_SHADER);

			glShaderSource(fso, 1, (const char **)&fs, NULL);
			glCompileShader(fso);
			
			//FRAGMENT SHADER ERROR
			glGetShaderiv(fso, GL_COMPILE_STATUS, &Result);
			glGetShaderiv(fso, GL_INFO_LOG_LENGTH, &InfoLogLength);
			if (InfoLogLength > 0)
			{
				std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
				glGetShaderInfoLog(fso, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
				fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);
			}

			glAttachShader(Program, vso);
			glAttachShader(Program, fso);

			glLinkProgram(Program);

			//Program LINK ERROR
			glGetProgramiv(Program, GL_LINK_STATUS, &Result);
			glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &InfoLogLength);
			if (InfoLogLength > 0)
			{
				std::vector<char> ProgramErrorMessage(std::max(InfoLogLength, int(1)));
				glGetProgramInfoLog(Program, InfoLogLength, NULL, &ProgramErrorMessage[0]);
				fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);
			}

			glDeleteShader(vso);
			glDeleteShader(fso);

			glUseProgram(Program);
			ProgramViewportLocation   = glGetUniformLocation(Program, "Viewport");
			//ProgramProjectionLocation = glGetUniformLocation(Program, "Projection");
			ProgramTextureLocation    = glGetUniformLocation(Program, "Texture");
			ProgramTextureEnabledLocation = glGetUniformLocation(Program, "TextureEnabled");
			glUniform1f(ProgramTextureEnabledLocation, 0.0f);
			glUseProgram(0);

		}

		void OpenGL3::Begin()
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glUseProgram(Program);
			glUniform2f(ProgramViewportLocation, (float)windowWidth, (float)windowHeight);

			glActiveTexture(GL_TEXTURE0);
			glUniform1i(ProgramTextureLocation, 0);

			//Bind VAO
			glBindVertexArray(VAO);

			//Bind VBO
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);

		}

		void OpenGL3::End()
		{
			Flush();

			//Unbind VBO
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);

			//Unbind VAO!
			glBindVertexArray(0);
		}

		void OpenGL3::Flush()
		{
			if ( m_iVertNum == 0 ) { return; }

			GLvoid* VBO_Map = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			memcpy(VBO_Map, &vertexBufferData[0], vertexBufferData.size() * sizeof(GLfloat));
			glUnmapBuffer(GL_ARRAY_BUFFER);

			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_iVertNum );

			vertexBufferData.clear();
			m_iVertNum = 0;
		}

		void OpenGL3::AddVert( int x, int y, float u, float v )
		{
			if ( m_iVertNum >= MaxVerts - 1 )
			{
				Flush();
			}

			size_t index=vertexBufferData.size();
			vertexBufferData.resize(vertexBufferData.size()+9);
			
			//Position
			vertexBufferData[index++]=(float)x;
			vertexBufferData[index++]=(float)windowHeight - y;
			vertexBufferData[index++]=0.5f;

			//Color
			vertexBufferData[index++]=(1.0f / 255.0f)*(float)m_Color.r;
			vertexBufferData[index++]=(1.0f / 255.0f)*(float)m_Color.g;
			vertexBufferData[index++]=(1.0f / 255.0f)*(float)m_Color.b;
			vertexBufferData[index++]=(1.0f / 255.0f)*(float)m_Color.a;

			//UV
			vertexBufferData[index++]=u;
			vertexBufferData[index++]=v;

			m_iVertNum++;
		}

		void OpenGL3::DrawFilledRect( Gwen::Rect rect )
		{
			if(m_textureEnabled)
			{
				Flush();
				glDisable( GL_TEXTURE_2D );
				m_textureEnabled=false;
				glUniform1f(ProgramTextureEnabledLocation, 0.0f);
			}

			Translate( rect );
			AddVert( rect.x, rect.y );
			AddVert( rect.x + rect.w, rect.y );
			AddVert( rect.x, rect.y + rect.h );
			AddVert( rect.x + rect.w, rect.y );
			AddVert( rect.x + rect.w, rect.y + rect.h );
			AddVert( rect.x, rect.y + rect.h );
			//Flush();
		}

		void OpenGL3::SetDrawColor( Gwen::Color color )
		{
			//glColor4ubv( ( GLubyte* ) &color );
			m_Color = color;
		}

		void OpenGL3::StartClip()
		{
			Flush();
			Gwen::Rect rect = ClipRegion();
			// OpenGL's coords are from the bottom left
			// so we need to translate them here.
			{
				GLint view[4];
				glGetIntegerv( GL_VIEWPORT, &view[0] );
				rect.y = view[3] - ( rect.y + rect.h );
			}
			glScissor( rect.x * Scale(), rect.y * Scale(), rect.w * Scale(), rect.h * Scale() );
			glEnable( GL_SCISSOR_TEST );
		};

		void OpenGL3::EndClip()
		{
			Flush();
			glDisable( GL_SCISSOR_TEST );
		};

		void OpenGL3::DrawTexturedRect( Gwen::Texture* pTexture, Gwen::Rect rect, float u1, float v1, float u2, float v2 )
		{
			GLuint* tex = ( GLuint* ) pTexture->data;

			// Missing image, not loaded properly?
			if ( !tex )
			{
				return DrawMissingImage( rect );
			}

			Translate( rect );

			if(!m_textureEnabled || m_currentTexture != *tex)
			{
				Flush();
				glBindTexture( GL_TEXTURE_2D, *tex );
				glEnable( GL_TEXTURE_2D );
				glUniform1f(ProgramTextureEnabledLocation, 1.0f);

				m_textureEnabled=true;
				m_currentTexture=*tex;
			}

			AddVert( rect.x, rect.y,			u1, v1 );
			AddVert( rect.x + rect.w, rect.y,		u2, v1 );
			AddVert( rect.x, rect.y + rect.h,	u1, v2 );
			AddVert( rect.x + rect.w, rect.y,		u2, v1 );
			AddVert( rect.x + rect.w, rect.y + rect.h, u2, v2 );
			AddVert( rect.x, rect.y + rect.h, u1, v2 );
		}

		void OpenGL3::LoadTexture( Gwen::Texture* pTexture )
		{
			const wchar_t* wFileName = pTexture->name.GetUnicode().c_str();
			FREE_IMAGE_FORMAT imageFormat = FreeImage_GetFileTypeU( wFileName );

			if ( imageFormat == FIF_UNKNOWN )
			{ imageFormat = FreeImage_GetFIFFromFilenameU( wFileName ); }

			// Image failed to load..
			if ( imageFormat == FIF_UNKNOWN )
			{
				pTexture->failed = true;
				return;
			}

			// Try to load the image..
			FIBITMAP* bits = FreeImage_LoadU( imageFormat, wFileName );

			if ( !bits )
			{
				pTexture->failed = true;
				return;
			}

			// Convert to 32bit
			FIBITMAP* bits32 = FreeImage_ConvertTo32Bits( bits );
			FreeImage_Unload( bits );

			if ( !bits32 )
			{
				pTexture->failed = true;
				return;
			}

			// Flip
			::FreeImage_FlipVertical( bits32 );
			// Create a little texture pointer..
			GLuint* pglTexture = new GLuint;
			// Sort out our GWEN texture
			pTexture->data = pglTexture;
			pTexture->width = FreeImage_GetWidth( bits32 );
			pTexture->height = FreeImage_GetHeight( bits32 );
			// Create the opengl texture
			glGenTextures( 1, pglTexture );
			glBindTexture( GL_TEXTURE_2D, *pglTexture );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#ifdef FREEIMAGE_BIGENDIAN
			GLenum format = GL_RGBA;
#else
			GLenum format = GL_BGRA;
#endif
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, pTexture->width, pTexture->height, 0, format, GL_UNSIGNED_BYTE, ( const GLvoid* ) FreeImage_GetBits( bits32 ) );
			FreeImage_Unload( bits32 );
		}

		void OpenGL3::FreeTexture( Gwen::Texture* pTexture )
		{
			GLuint* tex = ( GLuint* ) pTexture->data;

			if ( !tex ) { return; }

			glDeleteTextures( 1, tex );
			delete tex;
			pTexture->data = NULL;
		}

		Gwen::Color OpenGL3::PixelColour( Gwen::Texture* pTexture, unsigned int x, unsigned int y, const Gwen::Color & col_default )
		{
			GLuint* tex = ( GLuint* ) pTexture->data;

			if ( !tex ) { return col_default; }

			unsigned int iPixelSize = sizeof( unsigned char ) * 4;
			glBindTexture( GL_TEXTURE_2D, *tex );
			unsigned char* data = ( unsigned char* ) malloc( iPixelSize * pTexture->width * pTexture->height );
			glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
			unsigned int iOffset = ( y * pTexture->width + x ) * 4;
			Gwen::Color c;
			c.r = data[0 + iOffset];
			c.g = data[1 + iOffset];
			c.b = data[2 + iOffset];
			c.a = data[3 + iOffset];
			//
			// Retrieving the entire texture for a single pixel read
			// is kind of a waste - maybe cache this pointer in the texture
			// data and then release later on? It's never called during runtime
			// - only during initialization.
			//
			free( data );
			return c;
		}

///////////////////////////////////////////////////////////////////////////////////////////////////////

		bool OpenGL3::InitializeContext( Gwen::WindowProvider* pWindow )
		{
#ifdef _WIN32
			HWND pHwnd = ( HWND ) pWindow->GetWindow();

			if ( !pHwnd ) { return false; }

			HDC hDC = GetDC( pHwnd );
			//
			// Set the pixel format
			//
			PIXELFORMATDESCRIPTOR pfd;
			memset( &pfd, 0, sizeof( pfd ) );
			pfd.nSize = sizeof( pfd );
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.cColorBits = 24;
			pfd.cDepthBits = 32;
			pfd.iLayerType = PFD_MAIN_PLANE;
			int iFormat = ChoosePixelFormat( hDC, &pfd );
			SetPixelFormat( hDC, iFormat, &pfd );
			HGLRC hRC;
			hRC = wglCreateContext( hDC );
			wglMakeCurrent( hDC, hRC );
			RECT r;

			if ( GetClientRect( pHwnd, &r ) )
			{
				glMatrixMode( GL_PROJECTION );
				glLoadIdentity();
				glOrtho( r.left, r.right, r.bottom, r.top, -1.0, 1.0 );
				glMatrixMode( GL_MODELVIEW );
				glViewport( 0, 0, r.right - r.left, r.bottom - r.top );
			}

			m_pContext = ( void* ) hRC;

			return true;
#endif
			return false;
		}

		bool OpenGL3::ShutdownContext( Gwen::WindowProvider* pWindow )
		{
#ifdef _WIN32
			wglDeleteContext( ( HGLRC ) m_pContext );
			return true;
#endif
			return false;
		}

		bool OpenGL3::PresentContext( Gwen::WindowProvider* pWindow )
		{
#ifdef _WIN32
			HWND pHwnd = ( HWND ) pWindow->GetWindow();

			if ( !pHwnd ) { return false; }

			HDC hDC = GetDC( pHwnd );
			SwapBuffers( hDC );
			return true;
#endif
			return false;
		}

		void *OpenGL3::getOpenGlExtension(std::string funcName)
		{
			void *funcPtr=NULL;
#ifdef _WIN32
			funcPtr=(void *)wglGetProcAddress(funcName.c_str());

			if((funcPtr == 0) || (funcPtr == (void *)0x1) || (funcPtr == (void *)0x2) || (funcPtr == (void *)0x3) || (funcPtr == (void *)-1))
			{
				return NULL;
				//		HMODULE module=LoadLibraryA("opengl32.dll");
				//		funcPtr=(void *)GetProcAddress(module, name);
			}
#else
#endif
			return funcPtr;
		}

		void OpenGL3::getOpenGlExtensions()
		{
			glActiveTexture=(PFNGLACTIVETEXTUREPROC)getOpenGlExtension("glActiveTexture");
			glAttachShader=(PFNGLATTACHSHADERPROC)getOpenGlExtension("glAttachShader");
			glBindBuffer=(PFNGLBINDBUFFERPROC)getOpenGlExtension("glBindBuffer");
			glBindVertexArray=(PFNGLBINDVERTEXARRAYPROC)getOpenGlExtension("glBindVertexArray");
			glBufferData=(PFNGLBUFFERDATAPROC)getOpenGlExtension("glBufferData");
			glCompileShader=(PFNGLCOMPILESHADERPROC)getOpenGlExtension("glCompileShader");
			glCreateProgram=(PFNGLCREATEPROGRAMPROC)getOpenGlExtension("glCreateProgram");
			glCreateShader=(PFNGLCREATESHADERPROC)getOpenGlExtension("glCreateShader");
			glDeleteBuffers=(PFNGLDELETEBUFFERSPROC)getOpenGlExtension("glDeleteBuffers");
			glDeleteShader=(PFNGLDELETESHADERPROC)getOpenGlExtension("glDeleteShader");
			glDeleteProgram=(PFNGLDELETEPROGRAMPROC)getOpenGlExtension("glDeleteProgram");
			glDeleteVertexArrays=(PFNGLDELETEVERTEXARRAYSPROC)getOpenGlExtension("glDeleteVertexArrays");
			glDisableVertexAttribArray=(PFNGLDISABLEVERTEXATTRIBARRAYPROC)getOpenGlExtension("glDisableVertexAttribArray");
			glEnableVertexAttribArray=(PFNGLENABLEVERTEXATTRIBARRAYPROC)getOpenGlExtension("glEnableVertexAttribArray");
			glGenBuffers=(PFNGLGENBUFFERSPROC)getOpenGlExtension("glGenBuffers");
			glGenVertexArrays=(PFNGLGENVERTEXARRAYSPROC)getOpenGlExtension("glGenVertexArrays");
			glGetProgramiv=(PFNGLGETPROGRAMIVPROC)getOpenGlExtension("glGetProgramiv");
			glGetProgramInfoLog=(PFNGLGETPROGRAMINFOLOGPROC)getOpenGlExtension("glGetProgramInfoLog");
			glGetShaderInfoLog=(PFNGLGETSHADERINFOLOGPROC)getOpenGlExtension("glGetShaderInfoLog");
			glGetShaderiv=(PFNGLGETSHADERIVPROC)getOpenGlExtension("glGetShaderiv");
			glGetUniformLocation=(PFNGLGETUNIFORMLOCATIONPROC)getOpenGlExtension("glGetUniformLocation");
			glLinkProgram=(PFNGLLINKPROGRAMPROC)getOpenGlExtension("glLinkProgram");
			glMapBuffer=(PFNGLMAPBUFFERPROC)getOpenGlExtension("glMapBuffer");
			glShaderSource=(PFNGLSHADERSOURCEPROC)getOpenGlExtension("glShaderSource");
			glUniform1f=(PFNGLUNIFORM1FPROC)getOpenGlExtension("glUniform1f");
			glUniform2f=(PFNGLUNIFORM2FPROC)getOpenGlExtension("glUniform2f");
			glUniform1i=(PFNGLUNIFORM1IPROC)getOpenGlExtension("glUniform1i");
			glUnmapBuffer=(PFNGLUNMAPBUFFERPROC)getOpenGlExtension("glUnmapBuffer");
			glUseProgram=(PFNGLUSEPROGRAMPROC)getOpenGlExtension("glUseProgram");
			glVertexAttribPointer=(PFNGLVERTEXATTRIBPOINTERPROC)getOpenGlExtension("glVertexAttribPointer");
		}

		bool OpenGL3::ResizedContext( Gwen::WindowProvider* pWindow, int w, int h )
		{
#ifdef _WIN32
			RECT r;

			if ( GetClientRect( ( HWND ) pWindow->GetWindow(), &r ) )
			{
				glMatrixMode( GL_PROJECTION );
				glLoadIdentity();
				glOrtho( r.left, r.right, r.bottom, r.top, -1.0, 1.0 );
				glMatrixMode( GL_MODELVIEW );
				glViewport( 0, 0, r.right - r.left, r.bottom - r.top );
			}

			return true;
#endif
			return false;
		}

		bool OpenGL3::BeginContext( Gwen::WindowProvider* pWindow )
		{
			glClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			return true;
		}

		bool OpenGL3::EndContext( Gwen::WindowProvider* pWindow )
		{
			return true;
		}

	}
}