/*
 *	wadext2.c - dump the lumps of a wad to files
 *	AYM 2001-08-10
 */


/*
This file is copyright André Majorel 2001.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include "wadext2.h"
#include "format.h"


static int process_wad (const char *pathname, const char *ofmt);
static int gets32l (FILE *fp, long *value);
static int getname (FILE *fp, char *buf);


static int dry_run = 0;			/* -n */
static int verbose = 0;			/* -v */


int main (int argc, char *argv[])
{
  int rc = 0;
  const char *output = "%04n_%l.lmp";	/* ####_lumpname.lmp */

  /* Parse the command line */
  if (argc >= 2 && strcmp (argv[1], "--help") == 0)
  {
    printf ("Usage:\n"
      "  wadext2 --help\n"
      "  wadext2 --version\n"
      "  wadext2 [-nv] [-o fmt] file\n");
    printf ("Options\n"
      "  --help     Print help to stdout and exit successfully\n"
      "  -n         Do everything as normal but don't write any output files\n"
      "  -o fmt     Name output files according to fmt (default \"%s\")\n"
      "  -v         Verbose\n"
      "  --version  Print version number to stdout and exit successfully\n",
      output);
    exit (0);
  }
  if (argc >= 2 && strcmp (argv[1], "--version") == 0)
  {
    puts (version);
    exit (0);
  }
  {
    int g;

    while ((g = getopt (argc, argv, "no:v")) != EOF)
    {
      if (g == 'n')
	dry_run = 1;
      else if (g == 'o')
	output = optarg;
      else if (g == 'v')
	verbose = 1;
      else
	exit (1);
    }
  }
  if (argc - optind != 1)
  {
    err ("wrong number of arguments");
    exit (1);
  }

  /* Process all files */
  {
    if (process_wad (argv[optind], output) != 0)
      rc = 1;
  }

  return rc;
}


/*
 *	process_wad - extract all lumps from a wad file
 *
 *	Return 0 on success, non-zero on failure
 */
#define FAIL do { rc = 1; goto byebye; } while (0)

