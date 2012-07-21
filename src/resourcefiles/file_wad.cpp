/*
** file_wad.cpp
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** Copyright 2005-2009 Christoph Oelckers
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
**
*/

#include "resourcefile.h"
#include "cmdlib.h"
#include "v_text.h"
#include "w_wad.h"

//==========================================================================
//
// Wad file
//
//==========================================================================

class FWadFile : public FUncompressedFile
{
	bool IsMarker(int lump, const char *marker);
	void SetNamespace(const char *startmarker, const char *endmarker, namespace_t space, bool flathack=false);
	void SkinHack ();

public:
	FWadFile(const char * filename, FileReader *file);
	void FindStrifeTeaserVoices ();
	bool Open(bool quiet);
};


//==========================================================================
//
// FWadFile::FWadFile
//
// Initializes a WAD file
//
//==========================================================================

FWadFile::FWadFile(const char *filename, FileReader *file) : FUncompressedFile(filename, file)
{
	Lumps = NULL;
}

//==========================================================================
//
// Open it
//
//==========================================================================

bool FWadFile::Open(bool quiet)
{
	wadinfo_t header;

	Reader->Read(&header, sizeof(header));
	NumLumps = LittleLong(header.NumLumps);
	header.InfoTableOfs = LittleLong(header.InfoTableOfs);
	
	wadlump_t *fileinfo = new wadlump_t[NumLumps];
	Reader->Seek (header.InfoTableOfs, SEEK_SET);
	Reader->Read (fileinfo, NumLumps * sizeof(wadlump_t));

	Lumps = new FUncompressedLump[NumLumps];

	if (!quiet) Printf(", %d lumps\n", NumLumps);

	for(DWORD i = 0; i < NumLumps; i++)
	{
		uppercopy (Lumps[i].Name, fileinfo[i].Name);
		Lumps[i].Name[8] = 0;
		Lumps[i].Owner = this;
		Lumps[i].Position = LittleLong(fileinfo[i].FilePos);
		Lumps[i].LumpSize = LittleLong(fileinfo[i].Size);
		Lumps[i].Namespace = ns_global;
		Lumps[i].Flags = 0;
		Lumps[i].FullName = NULL;
	}

	if (!quiet)	// don't bother with namespaces here. We won't need them.
	{
		SetNamespace("S_START", "S_END", ns_sprites);
		SetNamespace("F_START", "F_END", ns_flats, true);
		SetNamespace("C_START", "C_END", ns_colormaps);
		SetNamespace("A_START", "A_END", ns_acslibrary);
		SetNamespace("TX_START", "TX_END", ns_newtextures);
		SetNamespace("T_START", "T_END", ns_newtextures); // Doom 64 Textures
		SetNamespace("V_START", "V_END", ns_strifevoices);
		SetNamespace("HI_START", "HI_END", ns_hires);
		SetNamespace("VX_START", "VX_END", ns_voxels);
		SkinHack();
	}
	delete [] fileinfo;
	return true;
}

//==========================================================================
//
// IsMarker
//
// (from BOOM)
//
//==========================================================================

inline bool FWadFile::IsMarker(int lump, const char *marker)
{
	if (Lumps[lump].Name[0] == marker[0])
	{
		return (!strcmp(Lumps[lump].Name, marker) ||
			(marker[1] == '_' && !strcmp(Lumps[lump].Name+1, marker)));
	}
	else return false;
}

//==========================================================================
//
// SetNameSpace
//
// Sets namespace information for the lumps. It always looks for the first
// x_START and the last x_END lump, except when loading flats. In this case
// F_START may be absent and if that is the case all lumps with a size of
// 4096 will be flagged appropriately.
//
//==========================================================================

// This class was supposed to be local in the function but GCC
// does not like that.
struct Marker
{
	int markertype;
	unsigned int index;
};

