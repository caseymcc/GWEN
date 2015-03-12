/*
	GWEN
	Copyright (c) 2011 Facepunch Studios
	See license in Gwen.h
*/

#ifndef GWEN_RENDERERS_OPENGL_DEBUGFONT_H
#define GWEN_RENDERERS_OPENGL_DEBUGFONT_H

#include "Gwen/Gwen.h"
#include "Gwen/Renderers/OpenGL3.h"

namespace Gwen
{
	namespace Renderer
	{

		class OpenGL3_DebugFont : public Gwen::Renderer::OpenGL3
		{
			public:

				OpenGL3_DebugFont();
				~OpenGL3_DebugFont();

				void Init(int _windowWidth, int _windowHeight);

				void RenderText( Gwen::Font* pFont, Gwen::Point pos, const Gwen::UnicodeString & text );
				Gwen::Point MeasureText( Gwen::Font* pFont, const Gwen::UnicodeString & text );

			protected:

				void CreateDebugFont();
				void DestroyDebugFont();

				Gwen::Texture*	m_pFontTexture;
				float			m_fFontScale[2];
				float			m_fLetterSpacing;

		};

	}
}
#endif
