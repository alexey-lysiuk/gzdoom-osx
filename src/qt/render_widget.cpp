
#include "render_widget.h"

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>

#include "c_console.h"
#include "d_event.h"
#include "d_gui.h"
#include "dikeys.h"
#include "doomstat.h"
#include "version.h"


RenderWidget* g_renderWidget = NULL;


RenderWidget::RenderWidget()
{
	setWindowTitle( GAMESIG " " DOTVERSIONSTR " (" __DATE__ ")" );
//f	setWindowFlags( Qt::Window | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint );
//	setAttribute( Qt::WA_QuitOnClose, true );
	
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
//	setMinimumSize( 640, 480 );
	
	makeCurrent();
}

RenderWidget::~RenderWidget()
{
	
}


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

//void RenderWidget::resizeEvent( QResizeEvent* event )
//{
//	extern int NewWidth, NewHeight, NewBits, DisplayBits;
//	extern bool setmodeneeded;
//	
//	NewWidth    = event->size().width();
//	NewHeight   = event->size().height();
//	NewBits     = 32;
//	DisplayBits = 32;
//	
//	setmodeneeded = true;
//}


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


// ----------------------------------------------------------------------------


extern int WaitingForKey, chatmodeon;
extern constate_e ConsoleState;


static void I_CheckGUICapture()
{
	bool wantCapt;
//	bool repeat;
//	int oldrepeat, interval;
//
//	SDL_GetKeyRepeat(&oldrepeat, &interval);
	
	if (menuactive == MENU_Off)
	{
		wantCapt = ConsoleState == c_down || ConsoleState == c_falling || chatmodeon;
	}
	else
	{
		wantCapt = (menuactive == MENU_On || menuactive == MENU_OnNoPause);
	}
	
	if (wantCapt != GUICapture)
	{
		GUICapture = wantCapt;
//		if (wantCapt)
//		{
//			int x, y;
//			SDL_GetMouseState (&x, &y);
//			cursorBlit.x = x;
//			cursorBlit.y = y;
//
//			FlushDIKState ();
//			memset (DownState, 0, sizeof(DownState));
//			repeat = !sdl_nokeyrepeat;
//			SDL_EnableUNICODE (1);
//		}
//		else
//		{
//			repeat = false;
//			SDL_EnableUNICODE (0);
//		}
	}
//	if (wantCapt)
//	{
//		repeat = !sdl_nokeyrepeat;
//	}
//	else
//	{
//		repeat = false;
//	}
//	if (repeat != (oldrepeat != 0))
//	{
//		if (repeat)
//		{
//			SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
//		}
//		else
//		{
//			SDL_EnableKeyRepeat (0, 0);
//		}
//	}
}

static void I_CheckNativeMouse()
{
	
}


bool GUICapture;


void I_GetEvent ()
{
	if ( !g_renderWidget->isVisible() )
	{
		const QRect desktopGeometry = QApplication::desktop()->availableGeometry();
		
		QRect newGeometry = g_renderWidget->geometry();
		newGeometry.moveCenter( desktopGeometry.center() );
		
		g_renderWidget->move( newGeometry.topLeft() );
		g_renderWidget->show();
	}
	
	qApp->processEvents();
}

void I_StartTic ()
{
	I_CheckGUICapture ();
	I_CheckNativeMouse ();
	I_GetEvent ();
}

void I_StartFrame ()
{
//	if (KeySymToDIK[SDLK_BACKSPACE] == 0)
//	{
//		InitKeySymMap ();
//	}
}


void I_SetMouseCapture()
{
	
}

void I_ReleaseMouseCapture()
{
	
}
