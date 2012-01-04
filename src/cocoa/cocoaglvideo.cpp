
// HEADER FILES ------------------------------------------------------------

#include "cocoaglvideo.h"

#include "doomtype.h"
#include "templates.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "v_pfx.h"
#include "stats.h"
#include "version.h"
#include "c_console.h"
#include "r_defs.h"

#include "gl/system/gl_system.h"
#include "gl/gl_functions.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/system/gl_framebuffer.h"
#include "gl/shaders/gl_shader.h"
#include "gl/utility/gl_templates.h"
#include "gl/textures/gl_material.h"
#include "gl/system/gl_cvars.h"

#include "cocoa_application.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(CocoaGLFB)

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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// Dummy screen sizes to pass when windowed
static const MiniModeInfo WinModes[] =
{
	{ 320, 200 },
	{ 320, 240 },
	{ 400, 225 },	// 16:9
	{ 400, 300 },
	{ 480, 270 },	// 16:9
	{ 480, 360 },
	{ 512, 288 },	// 16:9
	{ 512, 384 },
	{ 640, 360 },	// 16:9
	{ 640, 400 },
	{ 640, 480 },
	{ 720, 480 },	// 16:10
	{ 720, 540 },
	{ 800, 450 },	// 16:9
	{ 800, 500 },	// 16:10
	{ 800, 600 },
	{ 848, 480 },	// 16:9
	{ 960, 600 },	// 16:10
	{ 960, 720 },
	{ 1024, 576 },	// 16:9
	{ 1024, 640 },	// 16:10
	{ 1024, 768 },
	{ 1088, 612 },	// 16:9
	{ 1152, 648 },	// 16:9
	{ 1152, 720 },	// 16:10
	{ 1152, 864 },
	{ 1280, 720 },	// 16:9
	{ 1280, 800 },	// 16:10
	{ 1280, 960 },
	{ 1344, 756 },  // 16:9
	{ 1360, 768 },	// 16:9
	{ 1400, 787 },	// 16:9
	{ 1400, 875 },	// 16:10
	{ 1440, 900 },
	{ 1400, 1050 },
	{ 1600, 900 },	// 16:9
	{ 1600, 1000 },	// 16:10
	{ 1600, 1200 },
	{ 1680, 1050 }, // 16:10
	{ 1920, 1080 }, // 16:9
	{ 1920, 1200 }, // 16:10
	{ 2054, 1536 },
	{ 2560, 1440 }  // 16:9
};

static const size_t MODES_TOTAL = sizeof( WinModes ) / sizeof( WinModes[0] );

// CODE --------------------------------------------------------------------

static size_t GetHighestFullscreenMode()
{
	const CocoaApplication::ScreenSize screenSize = CocoaApplication::GetScreenSize();
	
	size_t widthIndex = 0;
	
	while ( widthIndex < MODES_TOTAL 
		   && WinModes[ widthIndex ].Width <= screenSize.X )
	{
		++widthIndex;
	}
	
	size_t heightIndex = 0;
	
	while ( heightIndex < MODES_TOTAL 
		   && WinModes[ widthIndex ].Height <= screenSize.Y )
	{
		++heightIndex;
	}
	
	return std::min( widthIndex, heightIndex );
}

CocoaGLVideo::CocoaGLVideo( int parm )
: m_highestFullscreenMode( GetHighestFullscreenMode() )
{
	IteratorBits = 0;
	IteratorFS = false;
    
	GetContext(gl);

	CocoaApplication::SetCursorVisible( false );
}

CocoaGLVideo::~CocoaGLVideo()
{
	if ( NULL != GLRenderer )
	{
		GLRenderer->FlushTextures();
	}
}

void CocoaGLVideo::StartModeIterator( int bits, bool fs )
{
	IteratorMode = 0;
	IteratorBits = bits;
	IteratorFS = fs;
}

bool CocoaGLVideo::NextMode( int *width, int *height, bool *letterbox )
{
	if ( 8 != IteratorBits )
	{
		return false;
	}
	
	const size_t highestMode = IteratorFS 
		? m_highestFullscreenMode 
		: MODES_TOTAL;

	if ( IteratorMode < highestMode )
	{
		*width  = WinModes[IteratorMode].Width;
		*height = WinModes[IteratorMode].Height;
		++IteratorMode;
		return true;			
	}
	
	return false;
}