static int process_wad (const char *pathname, const char *ofmt)
{
  int rc = 0;
  FILE *fp = NULL;
  unsigned long offset;
  long magic;
  long dirofs;
  long dirents;
  waddirent_t *dir = NULL;

  /* Open the wad */
  fp = fopen (pathname, "rb");
  if (fp == NULL)
  {
    err ("%s: %s", pathname, strerror (errno));
    FAIL;
  }

  /* Parse the header */
  offset = ftell (fp);
  if (gets32l (fp, &magic) != 0)
  {
    err ("%s(%lXh): short read in header", pathname, offset);
    FAIL;
  }
  if (magic != 0x44415749 && magic != 0x44415750)  /* "IWAD" or "PWAD" */
    warn ("%s: bad magic", pathname);

  offset = ftell (fp);
  if (gets32l (fp, &dirents) != 0)
  {
    err ("%s(%lXh): short read in header", pathname, offset);
    FAIL;
  }
  if (dirents < 0)
  {
    err ("%s: negative directory size", pathname);
    FAIL;
  }
  if (dirents > 10000)
    warn ("%s: unlikely directory size", pathname);

  offset = ftell (fp);
  if (gets32l (fp, &dirofs) != 0)
  {
    err ("%s(%lXh): short read in header", pathname, offset);
    FAIL;
  }
  if (dirofs < 0)
  {
    err ("%s: negative directory offset", pathname);
    FAIL;
  }
  if (dirofs > 100 * (1l << 20))  /* 100 MB */
    warn ("%s: unlikely directory offset", pathname);
  
  /* Read the directory */
  dir = malloc (dirents * sizeof *dir);
  if (dir == NULL)
  {
    err ("%s: %s", pathname, strerror (ENOMEM));
    FAIL;
  }
  if (ftell (fp) != dirofs)
  {
    if (fseek (fp, dirofs, SEEK_SET) != 0)
    {
      warn ("%s: can't seek to directory at %lXh", pathname, dirofs);
      FAIL;
    }
  }
  {
    long n;

    for (n = 0; n < dirents; n++)
    {
      dir[n].num = n;
      offset = ftell (fp);
      if (gets32l (fp, &dir[n].ofs) != 0)
      {
	err ("%s(%lXh): read error in directory", pathname, offset);
	FAIL;
      }
      offset = ftell (fp);
      if (gets32l (fp, &dir[n].size) != 0)
      {
	err ("%s(%lXh): read error in directory", pathname, offset);
	FAIL;
      }
      offset = ftell (fp);
      if (getname (fp, dir[n].name) != 0)
      {
	err ("%s(%lXh): read error in directory", pathname, offset);
	FAIL;
      }
    }
  }

  /* Walk through the directory and write all lumps into files */
  {
    long n;
    const size_t bufsz = 0x8000;	/* 16 kB, OK for 16-bit platforms */
    char *buf = NULL;
    
    buf = malloc (bufsz);
    if (buf == NULL)
    {
      err ("%s", strerror (ENOMEM));
      FAIL;
    }

    for (n = 0; n < dirents; n++)
    {
      int lump_error = 0;		/* Error occurred extracting lump */
      long bleft;
      char *opathname = NULL;
      FILE *ofp = NULL;

      /* Position ourselves at the beginning of the lump */
      if (ftell (fp) != dir[n].ofs)
      {
	if (fseek (fp, dir[n].ofs, SEEK_SET) != 0)
	{
	  warn ("%s[%ld]: can't seek to %lXh, skipping lump %.8s",
	      pathname, n, dir[n].ofs, dir[n].name);
	  lump_error = 1;
	  goto lump_done;
	}
      }

      /* Open the output file */
      if (expand (&opathname, ofmt, dir + n) != 0)
      {
	err ("can't expand format");	/* FIXME not very explicit */
	FAIL;
      }
      if (verbose)
	puts (opathname);
      if (! dry_run)
      {
	ofp = fopen (opathname, "wb");
	if (ofp == NULL)
	{
	  err ("%s: %s", opathname, strerror (errno));
	  lump_error = 1;
	  goto lump_done;
	}
      }

      /* Dump the lump into the output file */
      for (bleft = dir[n].size; bleft > 0;)
      {
	size_t bytes = fread (buf, 1, bleft > bufsz ? bufsz : bleft, fp);
	if (bytes == 0)
	{
	  err ("%s(%lXh): unexpected EOF in lump",
	      pathname, (unsigned long) ftell (fp));
	  lump_error = 1;
	  goto lump_done;
	}
	if (! dry_run)
	{
	  if (fwrite (buf, 1, bytes, ofp) != bytes)
	  {
	    err ("%s: write error", opathname);
	    lump_error = 1;
	    goto lump_done;
	  }
	}
	bleft -= bytes;
      }

lump_done:
      if (ofp != NULL)
	if (fclose (ofp) != 0)
	{
	  err ("%s: %s", opathname, strerror (errno));
	  lump_error = 1;
	}
      if (opathname != NULL)
	free (opathname);
      if (lump_error)
      {
	err ("%s: lump %ld (%.8s) not properly extracted",
	    pathname, n, dir[n].name);
	rc = 1;
      }
    }
  }

  /* Exit point */
byebye:
  if (dir != NULL)
    free (dir);
  if (fp != NULL)
    fclose (fp);
  return rc;
}


/*
 *	gets32l - read a little-endian 32-bit signed int from a file
 *
 *	Return 0 on success, non-zero on failure.
 */
static int gets32l (FILE *fp, long *value)
{
  *value = getc (fp)
	| (getc (fp) <<  8)
        | (getc (fp) << 16)
        | (getc (fp) << 24);
  return feof (fp) || ferror (fp);
}


/*
 *	getname - read a lump name fror a file
 *
 *	Return 0 on success, non-zero on failure.
 */
static int getname (FILE *fp, char *buf)
{
  fread (buf, 8, 1, fp);
  return feof (fp) || ferror (fp);
}


/*
 *	err - printf an error message followed by a newline
 */
void err (const char *fmt, ...)
{
  va_list argp;

  fflush (stdout);
  fputs ("wadext2: ", stderr);
  va_start (argp, fmt);
  vfprintf (stderr, fmt, argp);
  va_end (argp);
  fputc ('\n', stderr);
}


/*
 *	warn - printf a warning message followed by a newline
 */
void warn (const char *fmt, ...)
{
  va_list argp;

  fflush (stdout);
  fputs ("wadext2: warning: ", stderr);
  va_start (argp, fmt);
  vfprintf (stderr, fmt, argp);
  va_end (argp);
  fputc ('\n', stderr);
}


