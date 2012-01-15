/*
 ** iwadpicker_cocoa.mm
 **
 ** Implements Mac OS X native IWAD Picker.
 **
 **---------------------------------------------------------------------------
 ** Copyright 2010 Braden Obrzut
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

#include "d_main.h"
#include "version.h"
#include "c_cvars.h"
#include "m_argv.h"
#include "m_misc.h"
#include <Cocoa/Cocoa.h>

CVAR( String, macosx_additional_parameters, "", CVAR_ARCHIVE | CVAR_NOSET | CVAR_GLOBALCONFIG );

enum
{
	COLUMN_IWAD,
	COLUMN_GAME,

	NUM_COLUMNS
};

static const char* const tableHeaders[NUM_COLUMNS] = { "IWAD", "Game" };

// Class to convert the IWAD data into a form that Cocoa can use.
@interface IWADTableData : NSObject 
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
<NSTableViewDataSource>
#endif
{
	NSMutableArray *data;
}

- (void)dealloc;
- (IWADTableData *)init:(WadStuff *) wads:(int) numwads;

- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
@end

@implementation IWADTableData

- (void)dealloc
{
	[data release];

	[super dealloc];
}

- (IWADTableData *)init:(WadStuff *) wads:(int) numwads
{
	data = [[NSMutableArray alloc] initWithCapacity:numwads];

	for(int i = 0;i < numwads;i++)
	{
		NSMutableDictionary *record = [[NSMutableDictionary alloc] initWithCapacity:NUM_COLUMNS];
		const char* filename = strrchr(wads[i].Path, '/');
		if(filename == NULL)
			filename = wads[i].Path;
		else
			filename++;
		[record setObject:[NSString stringWithUTF8String:filename] forKey:[NSString stringWithUTF8String:tableHeaders[COLUMN_IWAD]]];
		[record setObject:[NSString stringWithUTF8String:wads[i].Name] forKey:[NSString stringWithUTF8String:tableHeaders[COLUMN_GAME]]];
		[data addObject:record];
		[record release];
	}

	return self;
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
	return [data count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
	NSParameterAssert(rowIndex >= 0 && (unsigned int) rowIndex < [data count]);
	NSMutableDictionary *record = [data objectAtIndex:rowIndex];
	return [record objectForKey:[aTableColumn identifier]];
}

@end

// So we can listen for button actions and such we need to have an Obj-C class.
@interface IWADPicker : NSObject
{
	NSApplication *app;
	NSWindow *window;
	NSButton *okButton;
	NSButton *cancelButton;
	NSTextField *parametersTextField;
	bool cancelled;
}

- (void)buttonPressed:(id) sender;
- (void)doubleClicked:(id) sender;
- (void)makeLabel:(NSTextField *)label:(const char*) str;
- (int)pickIWad:(WadStuff *)wads:(int) numwads:(bool) showwin:(int) defaultiwad;
- (NSString*)commandLineParameters;
- (void)menuActionSent:(NSNotification*)notification;
@end

@implementation IWADPicker

- (void)buttonPressed:(id) sender;
{
	if(sender == cancelButton)
		cancelled = true;

	[window orderOut:self];
	[app stopModal];
}

- (void)doubleClicked:(id) sender;
{
	if ([sender clickedRow] >= 0)
	{
		[window orderOut:self];
		[app stopModal];
	}
}

// Apparently labels in Cocoa are uneditable text fields, so lets make this a
// little more automated.
- (void)makeLabel:(NSTextField *)label:(const char*) str
{
	[label setStringValue:[NSString stringWithUTF8String:str]];
	[label setBezeled:NO];
	[label setDrawsBackground:NO];
	[label setEditable:NO];
	[label setSelectable:NO];
}

- (int)pickIWad:(WadStuff *)wads:(int) numwads:(bool) showwin:(int) defaultiwad
{
	cancelled = false;

	app = [NSApplication sharedApplication];
	id windowTitle = [NSString stringWithUTF8String:GAMESIG " " DOTVERSIONSTR];

	NSRect frame = NSMakeRect(0, 0, 440, 450);
	window = [[NSWindow alloc] initWithContentRect:frame styleMask:NSTitledWindowMask backing:NSBackingStoreBuffered defer:NO];
	[window setTitle:windowTitle];

	NSTextField *description = [[NSTextField alloc] initWithFrame:NSMakeRect(18, 384, 402, 50)];
	[self makeLabel:description:"GZDoom found more than one IWAD\nSelect from the list below to determine which one to use:"];
	[[window contentView] addSubview:description];
	[description release];

	NSScrollView *iwadScroller = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 135, 402, 256)];
	NSTableView *iwadTable = [[NSTableView alloc] initWithFrame:[iwadScroller bounds]];
	IWADTableData *tableData = [[IWADTableData alloc] init:wads:numwads];
	for(int i = 0;i < NUM_COLUMNS;i++)
	{
		NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:[NSString stringWithUTF8String:tableHeaders[i]]];
		[[column headerCell] setStringValue:[column identifier]];
		if(i == 0)
			[column setMaxWidth:110];
		[column setEditable:NO];
		[column setResizingMask:NSTableColumnAutoresizingMask];
		[iwadTable addTableColumn:column];
		[column release];
	}
	[iwadScroller setDocumentView:iwadTable];
	[iwadScroller setHasVerticalScroller:YES];
	[iwadTable setDataSource:tableData];
	[iwadTable sizeToFit];
	[iwadTable setDoubleAction:@selector(doubleClicked:)];
	[iwadTable setTarget:self];
	NSIndexSet *selection = [[NSIndexSet alloc] initWithIndex:defaultiwad];
	[iwadTable selectRowIndexes:selection byExtendingSelection:NO];
	[selection release];
	[iwadTable scrollRowToVisible:defaultiwad];
	[[window contentView] addSubview:iwadScroller];
	[iwadTable release];
	[iwadScroller release];

	NSTextField *additionalParametersLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(18, 108, 144, 17)];
	[self makeLabel:additionalParametersLabel:"Additional Parameters:"];
	[[window contentView] addSubview:additionalParametersLabel];
	parametersTextField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 48, 402, 54)];
	[parametersTextField setStringValue:[NSString stringWithUTF8String:macosx_additional_parameters]];
	[[window contentView] addSubview:parametersTextField];

	// Doesn't look like the SDL version implements this so lets not show it.
	/*NSButton *dontAsk = [[NSButton alloc] initWithFrame:NSMakeRect(18, 18, 178, 18)];
	[dontAsk setTitle:[NSString stringWithCString:"Don't ask me this again"]];
	[dontAsk setButtonType:NSSwitchButton];
	[dontAsk setState:(showwin ? NSOffState : NSOnState)];
	[[window contentView] addSubview:dontAsk];*/

	okButton = [[NSButton alloc] initWithFrame:NSMakeRect(236, 8, 96, 32)];
	[okButton setTitle:@"OK"];
	[okButton setBezelStyle:NSRoundedBezelStyle];
	[okButton setAction:@selector(buttonPressed:)];
	[okButton setTarget:self];
	[okButton setKeyEquivalent:@"\r"];
	[[window contentView] addSubview:okButton];

	cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(332, 8, 96, 32)];
	[cancelButton setTitle:@"Cancel"];
	[cancelButton setBezelStyle:NSRoundedBezelStyle];
	[cancelButton setAction:@selector(buttonPressed:)];
	[cancelButton setTarget:self];
	[cancelButton setKeyEquivalent:@"\033"];
	[[window contentView] addSubview:cancelButton];

	NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(menuActionSent:) name:NSMenuDidSendActionNotification object:nil];

	[app runModalForWindow:window];

	[center removeObserver:self name:NSMenuDidSendActionNotification object:nil];

	[window release];
	[okButton release];
	[cancelButton release];

	return cancelled ? -1 : [iwadTable selectedRow];
}

