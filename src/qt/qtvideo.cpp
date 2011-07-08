
// HEADER FILES ------------------------------------------------------------

#include <iostream>

#include "doomtype.h"

#include "templates.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "v_pfx.h"
#include "stats.h"
#include "version.h"
#include "c_console.h"

#include "qtvideo.h"
#include "gl/system/gl_system.h"
#include "r_defs.h"
#include "gl/gl_functions.h"
//#include "gl/gl_intern.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/system/gl_framebuffer.h"
#include "gl/shaders/gl_shader.h"
#include "gl/utility/gl_templates.h"
#include "gl/textures/gl_material.h"
#include "gl/system/gl_cvars.h"

#include "render_widget.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS( QtGLFB )

struct MiniModeInfo
{
	WORD Width, Height;
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern IVideo *Video;
// extern int vid_renderer;

EXTERN_CVAR (Float, Gamma)
EXTERN_CVAR (Int, vid_displaybits)
EXTERN_CVAR (Int, vid_renderer)


// PUBLIC DATA DEFINITIONS -------------------------------------------------

CUSTOM_CVAR(Int, gl_vid_multisample, 0, CVAR_ARCHIVE | CVAR_GLOBALCONFIG | CVAR_NOINITCALL )
{
	Printf("This won't take effect until "GAMENAME" is restarted.\n");
}

RenderContext gl;

// CODE --------------------------------------------------------------------

QtGLVideo::QtGLVideo()
{
//	IteratorBits = 0;
//	IteratorFS = false;
//    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
//        fprintf( stderr, "Video initialization failed: %s\n",
//             SDL_GetError( ) );
//    }
	GetContext(gl);
//#ifndef	_WIN32
//	// mouse cursor is visible by default on linux systems, we disable it by default
//	SDL_ShowCursor (0);
//#endif
}

QtGLVideo::~QtGLVideo()
{
//	if (GLRenderer != NULL) GLRenderer->FlushTextures();
//	SDL_Quit( );
}

void QtGLVideo::StartModeIterator (int bits, bool fs)
{
//	IteratorMode = 0;
//	IteratorBits = bits;
//	IteratorFS = fs;
}

bool QtGLVideo::NextMode (int *width, int *height, bool *letterbox)
{
//	if (IteratorBits != 8)
//		return false;
//	
//	if (!IteratorFS)
//	{
//		if ((unsigned)IteratorMode < sizeof(WinModes)/sizeof(WinModes[0]))
//		{
//			*width = WinModes[IteratorMode].Width;
//			*height = WinModes[IteratorMode].Height;
//			++IteratorMode;
//			return true;
//		}
//	}
//	else
//	{
//		SDL_Rect **modes = SDL_ListModes (NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
//		if (modes != NULL && modes[IteratorMode] != NULL)
//		{
//			*width = modes[IteratorMode]->w;
//			*height = modes[IteratorMode]->h;
//			++IteratorMode;
//			return true;
//		}
//	}
	return false;
}

DFrameBuffer *QtGLVideo::CreateFrameBuffer (int width, int height, bool fullscreen, DFrameBuffer *old)
{
//	static int retry = 0;
//	static int owidth, oheight;
//	
//	PalEntry flashColor;
////	int flashAmount;
//
//	if (old != NULL)
//	{ // Reuse the old framebuffer if its attributes are the same
//		SDLGLFB *fb = static_cast<SDLGLFB *> (old);
//		if (fb->Width == width &&
//			fb->Height == height)
//		{
//			bool fsnow = (fb->Screen->flags & SDL_FULLSCREEN) != 0;
//	
//			if (fsnow != fullscreen)
//			{
//				SDL_WM_ToggleFullScreen (fb->Screen);
//			}
//			return old;
//		}
////		old->GetFlash (flashColor, flashAmount);
//		delete old;
//	}
//	else
//	{
//		flashColor = 0;
////		flashAmount = 0;
//	}
//	
//	SDLGLFB *fb = new OpenGLFrameBuffer (0, width, height, 32, 60, fullscreen);
//	retry = 0;
//	
//	// If we could not create the framebuffer, try again with slightly
//	// different parameters in this order:
//	// 1. Try with the closest size
//	// 2. Try in the opposite screen mode with the original size
//	// 3. Try in the opposite screen mode with the closest size
//	// This is a somewhat confusing mass of recursion here.
//
//	while (fb == NULL || !fb->IsValid ())
//	{
//		if (fb != NULL)
//		{
//			delete fb;
//		}
//
//		switch (retry)
//		{
//		case 0:
//			owidth = width;
//			oheight = height;
//		case 2:
//			// Try a different resolution. Hopefully that will work.
//			I_ClosestResolution (&width, &height, 8);
//			break;
//
//		case 1:
//			// Try changing fullscreen mode. Maybe that will work.
//			width = owidth;
//			height = oheight;
//			fullscreen = !fullscreen;
//			break;
//
//		default:
//			// I give up!
//			I_FatalError ("Could not create new screen (%d x %d)", owidth, oheight);
//
//			fprintf( stderr, "!!! [SDLGLVideo::CreateFrameBuffer] Got beyond I_FatalError !!!" );
//			return NULL;	//[C] actually this shouldn't be reached; probably should be replaced with an ASSERT
//		}
//
//		++retry;
//		fb = static_cast<SDLGLFB *>(CreateFrameBuffer (width, height, fullscreen, NULL));
//	}
//
////	fb->SetFlash (flashColor, flashAmount);
//	return fb;

	if ( NULL == g_renderWidget )
	{
		g_renderWidget = new RenderWidget;
	}

	QRect newGeometry = g_renderWidget->geometry();
	newGeometry.setWidth ( width  );
	newGeometry.setHeight( height );
	
	g_renderWidget->setGeometry( newGeometry );
	
	if ( NULL != old )
	{
		return old;
	}
	
	QtGLFB* result = new OpenGLFrameBuffer( 0, width, height, 32, 60, fullscreen );
	if ( NULL == result )
	{
		I_FatalError( "Could not create new screen (%d x %d)", width, height );
	}
	
//	g_renderWidget->setGeometry( 100, 100, width, height );
//	g_renderWidget->show();
	
	return result;
}

//void SDLGLVideo::SetWindowedScale (float scale)
//{
//}
//
//bool QtGLVideo::SetResolution (int width, int height, int bits)
//{
//	// FIXME: Is it possible to do this without completely destroying the old
//	// interface?
//#ifndef NO_GL
//
//	if (GLRenderer != NULL) GLRenderer->FlushTextures();
//	I_ShutdownGraphics();
//
//	Video = new QtGLVideo;
//	if (Video == NULL) I_FatalError ("Failed to initialize display");
//
//#if (defined(WINDOWS)) || defined(WIN32)
//	bits=32;
//#else
//	bits=24;
//#endif
//	
//	V_DoModeSetup(width, height, bits);
//#endif
//	return true;	// We must return true because the old video context no longer exists.
//}

// FrameBuffer implementation -----------------------------------------------

QtGLFB::QtGLFB (void *, int width, int height, int, int, bool fullscreen)
	: DFrameBuffer (width, height)
{
//	static int localmultisample=-1;
//
//	if (localmultisample<0) localmultisample=gl_vid_multisample;
//
//	int i;
	
	m_Lock=0;

	UpdatePending = false;
	
//	if (!gl.InitHardware(false, gl_vid_compatibility, localmultisample))
//	{
//		vid_renderer = 0;
//		return;
//	}
//
//#if defined(__APPLE__)
//
//	// Mac OS X version will crash when entering fullscreen mode with BPP <= 8
//	// Also it may crash with BPP == 16 on some configurations
//	// It seems 24 and 32 bits are safe values
//	// So value of vid_displaybits is ignored and hardcoded constant is used instead
//	
//	const int bpp = 32;
//
//#else // ! __APPLE__
//	const int bpp = vid_displaybits;
//#endif // __APPLE__
//	
//	Screen = SDL_SetVideoMode (width, height, bpp,
//		SDL_HWSURFACE|SDL_HWPALETTE|SDL_OPENGL | SDL_GL_DOUBLEBUFFER|SDL_ANYFORMAT|
//		(fullscreen ? SDL_FULLSCREEN : 0));
//
//	if (Screen == NULL)
//		return;
//
//	m_supportsGamma = -1 != SDL_GetGammaRamp(m_origGamma[0], m_origGamma[1], m_origGamma[2]);
	
	m_supportsGamma = false;
	
//#if defined(__APPLE__)
//	// Need to set title here because a window is not created yet when calling the same function from main()
//	SDL_WM_SetCaption( GAMESIG " " DOTVERSIONSTR " (" __DATE__ ")", NULL );
//#endif // __APPLE__
}

QtGLFB::~QtGLFB ()
{
//	if (m_supportsGamma) 
//	{
//		SDL_SetGammaRamp(m_origGamma[0], m_origGamma[1], m_origGamma[2]);
//	}
}

void QtGLFB::InitializeState() 
{
//	int value = 0;
//	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &value );
//	if (!value) 
//	{
//		Printf("Failed to use stencil buffer!\n");	//[C] is it needed to recreate buffer in "cheapest mode"?
//		gl.flags|=RFL_NOSTENCIL;
//	}
	
//	extern QGLWidget* g_renderWidget;
//	
//	if ( NULL != g_renderWidget )
//	{
//		g_renderWidget->show();
//	}
}

bool QtGLFB::CanUpdate ()
{
	if (m_Lock != 1)
	{
		if (m_Lock > 0)
		{
			UpdatePending = true;
			--m_Lock;
		}
		return false;
	}
	return true;
}

void QtGLFB::SetGammaTable(WORD *tbl)
{
//	SDL_SetGammaRamp(&tbl[0], &tbl[256], &tbl[512]);
}

bool QtGLFB::Lock(bool buffered)
{
	m_Lock++;
//	Buffer = MemBuffer;
	return true;
}

bool QtGLFB::Lock () 
{ 	
	return Lock(false); 
}

void QtGLFB::Unlock () 	
{ 
	if (UpdatePending && m_Lock == 1)
	{
		Update ();
	}
	else if (--m_Lock <= 0)
	{
		m_Lock = 0;
	}
}

bool QtGLFB::IsLocked () 
{ 
	return m_Lock>0;// true;
}

bool QtGLFB::IsFullscreen ()
{
	//return (Screen->flags & SDL_FULLSCREEN) != 0;
	return false;
}


bool QtGLFB::IsValid ()
{
	//return DFrameBuffer::IsValid() && Screen != NULL;
	return true;
}

void QtGLFB::SetVSync( bool vsync )
{
#if defined (__APPLE__)
	const GLint value = vsync ? 1 : 0;
	CGLSetParameter( CGLGetCurrentContext(), kCGLCPSwapInterval, &value );
#endif
}

void QtGLFB::NewRefreshRate ()
{
}