void FWadFile::SetNamespace(const char *startmarker, const char *endmarker, namespace_t space, bool flathack)
{
	bool warned = false;
	int numstartmarkers = 0, numendmarkers = 0;
	unsigned int i;
	TArray<Marker> markers;
	
	for(i = 0; i < NumLumps; i++)
	{
		if (IsMarker(i, startmarker))
		{
			Marker m = { 0, i };
			markers.Push(m);
			numstartmarkers++;
		}
		else if (IsMarker(i, endmarker))
		{
			Marker m = { 1, i };
			markers.Push(m);
			numendmarkers++;
		}
	}

	if (numstartmarkers == 0)
	{
		if (numendmarkers == 0) return;	// no markers found

		Printf(TEXTCOLOR_YELLOW"WARNING: %s marker without corresponding %s found.\n", endmarker, startmarker);

		
		if (flathack)
		{
			// We have found no F_START but one or more F_END markers.
			// mark all lumps before the last F_END marker as potential flats.
			unsigned int end = markers[markers.Size()-1].index;
			for(unsigned int i = 0; i < end; i++)
			{
				if (Lumps[i].LumpSize == 4096)
				{
					// We can't add this to the flats namespace but 
					// it needs to be flagged for the texture manager.
					DPrintf("Marking %s as potential flat\n", Lumps[i].Name);
					Lumps[i].Flags |= LUMPF_MAYBEFLAT;
				}
			}
		}
		return;
	}

	i = 0;
	while (i < markers.Size())
	{
		int start, end;
		if (markers[i].markertype != 0)
		{
			Printf(TEXTCOLOR_YELLOW"WARNING: %s marker without corresponding %s found.\n", endmarker, startmarker);
			i++;
			continue;
		}
		start = i++;

		// skip over subsequent x_START markers
		while (i < markers.Size() && markers[i].markertype == 0)
		{
			Printf(TEXTCOLOR_YELLOW"WARNING: duplicate %s marker found.\n", startmarker);
			i++;
			continue;
		}
		// same for x_END markers
		while (i < markers.Size()-1 && (markers[i].markertype == 1 && markers[i+1].markertype == 1))
		{
			Printf(TEXTCOLOR_YELLOW"WARNING: duplicate %s marker found.\n", endmarker);
			i++;
			continue;
		}
		// We found a starting marker but no end marker. Ignore this block.
		if (i >= markers.Size())
		{
			Printf(TEXTCOLOR_YELLOW"WARNING: %s marker without corresponding %s found.\n", startmarker, endmarker);
			end = NumLumps;
		}
		else
		{
			end = markers[i++].index;
		}

		// we found a marked block
		DPrintf("Found %s block at (%d-%d)\n", startmarker, markers[start].index, end);
		for(int j = markers[start].index + 1; j < end; j++)
		{
			if (Lumps[j].Namespace != ns_global)
			{
				if (!warned)
				{
					Printf(TEXTCOLOR_YELLOW"WARNING: Overlapping namespaces found (lump %d)\n", j);
				}
				warned = true;
			}
			else if (space == ns_sprites && Lumps[j].LumpSize < 8)
			{
				// sf 26/10/99:
				// ignore sprite lumps smaller than 8 bytes (the smallest possible)
				// in size -- this was used by some dmadds wads
				// as an 'empty' graphics resource
				DPrintf(" Skipped empty sprite %s (lump %d)\n", Lumps[j].Name, j);
			}
			else
			{
				Lumps[j].Namespace = space;
			}
		}
	}
}


//==========================================================================
//
// W_SkinHack
//
// Tests a wad file to see if it contains an S_SKIN marker. If it does,
// every lump in the wad is moved into a new namespace. Because skins are
// only supposed to replace player sprites, sounds, or faces, this should
// not be a problem. Yes, there are skins that replace more than that, but
// they are such a pain, and breaking them like this was done on purpose.
// This also renames any S_SKINxx lumps to just S_SKIN.
//
//==========================================================================

void FWadFile::SkinHack ()
{
	static int namespc = ns_firstskin;
	bool skinned = false;
	bool hasmap = false;
	DWORD i;

	for (i = 0; i < NumLumps; i++)
	{
		FResourceLump *lump = &Lumps[i];

		if (lump->Name[0] == 'S' &&
			lump->Name[1] == '_' &&
			lump->Name[2] == 'S' &&
			lump->Name[3] == 'K' &&
			lump->Name[4] == 'I' &&
			lump->Name[5] == 'N')
		{ // Wad has at least one skin.
			lump->Name[6] = lump->Name[7] = 0;
			if (!skinned)
			{
				skinned = true;
				DWORD j;

				for (j = 0; j < NumLumps; j++)
				{
					Lumps[j].Namespace = namespc;
				}
				namespc++;
			}
		}
		if (lump->Name[0] == 'M' &&
			lump->Name[1] == 'A' &&
			lump->Name[2] == 'P')
		{
			hasmap = true;
		}
	}
	if (skinned && hasmap)
	{
		Printf (TEXTCOLOR_BLUE
			"The maps in %s will not be loaded because it has a skin.\n"
			TEXTCOLOR_BLUE
			"You should remove the skin from the wad to play these maps.\n",
			Filename);
	}
}


//==========================================================================
//
// FindStrifeTeaserVoices
//
// Strife0.wad does not have the voices between V_START/V_END markers, so
// figure out which lumps are voices based on their names.
//
//==========================================================================

void FWadFile::FindStrifeTeaserVoices ()
{
	for (DWORD i = 0; i <= NumLumps; ++i)
	{
		if (Lumps[i].Name[0] == 'V' &&
			Lumps[i].Name[1] == 'O' &&
			Lumps[i].Name[2] == 'C')
		{
			int j;

			for (j = 3; j < 8; ++j)
			{
				if (Lumps[i].Name[j] != 0 && !isdigit(Lumps[i].Name[j]))
					break;
			}
			if (j == 8)
			{
				Lumps[i].Namespace = ns_strifevoices;
			}
		}
	}
}


