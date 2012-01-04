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


@interface FullscreenWindow : NSWindow
{
@private
	bool m_fullscreen;
}

- (bool)canBecomeKeyWindow;

- (bool)fullscreen;
- (void)setFullscreen:(bool)on;

@end


@implementation FullscreenWindow

- (bool)canBecomeKeyWindow
{
	return true;
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


@interface ApplicationDelegate : NSResponder
{
@private
    FullscreenWindow* m_window;
}

- (id)init;
- (void)dealloc;

- (FullscreenWindow*)window;

- (void)processEvents:(NSTimer*)timer;

@end


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
		
		[NSApp sendEvent:event];
	}
    
    [NSApp updateWindows];
}

@end


static ApplicationDelegate* s_applicationDelegate;


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
	
	
	void SetWindowTitle( const char* const title )
	{
		NSString* titleString = [NSString stringWithUTF8String:title];
		
		NSWindow* window = [s_applicationDelegate window];
		[window setTitle:titleString];
	}
	
	
	void SetCursorVisible( const bool visible )
	{
		if ( visible )
		{
			CGDisplayShowCursor( kCGDirectMainDisplay );
		}
		else
		{
			CGDisplayHideCursor( kCGDirectMainDisplay );
		}
	}
	
	
	void SetVideoResolution( const int width, const int height, const bool fullscreen )
	{
		FullscreenWindow* window = [s_applicationDelegate window];
		[window setFullscreen:fullscreen];
		
		if ( fullscreen )
		{
			const NSRect mainDisplayRect = [[NSScreen mainScreen] frame];
			
			[window setLevel:NSMainMenuWindowLevel + 1];
			[window setStyleMask:NSBorderlessWindowMask];
			[window setHidesOnDeactivate:YES];
			[window setFrame:mainDisplayRect display:YES];
			[window setFrameOrigin:NSMakePoint( 0.0f, 0.0f )];
		}
		else
		{
			[window setLevel:NSNormalWindowLevel];
			[window setStyleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask];
			[window setHidesOnDeactivate:NO];
			[window setContentSize:NSMakeSize( width, height )];
			[window center];
		}
				
		if ( ![window isKeyWindow] )
		{
			glClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
			
			SwapVideoBuffers();			
			
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
