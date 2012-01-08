/*
 ** cocoa_application.mm
 **
 **---------------------------------------------------------------------------
 ** Copyright 2012 Alexey Lysiuk
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

#include "cocoa_application.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include "c_console.h"
#include "c_cvars.h"
#include "d_event.h"
#include "d_gui.h"
#include "dikeys.h"
#include "doomdef.h"
#include "doomstat.h"
#include "m_joy.h"
#include "s_sound.h"
#include "version.h"


void I_StartupJoysticks()
{
	
}

void I_ShutdownJoysticks()
{
	
}

void I_GetJoysticks( TArray< IJoystickConfig* >& sticks )
{
	sticks.Clear();
}

void I_GetAxes( float axes[ NUM_JOYAXIS ] )
{
	for ( size_t i = 0; i < NUM_JOYAXIS; ++i )
	{
		axes[i] = 0.0f;
	}
}

IJoystickConfig *I_UpdateDeviceList()
{
	return NULL;
}


// ---------------------------------------------------------------------------


@interface FullscreenWindow : NSWindow
{
@private
	bool m_fullscreen;
}

- (bool)canBecomeKeyWindow;

- (void)close;

- (bool)fullscreen;
- (void)setFullscreen:(bool)on;

@end


@implementation FullscreenWindow

- (bool)canBecomeKeyWindow
{
	return true;
}


- (void)close
{
	[super close];
	
	exit(0);
}


- (bool)fullscreen
{
	return m_fullscreen;
}

- (void)setFullscreen:(bool)on
{
	m_fullscreen = on;
}

@end


// ---------------------------------------------------------------------------


CVAR( Bool, use_mouse,    true,  CVAR_ARCHIVE | CVAR_GLOBALCONFIG )
CVAR( Bool, m_noprescale, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG )
CVAR( Bool, m_filter,     false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG )

CUSTOM_CVAR( Int, mouse_capturemode, 1, CVAR_GLOBALCONFIG | CVAR_ARCHIVE )
{
	if ( self < 0 )
	{
		self = 0;
	}
	else if ( self > 2 )
	{
		self = 2;
	}
}

bool GUICapture;

static bool  s_nativeMouse      = true;
static float s_pixelScaleFactor = 1.0f;

// TODO: remove this magic!
static size_t s_skipMouseMoves;


EXTERN_CVAR( Bool, fullscreen )

extern int paused, chatmodeon;
extern constate_e ConsoleState;


static void CheckGUICapture()
{
	const bool wantCapture = ( MENU_Off == menuactive )
		? ( c_down == ConsoleState || c_falling == ConsoleState || chatmodeon )
		: ( menuactive == MENU_On || menuactive == MENU_OnNoPause );
	
	if ( wantCapture != GUICapture )
	{
		GUICapture = wantCapture;
	}
}

static void CenterCursor()
{
	NSWindow* window = [NSApp keyWindow];
	if ( nil == window )
	{
		return;
	}
	
	const NSRect   windowRect = [window frame];
	const CGPoint centerPoint = CGPointMake( NSMidX( windowRect ), NSMidY( windowRect ) );
	
	CGEventSourceRef eventSource = CGEventSourceCreate( kCGEventSourceStateCombinedSessionState );
	
	if ( NULL != eventSource )
	{
		CGEventRef mouseMoveEvent = CGEventCreateMouseEvent( eventSource, 
			kCGEventMouseMoved, centerPoint, kCGMouseButtonLeft );
		
		if ( NULL != mouseMoveEvent )
		{
			CGEventPost( kCGHIDEventTap, mouseMoveEvent );
			CFRelease( mouseMoveEvent );
		}
		
		CFRelease( eventSource );
	}
	
	// TODO: remove this magic!
	s_skipMouseMoves = 2;
}


static bool IsInGame()
{
	switch ( mouse_capturemode )
	{
		default:
		case 0:
			return gamestate == GS_LEVEL;
			
		case 1:
			return gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_FINALE;
			
		case 2:
			return true;
	}
}

static void CheckNativeMouse()
{
	const bool focus      = CocoaApplication::IsActive();
	const bool fullscreen = CocoaApplication::IsFullscreen();
	const bool wantNative = !focus || !use_mouse || GUICapture || paused || demoplayback || !IsInGame();
	
	if ( wantNative != s_nativeMouse )
	{
		s_nativeMouse = wantNative;
		
		if ( !wantNative )
		{
			CenterCursor();
		}
		
		CGAssociateMouseAndMouseCursorPosition( wantNative );
		
		CocoaApplication::SetCursorVisible( wantNative );
	}
}

void I_GetEvent()
{
	CocoaApplication::ProcessEvents();
}

void I_StartTic()
{
	CheckGUICapture();
	CheckNativeMouse();
	
	I_GetEvent();
}

void I_StartFrame()
{

}


void I_SetMouseCapture()
{
	
}

void I_ReleaseMouseCapture()
{
	
}


// ---------------------------------------------------------------------------


static const size_t TABLE_SIZE = 128;

// See Carbon -> HIToolbox -> Events.h for kVK_ constants

static const uint8_t KEYCODE_TO_DIK[ TABLE_SIZE ] =
{
	DIK_A,        DIK_S,             DIK_D,         DIK_F,        DIK_H,           DIK_G,       DIK_Z,        DIK_X,          // 0x00 - 0x07
	DIK_C,        DIK_V,             0,             DIK_B,        DIK_Q,           DIK_W,       DIK_E,        DIK_R,          // 0x08 - 0x0F
	DIK_Y,        DIK_T,             DIK_1,         DIK_2,        DIK_3,           DIK_4,       DIK_6,        DIK_5,          // 0x10 - 0x17
	DIK_EQUALS,   DIK_9,             DIK_7,         DIK_MINUS,    DIK_8,           DIK_0,       DIK_RBRACKET, DIK_O,          // 0x18 - 0x1F
	DIK_U,        DIK_LBRACKET,      DIK_I,         DIK_P,        DIK_RETURN,      DIK_L,       DIK_J,        DIK_APOSTROPHE, // 0x20 - 0x27
	DIK_K,        DIK_SEMICOLON,     DIK_BACKSLASH, DIK_COMMA,    DIK_SLASH,       DIK_N,       DIK_M,        DIK_PERIOD,     // 0x28 - 0x2F
	DIK_TAB,      DIK_SPACE,         DIK_GRAVE,     DIK_BACK,     0,               DIK_ESCAPE,  0,            DIK_LWIN,       // 0x30 - 0x37
	DIK_LSHIFT,   DIK_CAPITAL,       DIK_LMENU,     DIK_LCONTROL, DIK_RSHIFT,      DIK_RMENU,   DIK_RCONTROL, 0,              // 0x38 - 0x3F
	0,            DIK_DECIMAL,       0,             DIK_MULTIPLY, 0,               DIK_ADD,     0,            0,              // 0x40 - 0x47
	DIK_VOLUMEUP, DIK_VOLUMEDOWN,    DIK_MUTE,      DIK_SLASH,    DIK_NUMPADENTER, 0,           DIK_SUBTRACT, 0,              // 0x48 - 0x4F
	0,            DIK_NUMPAD_EQUALS, DIK_NUMPAD0,   DIK_NUMPAD1,  DIK_NUMPAD2,     DIK_NUMPAD3, DIK_NUMPAD4,  DIK_NUMPAD5,    // 0x50 - 0x57
	DIK_NUMPAD6,  DIK_NUMPAD7,       0,             DIK_NUMPAD8,  DIK_NUMPAD9,     0,           0,            0,              // 0x58 - 0x5F
	DIK_F5,       DIK_F6,            DIK_F7,        DIK_F3,       DIK_F8,          DIK_F9,      0,            DIK_F11,        // 0x60 - 0x67
	0,            DIK_F13,           0,             DIK_F14,      0,               DIK_F10,     0,            DIK_F12,        // 0x68 - 0x6F
	0,            DIK_F15,           0,             DIK_HOME,     0,               DIK_DELETE,  DIK_F4,       DIK_END,        // 0x70 - 0x77
	DIK_F2,       0,                 DIK_F1,        DIK_LEFT,     DIK_RIGHT,       DIK_DOWN,    DIK_UP,       0,              // 0x78 - 0x7F	
};

static const uint8_t KEYCODE_TO_ASCII[ TABLE_SIZE ] =
{
	'a', 's',  'd', 'f', 'h', 'g', 'z',  'x', // 0x00 - 0x07
	'c', 'v',    0, 'b', 'q', 'w', 'e',  'r', // 0x08 - 0x0F
	'y', 't',  '1', '2', '3', '4', '6',  '5', // 0x10 - 0x17
	'=', '9',  '7', '-', '8', '0', ']',  'o', // 0x18 - 0x1F
	'u', '[',  'i', 'p',  13, 'l', 'j', '\'', // 0x20 - 0x27
	'k', ';', '\\', ',', '/', 'n', 'm',  '.', // 0x28 - 0x2F
	  9, ' ',  '`',  12,   0,  27,   0,    0, // 0x30 - 0x37
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x38 - 0x3F
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x40 - 0x47
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x48 - 0x4F
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x50 - 0x57
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x58 - 0x5F
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x60 - 0x67
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x68 - 0x6F
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x70 - 0x77
	  0,   0,    0,   0,   0,   0,   0,    0, // 0x78 - 0x7F
};


static uint8_t ModifierToDIK( const uint32_t modifier )
{
	switch ( modifier )
	{
		case NSAlphaShiftKeyMask: return DIK_CAPITAL;
		case NSShiftKeyMask:      return DIK_LSHIFT;
		case NSControlKeyMask:    return DIK_LCONTROL;
		case NSAlternateKeyMask:  return DIK_LMENU;
		case NSCommandKeyMask:    return DIK_LWIN;
	}
	
	return 0;
}

static SWORD ModifierFlagsToGUIKeyModifiers( NSEvent* theEvent )
{
	const SWORD modifiers( [theEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask );
	return ( ( modifiers & NSShiftKeyMask )     ? GKM_SHIFT : 0 )
		 | ( ( modifiers & NSControlKeyMask )   ? GKM_CTRL  : 0 )
		 | ( ( modifiers & NSAlternateKeyMask ) ? GKM_ALT   : 0 );
}

static void ProcessKeyboardEvent( NSEvent* theEvent )
{
	event_t event;
	memset( &event, 0, sizeof( event ) );
	
	const NSEventType cocoaEventType = [theEvent type];
	const uint32_t    modifiers      = [theEvent modifierFlags] & NSDeviceIndependentModifierFlagsMask;
	
	if ( NSFlagsChanged == cocoaEventType )
	{
		// TODO: add support simultaneous changes of several modifiers
		
		static uint32_t   oldModifiers = 0;
		const  uint32_t deltaModifiers = modifiers ^ oldModifiers;
		
		if ( 0 != deltaModifiers )
		{
			event.type  = modifiers > oldModifiers ? EV_KeyDown : EV_KeyUp;
			event.data1 = ModifierToDIK( deltaModifiers );
			
			D_PostEvent( &event );
			
			oldModifiers = modifiers;
		}
		
		return;
	}
	
	const unsigned short keyCode = [theEvent keyCode];
	if ( keyCode >= TABLE_SIZE )
	{
		assert( !"Unknown keycode" );
		return;
	}
		
	if ( GUICapture )
	{
		NSString* characters = [theEvent charactersIgnoringModifiers];
		const unichar character = [characters length] > 0
			? [characters characterAtIndex:0]
			: '\0';
		
		event.type = EV_GUI_Event;
		event.subtype = NSKeyDown == cocoaEventType ? EV_GUI_KeyDown : EV_GUI_KeyUp;
		
		event.data2 = character & 0xFF;
		event.data3 = ModifierFlagsToGUIKeyModifiers( theEvent );
		
		if ( EV_GUI_KeyDown == event.subtype && [theEvent isARepeat] )
		{
			event.subtype = EV_GUI_KeyRepeat;
		}

		switch ( keyCode )
		{
			case kVK_Return:        event.data1 = GK_RETURN;    break;
			case kVK_PageUp:        event.data1 = GK_PGUP;      break;
			case kVK_PageDown:      event.data1 = GK_PGDN;      break;
			case kVK_End:           event.data1 = GK_END;       break;
			case kVK_Home:          event.data1 = GK_HOME;      break;
			case kVK_LeftArrow:     event.data1 = GK_LEFT;      break;
			case kVK_RightArrow:    event.data1 = GK_RIGHT;     break;
			case kVK_UpArrow:       event.data1 = GK_UP;        break;
			case kVK_DownArrow:     event.data1 = GK_DOWN;      break;
			case kVK_Delete:        event.data1 = GK_BACKSPACE; break;
			case kVK_ForwardDelete: event.data1 = GK_DEL;       break;
			case kVK_Escape:        event.data1 = GK_ESCAPE;    break;
			case kVK_F1:            event.data1 = GK_F1;        break;
			case kVK_F2:            event.data1 = GK_F2;        break;
			case kVK_F3:            event.data1 = GK_F3;        break;
			case kVK_F4:            event.data1 = GK_F4;        break;
			case kVK_F5:            event.data1 = GK_F5;        break;
			case kVK_F6:            event.data1 = GK_F6;        break;
			case kVK_F7:            event.data1 = GK_F7;        break;
			case kVK_F8:            event.data1 = GK_F8;        break;
			case kVK_F9:            event.data1 = GK_F9;        break;
			case kVK_F10:           event.data1 = GK_F10;       break;
			case kVK_F11:           event.data1 = GK_F11;       break;
			case kVK_F12:           event.data1 = GK_F12;       break;
			default:
				event.data1 = KEYCODE_TO_ASCII[ keyCode ];
				break;
		}
		
		if ( event.data1 < 128 )
		{
			event.data1 = toupper( event.data1 );
			
			D_PostEvent( &event );
		}
		
		if ( !iscntrl( event.data2 ) && EV_GUI_KeyUp != event.subtype )
		{
			event.subtype = EV_GUI_Char;
			event.data1 = event.data2;
			event.data2 = modifiers & NSAlternateKeyMask;
			
			D_PostEvent( &event );
		}
	}
	else
	{
		event.type  = NSKeyDown == cocoaEventType ? EV_KeyDown : EV_KeyUp;
		event.data1 = KEYCODE_TO_DIK[ keyCode ];
		
		if ( 0 != event.data1 )
		{
			event.data2 = KEYCODE_TO_ASCII[ keyCode ];
			
			D_PostEvent( &event );
		}
	}
}


static void NSEventToGameMousePosition( NSEvent* inEvent, event_t* outEvent )
{
	NSWindow* window = [inEvent window];
	NSView*     view = [window contentView];
	
	const NSPoint screenPos = [NSEvent mouseLocation];
	const NSPoint windowPos = [window convertScreenToBase:screenPos];
	const NSPoint   viewPos = [view convertPointFromBase:windowPos];
	
	outEvent->data1 = static_cast< int >( (                            viewPos.x ) / s_pixelScaleFactor );
	outEvent->data2 = static_cast< int >( ( [view frame].size.height - viewPos.y ) / s_pixelScaleFactor );
}

static void ProcessMouseButtonEvent( NSEvent* theEvent )
{
	event_t event;
	memset( &event, 0, sizeof( event ) );
	
	const NSEventType cocoaEventType = [theEvent type];
	const NSInteger cocoaMouseButton = [theEvent buttonNumber];
	
	if ( GUICapture )
	{
		event.type = EV_GUI_Event;
		
		switch ( cocoaEventType )
		{
			case NSLeftMouseDown:  event.subtype = EV_GUI_LButtonDown; break;
			case NSRightMouseDown: event.subtype = EV_GUI_RButtonDown; break;
			case NSOtherMouseDown: event.subtype = EV_GUI_MButtonDown; break;
			case NSLeftMouseUp:    event.subtype = EV_GUI_LButtonUp;   break;
			case NSRightMouseUp:   event.subtype = EV_GUI_LButtonUp;   break;
			case NSOtherMouseUp:   event.subtype = EV_GUI_LButtonUp;   break;
		}
		
		NSEventToGameMousePosition( theEvent, &event );
		
		D_PostEvent( &event );
	}
	else
	{
		switch ( cocoaEventType )
		{
			case NSLeftMouseDown:
			case NSRightMouseDown:
			case NSOtherMouseDown:
				event.type = EV_KeyDown;
				break;
				
			case NSLeftMouseUp:
			case NSRightMouseUp:
			case NSOtherMouseUp:
				event.type = EV_KeyUp;
				break;
		}
		
		event.data1 = std::min( KEY_MOUSE1 + cocoaMouseButton, KEY_MOUSE8 );
		
		D_PostEvent( &event );
	}
}


static void ProcessMouseMoveInMenu( NSEvent* theEvent )
{
	event_t event;
	memset( &event, 0, sizeof( event ) );
	
	event.type = EV_GUI_Event;
	event.subtype = EV_GUI_MouseMove;
	
	NSEventToGameMousePosition( theEvent, &event );
	
	D_PostEvent( &event );
}

static void ProcessMouseMoveInGame( NSEvent* theEvent )
{
	if ( !use_mouse )
	{
		return;
	}	
	
	// TODO: remove this magic!
	
	if ( s_skipMouseMoves > 0 )
	{
		--s_skipMouseMoves;
		return;
	}
	
	int x(  [theEvent deltaX] );
	int y( -[theEvent deltaY] );
	
	if ( 0 == x && 0 == y )
	{
		return;
	}
	
	if ( !m_noprescale )
	{
		x *= 3;
		y *= 2;
	}	
	
	event_t event;
	memset( &event, 0, sizeof( event ) );
	
	static int lastX = 0, lastY = 0;
	
	if ( m_filter )
	{
		event.x = ( x + lastX ) / 2;
		event.y = ( y + lastY ) / 2;
	}
	else
	{
		event.x = x;
		event.y = y;
	}
	
	lastX = x;
	lastY = y;
	
	if ( 0 != event.x | 0 != event.y )
	{
		event.type = EV_Mouse;
		
		D_PostEvent( &event );
	}	
}

static void ProcessMouseMoveEvent( NSEvent* theEvent )
{	
	if ( GUICapture )
	{
		ProcessMouseMoveInMenu( theEvent );
	}
	else
	{
		ProcessMouseMoveInGame( theEvent );
	}
}


static void ProcessMouseWheelEvent( NSEvent* theEvent )
{
	const CGFloat delta    = [theEvent deltaY];
	const bool isZeroDelta = fabs( delta ) < 1.0E-5;

	if ( isZeroDelta && GUICapture )
	{
		return;
	}
	
	event_t event;
	memset( &event, 0, sizeof( event ) );
	
	if ( GUICapture )
	{
		event.type    = EV_GUI_Event;
		event.subtype = delta > 0.0f ? EV_GUI_WheelUp : EV_GUI_WheelDown;
		event.data3   = delta;
		event.data3   = ModifierFlagsToGUIKeyModifiers( theEvent );
	}
	else
	{
		event.type  = isZeroDelta  ? EV_KeyUp     : EV_KeyDown;
		event.data1 = delta > 0.0f ? KEY_MWHEELUP : KEY_MWHEELDOWN;
	}
	
	D_PostEvent( &event );
}


// ---------------------------------------------------------------------------


@interface ApplicationDelegate : NSResponder
{
@private
    FullscreenWindow* m_window;
}

- (id)init;
- (void)dealloc;

- (void)keyDown:(NSEvent*)theEvent;
- (void)keyUp:(NSEvent*)theEvent;

- (void)applicationDidBecomeActive:(NSNotification*)aNotification;
- (void)applicationWillResignActive:(NSNotification*)aNotification;

- (FullscreenWindow*)window;

- (void)processEvents:(NSTimer*)timer;

@end


static ApplicationDelegate* s_applicationDelegate;


@implementation ApplicationDelegate

- (id)init
{
	self = [super init];
	
	// Setup timer for event loop
	
	NSTimer* timer = [NSTimer timerWithTimeInterval:0
											 target:self
										   selector:@selector(processEvents:)
										   userInfo:nil
											repeats:YES];
	[[NSRunLoop mainRunLoop] addTimer:timer
							  forMode:NSDefaultRunLoopMode];
	
	// Create window
	
	m_window = [[FullscreenWindow alloc] initWithContentRect:NSMakeRect(0, 0, 640, 480)
												   styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask
													 backing:NSBackingStoreBuffered
													   defer:NO];
	[m_window setOpaque:YES];
	[m_window makeFirstResponder:self];
	[m_window setAcceptsMouseMovedEvents:YES];
	
	// Create OpenGL context and view
	
	const NSRect contentRect = [m_window contentRectForFrameRect:[m_window frame]];
	
	NSOpenGLPixelFormatAttribute attributes[] = 
	{
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize,
		(NSOpenGLPixelFormatAttribute)32,
		NSOpenGLPFADepthSize,
		(NSOpenGLPixelFormatAttribute)24,
		NSOpenGLPFAStencilSize,
		(NSOpenGLPixelFormatAttribute)8,
		0
	};
	
	NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	NSOpenGLView *glView = [[NSOpenGLView alloc] initWithFrame:contentRect
												   pixelFormat:pixelFormat];
	[[glView openGLContext] makeCurrentContext];
	
	[m_window setContentView:glView];
	
	return self;
}

- (void)dealloc
{
	[m_window release];
	
	[super dealloc];
}


- (void)keyDown:(NSEvent*)theEvent
{
	// Empty but present to avoid playing of 'beep' alert sound
}

- (void)keyUp:(NSEvent*)theEvent;
{
	// Empty but present to avoid playing of 'beep' alert sound
}


- (void)applicationDidBecomeActive:(NSNotification*)aNotification
{
	S_SetSoundPaused(1);
}

- (void)applicationWillResignActive:(NSNotification*)aNotification
{
	S_SetSoundPaused(0);
}


- (FullscreenWindow*)window
{
	return m_window;
}


- (void)processEvents:(NSTimer*)timer
{
    while ( true )
    {
        NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask
											untilDate:[NSDate dateWithTimeIntervalSinceNow:0]
											   inMode:NSDefaultRunLoopMode
											  dequeue:YES];
        if ( nil == event )
        {
            break;
        }
		
		const NSEventType eventType = [event type];
		
		switch ( eventType )
		{
			case NSMouseMoved:
				ProcessMouseMoveEvent( event );
				break;
				
			case NSLeftMouseDown:
			case NSLeftMouseUp:
			case NSRightMouseDown:
			case NSRightMouseUp:
			case NSOtherMouseDown:
			case NSOtherMouseUp:
				ProcessMouseButtonEvent( event );
				break;
				
			case NSLeftMouseDragged:
			case NSRightMouseDragged:
			case NSOtherMouseDragged:
				ProcessMouseButtonEvent( event );
				ProcessMouseMoveEvent( event );
				break;
				
			case NSScrollWheel:
				ProcessMouseWheelEvent( event );
				break;
				
			case NSKeyDown:
			case NSKeyUp:
			case NSFlagsChanged:
				ProcessKeyboardEvent( event );
				break;
		}
		
		[NSApp sendEvent:event];
	}
    
    [NSApp updateWindows];
}

@end


// ---------------------------------------------------------------------------


namespace CocoaApplication
{
	bool Init()
	{
		if ( nil != s_applicationDelegate )
		{
			assert( !"CocoaApplication::Init() called more than one time" );
			return false;
		}
		
		[NSApplication sharedApplication];
		[NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
		
		s_applicationDelegate = [ApplicationDelegate new];		
		[NSApp setDelegate:s_applicationDelegate];
		
		[NSApp finishLaunching];
		
		// When starting from command line with real executable path, e.g. GZDoom.app/Contents/MacOS/GZDoom
		// application remains deactivated for an unknown reason.
		// The following call resolves this issue
		[NSApp activateIgnoringOtherApps:YES];
		
		return true;		
	}
	
	void Shutdown()
	{
		[s_applicationDelegate release];
	}
	
	
	void ProcessEvents()
	{
		[[NSRunLoop mainRunLoop] limitDateForMode:NSDefaultRunLoopMode];
	}
	
	
	void SetCursorVisible( const bool visible )
	{
		if ( visible )
		{
			[NSCursor unhide];
		}
		else
		{
			[NSCursor hide];
		}
	}
		
	
	void SetVideoResolution( const int width, const int height, const bool fullscreen )
	{
		FullscreenWindow* window = [s_applicationDelegate window];
		[window setFullscreen:fullscreen];
		
		CGLContextObj context = CGLGetCurrentContext();
		
		if ( fullscreen )
		{
			GLint resolution[2] = { width, height };
			
			CGLSetParameter( context, kCGLCPSurfaceBackingSize, resolution );
			CGLEnable( context, kCGLCESurfaceBackingSize );			
			
			const NSRect displayRect = [[window screen] frame];
			
			s_pixelScaleFactor = displayRect.size.width / static_cast< float >( width );
			
			[window setLevel:NSMainMenuWindowLevel + 1];
			[window setStyleMask:NSBorderlessWindowMask];
			[window setHidesOnDeactivate:YES];
			[window setFrame:displayRect display:YES];
			[window setFrameOrigin:NSMakePoint( 0.0f, 0.0f )];
		}
		else
		{
			CGLDisable( context, kCGLCESurfaceBackingSize );
			
			s_pixelScaleFactor = 1.0f;
			
			[window setLevel:NSNormalWindowLevel];
			[window setStyleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask];
			[window setHidesOnDeactivate:NO];
			[window setContentSize:NSMakeSize( width, height )];
			[window center];
		}

		glClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		
		SwapVideoBuffers();		
		
		static const NSString* const TITLE_STRING = 
			[NSString stringWithUTF8String:GAMESIG " " DOTVERSIONSTR " (" __DATE__ ")"];
		[window setTitle:TITLE_STRING];		
		
		if ( ![window isKeyWindow] )
		{
			[window makeKeyAndOrderFront:nil];
		}
	}
	
	void SwapVideoBuffers()
	{
		[[NSOpenGLContext currentContext] flushBuffer];
	}
	
	
	ScreenSize GetScreenSize()
	{
		const NSSize mainDisplaySize = [[NSScreen mainScreen] frame].size;
		return ScreenSize( mainDisplaySize.width, mainDisplaySize.height );
	}
	
	
	bool IsFullscreen()
	{
		return [[s_applicationDelegate window] fullscreen];
	}
	
	bool IsActive()
	{
		return [NSApp isActive];
	}

	
	bool GetGammaRamp( uint16_t* red, uint16_t* green, uint16_t* blue )
	{
		static const uint32_t    TABLE_SIZE = 256;
		CGGammaValue   redTable[ TABLE_SIZE ];
		CGGammaValue greenTable[ TABLE_SIZE ];
		CGGammaValue  blueTable[ TABLE_SIZE ];
		
		uint32_t actual = 0;
		
		if ( CGDisplayNoErr != CGGetDisplayTransferByTable( kCGDirectMainDisplay, TABLE_SIZE, 
				redTable, greenTable, blueTable, &actual )
			|| actual != TABLE_SIZE )
		{
			return false;
		}
		
		for ( size_t i = 0; i < TABLE_SIZE; ++i )
		{
			  red[i] = uint16_t(   redTable[i] * 65535.0f );
			green[i] = uint16_t( greenTable[i] * 65535.0f );
			 blue[i] = uint16_t(  blueTable[i] * 65535.0f );			
		}
		
		return true;
	}
	
	bool SetGammaRamp( const uint16_t* red, const uint16_t* green, const uint16_t* blue )
	{
		static const uint32_t    TABLE_SIZE = 256;
		CGGammaValue   redTable[ TABLE_SIZE ];
		CGGammaValue greenTable[ TABLE_SIZE ];
		CGGammaValue  blueTable[ TABLE_SIZE ];
		
		// Extract gamma values into separate tables
		// Convert to floats between 0.0 and 1.0
		
		for ( size_t i = 0; i < TABLE_SIZE; ++i )
		{
			  redTable[i] =   red[i] / 65535.0f;
			greenTable[i] = green[i] / 65535.0f;
			 blueTable[i] =  blue[i] / 65535.0f;
		}
		
		return CGDisplayNoErr == CGSetDisplayTransferByTable( kCGDirectMainDisplay, 
			TABLE_SIZE, redTable, greenTable, blueTable );
	}

} // end of CocoaApplication namespace