//==========================================================================
//
// File open
//
//==========================================================================

FResourceFile *CheckWad(const char *filename, FileReader *file, bool quiet)
{
	char head[4];

	if (file->GetLength() >= 12)
	{
		file->Seek(0, SEEK_SET);
		file->Read(&head, 4);
		file->Seek(0, SEEK_SET);
		if (!memcmp(head, "IWAD", 4) || !memcmp(head, "PWAD", 4))
		{
			FResourceFile *rf = new FWadFile(filename, file);
			if (rf->Open(quiet)) return rf;
			delete rf;
		}
	}
	return NULL;
}



//==========================================================================
//
// Disk file
//
//==========================================================================


class FEmbeddedWadFile : public FWadFile
{
public:
	FEmbeddedWadFile( const char* filename, FileReader* reader, FResourceFile* owner );
	
	void CopyLumps( FUncompressedLump* dest );

private:
	FResourceFile* m_owner;
	
	int m_offset;
};


FEmbeddedWadFile::FEmbeddedWadFile( const char* filename, FileReader* reader, FResourceFile* owner )
: FWadFile( filename, reader )
, m_owner ( owner )
, m_offset( static_cast< int >( owner->GetReader()->Tell() ) )
{
	
}

void FEmbeddedWadFile::CopyLumps( FUncompressedLump* dest )
{
	if ( NULL == Lumps || NULL == dest )
	{
		return;
	}
	
	for ( DWORD i = 0; i < NumLumps; ++i )
	{
		uppercopy( dest[i].Name, Lumps[i].Name );
		dest[i].Name[8]   = 0;
		dest[i].Owner     = m_owner;
		dest[i].Position  = Lumps[i].Position + m_offset;
		dest[i].LumpSize  = Lumps[i].LumpSize;
		dest[i].Namespace = Lumps[i].Namespace;
		dest[i].Flags     = Lumps[i].Flags;
		dest[i].FullName  = Lumps[i].FullName;
	}
}


//==========================================================================


struct DiskEntry
{
	char  name[ 64 ];
	DWORD offset;
	DWORD size;
};

typedef TArray< DiskEntry > DiskEntryList;


static DWORD GetEntryCount( FileReader* file )
{
	DWORD result;
	
	file->Seek( 0, SEEK_SET );
	file->Read( &result, 4 );
	
	return BigLong( result );
}

static DWORD GetDataOffset( const DWORD entryCount )
{
	return 4 + sizeof( DiskEntry ) * entryCount + 4;
}


union LumpName  // from FResourceLump struct
{
	char  str[9];
	QWORD qword;
};

struct LumpRename
{
	LumpName oldName;
	LumpName newName;
	
};


static void RenameNerveLumps( FUncompressedLump* lumps, const DWORD count )
{
	static const LumpRename RENAME_TABLE[] =
	{
		{ "CWILV00", "NRWILV00" },
		{ "CWILV01", "NRWILV01" },
		{ "CWILV02", "NRWILV02" },
		{ "CWILV03", "NRWILV03" },
		{ "CWILV04", "NRWILV04" },
		{ "CWILV05", "NRWILV05" },
		{ "CWILV06", "NRWILV06" },
		{ "CWILV07", "NRWILV07" },
		{ "CWILV08", "NRWILV08" },
		
		{ "MAP01", "NERVE01" },
		{ "MAP02", "NERVE02" },
		{ "MAP03", "NERVE03" },
		{ "MAP04", "NERVE04" },
		{ "MAP05", "NERVE05" },
		{ "MAP06", "NERVE06" },
		{ "MAP07", "NERVE07" },
		{ "MAP08", "NERVE08" },
		{ "MAP09", "NERVE09" }
	};
	
	assert( NULL != lumps );
	
	for ( DWORD i = 0; i < count; ++i )
	{
		for ( size_t j = 0; j < countof( RENAME_TABLE ); ++j )
		{
			if ( lumps[i].qwName == RENAME_TABLE[j].oldName.qword )
			{
				lumps[i].qwName = RENAME_TABLE[j].newName.qword;
				
				break;
			}
		}
	}
}


//==========================================================================


class FDiskFile : public FUncompressedFile
{
public:
	FDiskFile( const char* path, FileReader* file );
	~FDiskFile();
	
	bool Open( bool quiet );
	
private:
	DiskEntryList     m_entries;
	
	FEmbeddedWadFile* m_iwad;
	FileReader*       m_iwadReader;
	
	FEmbeddedWadFile* m_pwad;
	FileReader*       m_pwadReader;
	
