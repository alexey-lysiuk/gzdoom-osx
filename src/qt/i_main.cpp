/*
** i_main.cpp
** System-specific startup code. Eventually calls D_DoomMain.
**
**---------------------------------------------------------------------------
** Copyright 1998-2007 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <new>
#include <sys/param.h>
#include <locale.h>
#if defined(__MACH__) && !defined(NOASM)
#include <sys/types.h>
#include <sys/mman.h>
#endif

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtOpenGL/QGLWidget>

#include "doomerrors.h"
#include "m_argv.h"
#include "d_main.h"
#include "d_event.h"
#include "d_gui.h"
#include "i_system.h"
#include "i_video.h"
#include "c_console.h"
#include "errors.h"
#include "version.h"
#include "w_wad.h"
#include "g_level.h"
#include "r_state.h"
#include "cmdlib.h"
#include "r_main.h"
#include "doomstat.h"
#include "dikeys.h"

// MACROS ------------------------------------------------------------------

// The maximum number of functions that can be registered with atterm.
#define MAX_TERMS	64

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

extern "C" int cc_install_handlers(int, char**, int, int*, const char*, int(*)(char*, char*));

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

#ifndef NO_GTK
bool GtkAvailable;
#endif

// The command line arguments.
DArgs *Args;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static void (*TermFuncs[MAX_TERMS]) ();
static const char *TermNames[MAX_TERMS];
static int NumTerms;

// CODE --------------------------------------------------------------------

void addterm (void (*func) (), const char *name)
{
	// Make sure this function wasn't already registered.
	for (int i = 0; i < NumTerms; ++i)
	{
		if (TermFuncs[i] == func)
		{
			return;
		}
	}
    if (NumTerms == MAX_TERMS)
	{
		func ();
		I_FatalError (
			"Too many exit functions registered.\n"
			"Increase MAX_TERMS in i_main.cpp");
	}
	TermNames[NumTerms] = name;
    TermFuncs[NumTerms++] = func;
}

void popterm ()
{
	if (NumTerms)
		NumTerms--;
}

void STACK_ARGS call_terms ()
{
    while (NumTerms > 0)
	{
//		printf ("term %d - %s\n", NumTerms, TermNames[NumTerms-1]);
		TermFuncs[--NumTerms] ();
	}
}

//==========================================================================
//
// FinalGC
//
// Collect garbage one last time before exiting.
//
//==========================================================================

static void FinalGC()
{
	Args = NULL;
	GC::FullGC();
}

static void STACK_ARGS NewFailure ()
{
    I_FatalError ("Failed to allocate memory from system heap");
}

static int DoomSpecificInfo (char *buffer, char *end)
{
	const char *arg;
	int size = end-buffer-2;
	int i, p;

	p = 0;
	p += snprintf (buffer+p, size-p, GAMENAME" version " DOTVERSIONSTR " (" __DATE__ ")\n");
#ifdef __VERSION__
	p += snprintf (buffer+p, size-p, "Compiler version: %s\n", __VERSION__);
#endif
	p += snprintf (buffer+p, size-p, "\nCommand line:");
	for (i = 0; i < Args->NumArgs(); ++i)
	{
		p += snprintf (buffer+p, size-p, " %s", Args->GetArg(i));
	}
	p += snprintf (buffer+p, size-p, "\n");
	
	for (i = 0; (arg = Wads.GetWadName (i)) != NULL; ++i)
	{
		p += snprintf (buffer+p, size-p, "\nWad %d: %s", i, arg);
	}

	if (gamestate != GS_LEVEL && gamestate != GS_TITLELEVEL)
	{
		p += snprintf (buffer+p, size-p, "\n\nNot in a level.");
	}
	else
	{
		char name[9];

		strncpy (name, level.mapname, 8);
		name[8] = 0;
		p += snprintf (buffer+p, size-p, "\n\nCurrent map: %s", name);

		if (!viewactive)
		{
			p += snprintf (buffer+p, size-p, "\n\nView not active.");
		}
		else
		{
			p += snprintf (buffer+p, size-p, "\n\nviewx = %d", (int)viewx);
			p += snprintf (buffer+p, size-p, "\nviewy = %d", (int)viewy);
			p += snprintf (buffer+p, size-p, "\nviewz = %d", (int)viewz);
			p += snprintf (buffer+p, size-p, "\nviewangle = %x", (unsigned int)viewangle);
		}
	}
	buffer[p++] = '\n';
	buffer[p++] = '\0';

	return p;
}

#if defined(__MACH__) && !defined(NOASM)
// NASM won't let us create custom sections for Mach-O. Whether that's a limitation of NASM
// or of Mach-O, I don't know, but since we're using NASM for the assembly, it doesn't much
// matter.
extern "C"
{
	extern void *rtext_a_start, *rtext_a_end;
	extern void *rtext_tmap_start, *rtext_tmap_end;
	extern void *rtext_tmap2_start, *rtext_tmap2_end;
	extern void *rtext_tmap3_start, *rtext_tmap3_end;
};

static void unprotect_pages(long pagesize, void *start, void *end)
{
	char *page = (char *)((intptr_t)start & ~(pagesize - 1));
	size_t len = (char *)end - (char *)start;
	if (mprotect(page, len, PROT_READ|PROT_WRITE|PROT_EXEC) != 0)
	{
		fprintf(stderr, "mprotect failed\n");
		exit(1);
	}
}

static void unprotect_rtext()
{
	static void *const pages[] =
	{
		rtext_a_start, rtext_a_end,
		rtext_tmap_start, rtext_tmap_end,
		rtext_tmap2_start, rtext_tmap2_end,
		rtext_tmap3_start, rtext_tmap3_end
	};
	long pagesize = sysconf(_SC_PAGESIZE);
	for (void *const *p = pages; p < &pages[countof(pages)]; p += 2)
	{
		unprotect_pages(pagesize, p[0], p[1]);
	}
}
#endif

void I_StartupJoysticks();
void I_ShutdownJoysticks();


QGLWidget* g_renderWidget = NULL;


class RenderWidget : public QGLWidget
{
public:
	
	
protected:
	virtual void initializeGL();
	//virtual void resizeGL( int width, int height );
	
	virtual void resizeEvent( QResizeEvent* event );
	
	virtual void keyPressEvent( QKeyEvent* keyEvent );
    virtual void keyReleaseEvent( QKeyEvent* keyEvent );

private:
	void processKeyEvent( QKeyEvent* keyEvent );
	
};


void RenderWidget::initializeGL()
{
	glClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT );
}

//void RenderWidget::resizeGL( int width, int height )
//{
//	extern int NewWidth, NewHeight, NewBits, DisplayBits;
//	extern bool setmodeneeded;
//	
//	NewWidth    = width;
//	NewHeight   = height;
//	NewBits     = 32;
//	DisplayBits = 32;
//	
//	setmodeneeded = true;
//}

void RenderWidget::resizeEvent( QResizeEvent* event )
{
	extern int NewWidth, NewHeight, NewBits, DisplayBits;
	extern bool setmodeneeded;
	
	NewWidth    = event->size().width();
	NewHeight   = event->size().height();
	NewBits     = 32;
	DisplayBits = 32;
	
	setmodeneeded = true;
}


void RenderWidget::keyPressEvent( QKeyEvent* keyEvent )
{
	processKeyEvent( keyEvent );
}

void RenderWidget::keyReleaseEvent( QKeyEvent* keyEvent )
{
	processKeyEvent( keyEvent );
}

extern bool GUICapture;

void RenderWidget::processKeyEvent( QKeyEvent* keyEvent )
{
	event_t event;
	memset( &event, 0, sizeof( event ) );
	
	if ( GUICapture )
	{
		event.type = EV_GUI_Event;
		event.subtype = QEvent::KeyPress == keyEvent->type()
			? EV_GUI_KeyDown
			: EV_GUI_KeyUp;
		
		switch ( keyEvent->key() )
		{
			case Qt::Key_Up:
				event.data1 = GK_UP;
				break;
				
			case Qt::Key_Down:
				event.data1 = GK_DOWN;
				break;
				
			case Qt::Key_Escape:
				event.data1 = GK_ESCAPE;
				break;
				
			case Qt::Key_Return:
			case Qt::Key_Enter:
				event.data1 = GK_RETURN;
				break;
				
			default:
				break;
		}
		
	}
	else
	{
		event.type = QEvent::KeyPress == keyEvent->type()
			? EV_KeyDown
			: EV_KeyUp;
		
		switch ( keyEvent->key() )
		{
			case Qt::Key_Up:
				event.data1 = DIK_UP;
				break;
				
			case Qt::Key_Down:
				event.data1 = DIK_DOWN;
				break;
				
			case Qt::Key_Escape:
				event.data1 = DIK_ESCAPE;
				break;
				
			case Qt::Key_Return:
			case Qt::Key_Enter:
				event.data1 = DIK_RETURN;
				break;
				
			default:
				break;
		}
	}
	
	D_PostEvent( &event );
}


int main (int argc, char **argv)
{
#if !defined (__APPLE__)
	{
		int s[4] = { SIGSEGV, SIGILL, SIGFPE, SIGBUS };
		cc_install_handlers(argc, argv, 4, s, "zdoom-crash.log", DoomSpecificInfo);
	}
#endif // !__APPLE__

	printf(GAMENAME" v%s - SVN revision %s - Qt version\nCompiled on %s\n\n",
		DOTVERSIONSTR_NOREV,SVN_REVISION_STRING,__DATE__);

	seteuid (getuid ());
    std::set_new_handler (NewFailure);

#if defined(__MACH__) && !defined(NOASM)
	unprotect_rtext();
#endif
	
	QApplication application( argc, argv );
	
	g_renderWidget = new RenderWidget; //new QGLWidget;
//	g_renderWidget->setAttribute( Qt::WA_QuitOnClose, true );
	g_renderWidget->setWindowTitle( GAMESIG " " DOTVERSIONSTR " (" __DATE__ ")" );
	g_renderWidget->setMinimumSize( 640, 480 );
//	g_renderWidget->setGeometry( 100, 100, 800, 600 );
	g_renderWidget->makeCurrent();
	
    try
    {
		Args = new DArgs(argc, argv);
		atterm(FinalGC);

		/*
		  killough 1/98:

		  This fixes some problems with exit handling
		  during abnormal situations.

		  The old code called I_Quit() to end program,
		  while now I_Quit() is installed as an exit
		  handler and exit() is called to exit, either
		  normally or abnormally. Seg faults are caught
		  and the error handler is used, to prevent
		  being left in graphics mode or having very
		  loud SFX noise because the sound card is
		  left in an unstable state.
		*/

		atexit (call_terms);
		atterm (I_Quit);

		// Should we even be doing anything with progdir on Unix systems?
		char program[PATH_MAX];
		if (realpath (argv[0], program) == NULL)
			strcpy (program, argv[0]);
		char *slash = strrchr (program, '/');
		if (slash != NULL)
		{
			*(slash + 1) = '\0';
			progdir = program;
		}
		else
		{
			progdir = "./";
		}

		I_StartupJoysticks();
		C_InitConsole (80*8, 25*8, false);
		D_DoomMain ();
    }
    catch (class CDoomError &error)
    {
		I_ShutdownJoysticks();
		if (error.GetMessage ())
			fprintf (stderr, "%s\n", error.GetMessage ());
		exit (-1);
    }
    catch (...)
    {
		call_terms ();
		throw;
    }
    return 0;
}