DFrameBuffer *CocoaGLVideo::CreateFrameBuffer( int width, int height, bool fullscreen, DFrameBuffer* old )
{
	static int retry = 0;
	static int owidth, oheight;
	
	if ( NULL != old )
	{
		// Reuse the old framebuffer
		return old;
	}
	
	CocoaGLFB *fb = new OpenGLFrameBuffer( 0, width, height, 32, 60, fullscreen );
	retry = 0;
	
	// If we could not create the framebuffer, try again with slightly
	// different parameters in this order:
	// 1. Try with the closest size
	// 2. Try in the opposite screen mode with the original size
	// 3. Try in the opposite screen mode with the closest size
	// This is a somewhat confusing mass of recursion here.

	while (fb == NULL || !fb->IsValid ())
	{
		if (fb != NULL)
		{
			delete fb;
		}

		switch (retry)
		{
		case 0:
			owidth = width;
			oheight = height;
		case 2:
			// Try a different resolution. Hopefully that will work.
			I_ClosestResolution (&width, &height, 8);
			break;

		case 1:
			// Try changing fullscreen mode. Maybe that will work.
			width = owidth;
			height = oheight;
			fullscreen = !fullscreen;
			break;

		default:
			// I give up!
			I_FatalError ("Could not create new screen (%d x %d)", owidth, oheight);

			fprintf( stderr, "!!! [CocoaGLVideo::CreateFrameBuffer] Got beyond I_FatalError !!!" );
			return NULL;	//[C] actually this shouldn't be reached; probably should be replaced with an ASSERT
		}

		++retry;
		fb = static_cast<CocoaGLFB *>(CreateFrameBuffer (width, height, fullscreen, NULL));
	}

	return fb;
}

void CocoaGLVideo::SetWindowedScale( float )
{
	
}

bool CocoaGLVideo::SetResolution( int width, int height, int bits )
{
	// FIXME: Is it possible to do this without completely destroying the old
	// interface?
#ifndef NO_GL

	if (GLRenderer != NULL) GLRenderer->FlushTextures();
	I_ShutdownGraphics();

	Video = new CocoaGLVideo(0);
	if (Video == NULL) I_FatalError ("Failed to initialize display");

#if (defined(WINDOWS)) || defined(WIN32)
	bits=32;
#else
	bits=24;
#endif
	
	V_DoModeSetup(width, height, bits);
#endif
	
	return true;	// We must return true because the old video context no longer exists.
}

// FrameBuffer implementation -----------------------------------------------

CocoaGLFB::CocoaGLFB( void *, int width, int height, int, int, bool fullscreen )
: DFrameBuffer( width, height )
{
	static int localmultisample=-1;

	if (localmultisample<0) localmultisample=gl_vid_multisample;

	int i;
	
	m_lock=0;

	m_updatePending = false;
	
	if (!gl.InitHardware(false, gl_vid_compatibility, localmultisample))
	{
		vid_renderer = 0;
		return;
	}

	CocoaApplication::SetVideoResolution( width, height, fullscreen );
	
#ifdef _DEBUG
	m_supportsGamma = false;
#else // !_DEBUG
	m_supportsGamma = CocoaApplication::GetGammaRamp( 
		m_originalGamma[0], m_originalGamma[1], m_originalGamma[2] );
#endif // _DEBUG
}

CocoaGLFB::~CocoaGLFB()
{
	if ( m_supportsGamma )
	{
		CocoaApplication::SetGammaRamp( m_originalGamma[0], m_originalGamma[1], m_originalGamma[2] );
	}
}

void CocoaGLFB::InitializeState() 
{
//	int value = 0;
//	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &value );
//	if (!value) 
//	{
//		Printf("Failed to use stencil buffer!\n");	//[C] is it needed to recreate buffer in "cheapest mode"?
//		gl.flags|=RFL_NOSTENCIL;
//	}
}

bool CocoaGLFB::CanUpdate()
{
	if ( 1 != m_lock )
	{
		if ( m_lock > 0 )
		{
			m_updatePending = true;
			--m_lock;
		}
		
		return false;
	}
	
	return true;
}

void CocoaGLFB::SetGammaTable( WORD* tbl )
{
	if ( m_supportsGamma )
	{
		CocoaApplication::SetGammaRamp( &tbl[0], &tbl[256], &tbl[512] );
	}
}

bool CocoaGLFB::Lock( bool buffered )
{
	m_lock++;
	
	Buffer = MemBuffer;
	
	return true;
}

bool CocoaGLFB::Lock() 
{ 	
	return Lock( false ); 
}

void CocoaGLFB::Unlock() 	
{ 
	if ( m_updatePending && 1 == m_lock )
	{
		Update();
	}
	else if ( --m_lock <= 0 )
	{
		m_lock = 0;
	}
}

bool CocoaGLFB::IsLocked() 
{ 
	return m_lock > 0;
}

bool CocoaGLFB::IsFullscreen()
{
	return CocoaApplication::IsFullscreen();
}


bool CocoaGLFB::IsValid()
{
	return DFrameBuffer::IsValid();
}

void CocoaGLFB::SetVSync( bool vsync )
{
//#if defined (__APPLE__)
//	const GLint value = vsync ? 1 : 0;
//	CGLSetParameter( CGLGetCurrentContext(), kCGLCPSwapInterval, &value );
//#endif
}

void CocoaGLFB::NewRefreshRate ()
{
}