	void ReadEntries();
	bool LoadWADs( bool quiet );
	bool LoadWAD( const char* name, FileReader** reader, FEmbeddedWadFile** wad, bool quiet );
	bool PrepareLumps();
	
	void DumpFiles();
	
};


FDiskFile::FDiskFile( const char* path, FileReader* file )
: FUncompressedFile( path, file )
, m_iwad      ( NULL )
, m_iwadReader( NULL )
, m_pwad      ( NULL )
, m_pwadReader( NULL )
{

}

FDiskFile::~FDiskFile()
{
	delete m_pwad;
	delete m_iwad;
}


bool FDiskFile::Open( bool quiet )
{
	ReadEntries();

#if 0
	DumpFiles();
#endif
	
	return LoadWADs( quiet )
		&& PrepareLumps();
}

void FDiskFile::ReadEntries()
{
	const DWORD entryCount = GetEntryCount( Reader );
	const DWORD dataOffest = GetDataOffset( entryCount );
	
	for ( DWORD i = 0; i < entryCount; ++i )
	{
		DiskEntry entry;
		Reader->Read( &entry, sizeof( entry ) );
		
		entry.offset = BigLong( entry.offset ) + dataOffest;
		entry.size   = BigLong( entry.size   );
		
		m_entries.Push( entry );
	}	
}

bool FDiskFile::LoadWADs( bool quiet )
{
	return LoadWAD( "doom.wad",       &m_iwadReader, &m_iwad, quiet )
		|| LoadWAD( "doom2.wad",      &m_iwadReader, &m_iwad, quiet )
		&& LoadWAD( "nerve_demo.wad", &m_pwadReader, &m_pwad, quiet );
}

bool FDiskFile::LoadWAD( const char* name, FileReader** reader, FEmbeddedWadFile** wad, bool quiet )
{
	const size_t nameLength = strlen( name );
	
	for ( DWORD i = 0, count = m_entries.Size(); i < count; ++i )
	{
		FString entryName = m_entries[i].name;
		entryName.ToLower();
		
		if ( entryName.Len() - nameLength == entryName.IndexOf( name ) )
		{
			Reader->Seek( m_entries[i].offset, SEEK_SET );
			
			*reader = new FileReader( Reader->GetFile(), m_entries[i].size );
			*wad    = new FEmbeddedWadFile( name, *reader, this );
			
			if ( (*wad)->Open( quiet ) )
			{
				return true;
			}
			
			delete *wad;
			*wad = NULL;
		}
	}
	
	return false;
}

bool FDiskFile::PrepareLumps()
{
	const DWORD iwadLumpCount = NULL == m_iwad ? 0 : m_iwad->LumpCount();
	const DWORD pwadLumpCount = NULL == m_pwad ? 0 : m_pwad->LumpCount();
	
	NumLumps = iwadLumpCount + pwadLumpCount;
	
	if ( 0 == NumLumps )
	{
		return false;
	}
	
	Lumps = new FUncompressedLump[ NumLumps ];
	m_iwad->CopyLumps( Lumps );
	
	if ( pwadLumpCount > 0 )
	{
		m_pwad->CopyLumps( Lumps + iwadLumpCount );
		
		RenameNerveLumps( Lumps + iwadLumpCount, pwadLumpCount );
	}
	
	return true;
}


void FDiskFile::DumpFiles()
{
	for ( DWORD i = 0, count = m_entries.Size(); i < count; ++i )
	{
		FString name = m_entries[i].name;
		
		const long colonIndex = name.IndexOf( ":\\" );
		if ( -1 != colonIndex )
		{
			name = name.Mid( colonIndex + 2 );
		}
		
		FixPathSeperator( name );
		CreatePath( ExtractFilePath( name ).GetChars() );
		
		const DWORD size = m_entries[i].size;
		
		TArray< unsigned char> buffer;
		buffer.Resize( size );
		
		Reader->Seek( m_entries[i].offset, SEEK_SET );
		Reader->Read( &buffer[0], size );
		
		FILE* outputFile = fopen( name, "wb" );
		if ( NULL != outputFile )
		{
			fwrite( &buffer[0], size, 1, outputFile );
			fclose( outputFile );
		}
	}
}


FResourceFile* CheckDisk( const char* filename, FileReader* file, bool quiet )
{
	const long fileSize = file->GetLength();
	
	if ( fileSize >= 4 )
	{
		const DWORD entryCount = GetEntryCount( file );
		
		if ( entryCount > 0 && entryCount < 4096
			&& fileSize > GetDataOffset( entryCount ) ) // sanity check
		{
			FResourceFile* result = new FDiskFile( filename, file );
			
			if ( result->Open( quiet ) )
			{
				return result;
			}
			
			delete result;
		}
	}
	
	return NULL;
}