- (NSString*)commandLineParameters
{
	return [parametersTextField stringValue];
}

- (void)menuActionSent:(NSNotification*)notification
{
	NSDictionary* userInfo = [notification userInfo];
	NSMenuItem* menuItem = [userInfo valueForKey:@"MenuItem"];

	if ( @selector(terminate:) == [menuItem action] )
	{
		exit(0);
	}
}

@end


static void RestartWithParameters( const char* iwadPath, NSString* parameters )
{
	assert( nil != parameters );
	
	M_SaveDefaults( NULL );
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	@try
	{
		const int commandLineParametersCount = Args->NumArgs();
		assert( commandLineParametersCount > 0 );
		
		NSString* executablePath = [NSString stringWithUTF8String:Args->GetArg(0)];
		
		NSMutableArray* arguments = [NSMutableArray arrayWithCapacity:commandLineParametersCount + 2];
		[arguments addObject:@"-iwad"];
		[arguments addObject:[NSString stringWithUTF8String:iwadPath]];
		
		for ( int i = 1; i < commandLineParametersCount; ++i )
		{
			NSString* currentParameter = [NSString stringWithUTF8String:Args->GetArg(i)];
			[arguments addObject:currentParameter];
		}
		
		NSArray* additionalParameters = [parameters componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		[arguments addObjectsFromArray:additionalParameters];
	
#if 0
		NSTask* task = [[NSTask alloc] init];
		[task setLaunchPath:executablePath];
		[task setArguments:arguments];
		
		[task launch];
		[task waitUntilExit];		
#else
		[NSTask launchedTaskWithLaunchPath:executablePath arguments:arguments];
#endif
		
		_exit(0); // to avoid atexit()'s functions
	}
	@catch ( NSException* e )
	{
		NSLog( @"Cannot restart: %@", [e reason] );
	}
	
	[pool release];
}

// Simple wrapper so we can call this from outside.
int I_PickIWad_Cocoa (WadStuff *wads, int numwads, bool showwin, int defaultiwad)
{
	IWADPicker *picker = [IWADPicker alloc];
	int ret = [picker pickIWad:wads:numwads:showwin:defaultiwad];
	
	NSString* parametersToAppend = [picker commandLineParameters];
	macosx_additional_parameters = [parametersToAppend UTF8String];
	
	if ( ret >= 0 )
	{
		if ( 0 != [parametersToAppend length] )
		{
			RestartWithParameters( wads[ ret ].Path, parametersToAppend );
		}
	}
	
	[picker release];
	return ret;
}
