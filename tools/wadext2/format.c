/*
 *	format.c - printf-style formatting functions
 *	AYM 2001-08-11
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "wadext2.h"
#include "format.h"


typedef struct
{
  char *buf;				/* Buffer */
  size_t len;				/* Current length, not counting \0 */
  size_t size;				/* Current size */
  int dyn;				/* Can be enlarged */
} buf_t;


typedef enum
{
  FLAG_CASE_,				/* no explicit flag given */
  FLAG_CASE_LOWER,			/* L */
  FLAG_CASE_PRESERVE,			/* = */
  FLAG_CASE_UPPER			/* U */
} flag_case_t;


typedef enum
{
  FLAG_PAD_,				/* no explicit flag given */
  FLAG_PAD_SPACE,			/* N/A */
  FLAG_PAD_UNDERSCORE,			/* _ */
  FLAG_PAD_ZERO				/* 0 */
} flag_pad_t;


typedef enum
{
  FLAG_BASE_,				/* no explicit flag given */
  FLAG_BASE_DEC,			/* D */
  FLAG_BASE_HEX,			/* H */
} flag_base_t;


typedef struct				/* Hold formatting flags */
{
  int minus;				/* - align to the left */
  int width;				/*   all: if >= 0, pad */
  int precision;			/*   strings: if >= 0, truncate */
  flag_base_t base;			/*   integers: decimal/hex */
  flag_case_t case_;			/*   all: lc/uc/preserve */
  flag_pad_t pad;			/*   all: pad with space/_/0 */
} flags_t;


static int do_expand (buf_t *buf, const char *fmt, const waddirent_t *dirent);
static int bufputld (buf_t *buf, long value, const flags_t *flags);
static int bufputlx (buf_t *buf, long value, const flags_t *flags);
static int bufputstr (buf_t *buf, const char *str, const flags_t *flags);
static int bufputcc (buf_t *buf, char c, int repeats);
static int bufputc (buf_t *buf, char c);
static void flags_init (flags_t *flags);
static int dectoi (char c);


/*
 *	expand - expand a format
 *
 *	This is a wrapper for do_expand(). It's less general but
 *	easier to use if you just want an strdup()-like
 *	behaviour.
 *
 *	Return 0 on success, non-zero on failure.
 */
int expand (char **buf, const char *fmt, const waddirent_t *dirent)
{
  buf_t b;
  int r;

  /* It's important to allocate a buffer even if it's empty, or
     we would end up with b.buf == NULL if do_expand() writes
     nothing. */
  b.buf  = malloc (10);
  b.size = 10;
  *b.buf = '\0';
  b.len  = 0;
  b.dyn  = 1;
  r = do_expand (&b, fmt, dirent);
  *buf = b.buf;
  return r;
}


/*
 *	do_expand - expand a format
 */
static int do_expand (buf_t *buf, const char *fmt, const waddirent_t *dirent)
{
  const char *f = fmt;

  for (;;)
  {
    if (*f == '\0')
      break;
    else if (*f == '%')
    {
      f++;
      if (*f == '%')
      {
	if (bufputc (buf, *f++) != 0)
	  return 1;
      }
      else
      {
	flags_t flags;

	flags_init (&flags);		/* Parse the flags */
	for (;;)
	{
	  if (*f == '-')
	  {
	    f++;
	    flags.minus = 1;
	  }
	  else if (*f == '0')
	  {
	    f++;
	    flags.pad = FLAG_PAD_ZERO;
	  }
	  else if (*f == '=')
	  {
	    f++;
	    flags.case_ = FLAG_CASE_PRESERVE;
	  }
	  else if (*f == 'D')
	  {
	    f++;
	    flags.base = FLAG_BASE_DEC;
	  }
	  else if (*f == 'H')
	  {
	    f++;
	    flags.base = FLAG_BASE_HEX;
	  }
	  else if (*f == 'L')
	  {
	    f++;
	    flags.case_ = FLAG_CASE_LOWER;
	  }
	  else if (*f == 'U')
	  {
	    f++;
	    flags.case_ = FLAG_CASE_UPPER;
	  }
	  else if (*f == '_')
	  {
	    f++;
	    flags.pad = FLAG_PAD_UNDERSCORE;
	  }
	  else
	    break;
	}

	if (dectoi (*f) > 0)		/* Parse the width */
	{
	  flags.width = 0;
	  while (dectoi (*f) > 0)
	    flags.width = 10 * flags.width + dectoi (*f++);
	}

	if (*f == '.')			/* Parse the precision */
	{
	  flags.precision = 0;
	  while (dectoi (*f) > 0)
	    flags.precision = 10 * flags.precision + dectoi (*f++);
	}

	if (*f == 'n')			/* Parse the conversion */
	{
	  f++;
	  if (flags.base == FLAG_BASE_)
	    flags.base = FLAG_BASE_DEC;
	  if (flags.case_ == FLAG_CASE_)
	    flags.case_ = FLAG_CASE_LOWER;
	  if (flags.pad == FLAG_PAD_)
	    flags.pad = FLAG_PAD_SPACE;

	  if (flags.base == FLAG_BASE_DEC)
	  {
	    if (bufputld (buf, dirent->num, &flags) != 0)
	      return 1;
	  }
	  else if (flags.base == FLAG_BASE_HEX)
	  {
	    if (bufputlx (buf, dirent->num, &flags) != 0)
	      return 1;
	  }
	}
	else if (*f == 'o')
	{
	  f++;
	  if (flags.base == FLAG_BASE_)
	    flags.base = FLAG_BASE_HEX;
	  if (flags.case_ == FLAG_CASE_)
	    flags.case_ = FLAG_CASE_LOWER;
	  if (flags.pad == FLAG_PAD_)
	    flags.pad = FLAG_PAD_SPACE;

	  if (flags.base == FLAG_BASE_DEC)
	  {
	    if (bufputld (buf, dirent->ofs, &flags) != 0)
	      return 1;
	  }
	  else if (flags.base == FLAG_BASE_HEX)
	  {
	    if (bufputlx (buf, dirent->ofs, &flags) != 0)
	      return 1;
	  }
	}
	else if (*f == 's')
	{
	  f++;
	  if (flags.base == FLAG_BASE_)
	    flags.base = FLAG_BASE_DEC;
	  if (flags.case_ == FLAG_CASE_)
	    flags.case_ = FLAG_CASE_LOWER;
	  if (flags.pad == FLAG_PAD_)
	    flags.pad = FLAG_PAD_SPACE;

	  if (flags.base == FLAG_BASE_DEC)
	  {
	    if (bufputld (buf, dirent->size, &flags) != 0)
	      return 1;
	  }
	  else if (flags.base == FLAG_BASE_HEX)
	  {
	    if (bufputlx (buf, dirent->size, &flags) != 0)
	      return 1;
	  }
	}
	else if (*f == 'l')
	{
	  f++;
	  if (flags.case_ == FLAG_CASE_)
	    flags.case_ = FLAG_CASE_LOWER;
	  if (flags.pad == FLAG_PAD_)
	    flags.pad = FLAG_PAD_SPACE;
	  if (flags.precision < 0 || flags.precision > 8)
	    flags.precision = 8;

	  if (bufputstr (buf, dirent->name, &flags) != 0)
	    return 1;
	}
	else
	{
	  err ("invalid conversion \"%c\"", *f);
	  exit (1);
	}
      }
    }
    else if (*f == '\\')
    {
      static const char escape_in[] = "abfnrtv\\";
      static const char escape_out[] = "\a\b\f\n\r\t\v\\";
      const char *e;

      f++;
      if (sizeof escape_in != sizeof escape_out)  /* Can't happen */
      {
	err ("bad escape sequence table; report this bug");
	exit (1);
      }
      e = strchr (escape_in, *f);
      if (e == NULL)
      {
	err ("invalid escape sequence \"\\%c\"", *f);
	exit (1);
      }
      f++;
      if (bufputc (buf, escape_out[e - escape_in]) != 0)
	return 1;
    }
    else
    {
      if (bufputc (buf, *f++) != 0)
	return 1;
    }
  }

  return 0;
}


/*
 *	bufputld - append a %ld conversion to a buf_t
 *
 *	A horrendous wastage of CPU cycles.
 *
 *	Return 0 on success, non-zero on failure.
 */
static int bufputld (buf_t *buf, long value, const flags_t *flags)
{
#define BASE 10
  static const char chiffres[BASE] =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
  char pad;
  int digits;
  long highest_power;

  {
    long v = value;

    for (digits = 0; v != 0; digits++)
    {
      v /= BASE;
    }
    if (digits == 0)
      digits = 1;
  }

  /* Left padding */
  if (flags->width >= 0 && flags->width > digits && ! flags->minus)
  {
    char pad;

    switch (flags->pad)
    {
      case FLAG_PAD_SPACE :
	pad = ' ';
	break;
      case FLAG_PAD_ZERO :
	pad = '0';
	break;
      case FLAG_PAD_UNDERSCORE :
	pad = '_';
	break;
      case FLAG_PAD_ :
      default :
	err ("bad flags->pad %d; report this bug", flags->pad);
	exit (1);
    }
    if (bufputcc (buf, pad, flags->width - digits) != 0)
      return 1;
  }

  for (highest_power = 1;
      BASE * highest_power > highest_power && BASE * highest_power <= value;
      highest_power *= BASE)
    ;
  for (; highest_power != 0; highest_power /= BASE)
  {
    if (bufputc (buf, chiffres[value / highest_power]) != 0)
      return 1;
    value %= highest_power;
  }

  /* Right padding. Never pad with zeros. */
  if (flags->width >= 0 && flags->width > digits && flags->minus)
  {
    char pad;

    switch (flags->pad)
    {
      case FLAG_PAD_SPACE :
      case FLAG_PAD_ZERO :
	pad = ' ';
	break;
      case FLAG_PAD_UNDERSCORE :
	pad = '_';
	break;
      case FLAG_PAD_ :
      default :
	err ("bad flags->pad %d; report this bug", flags->pad);
	exit (1);
    }
    if (bufputcc (buf, pad, flags->width - digits) != 0)
      return 1;
  }

  return 0;
}


/*
 *	bufputlx - append a %lx conversion to a buf_t
 *
 *	Return 0 on success, non-zero on failure.
 */
static int bufputlx (buf_t *buf, long value, const flags_t *flags)
{
  char pad;
  int digits;

  {
    unsigned long v = (unsigned long) value;

    for (digits = 0; v != 0; digits++)
      v >>= 4;
    if (digits == 0)
      digits = 1;
  }

  /* Left padding */
  if (flags->width >= 0 && flags->width > digits && ! flags->minus)
  {
    char pad;

    switch (flags->pad)
    {
      case FLAG_PAD_SPACE :
	pad = ' ';
	break;
      case FLAG_PAD_ZERO :
	pad = '0';
	break;
      case FLAG_PAD_UNDERSCORE :
	pad = '_';
	break;
      case FLAG_PAD_ :
      default :
	err ("bad flags->pad %d; report this bug", flags->pad);
	exit (1);
    }
    if (bufputcc (buf, pad, flags->width - digits) != 0)
      return 1;
  }

  {
    int bitnum;				/* Bit# of low bit of current nibble */
    static const char hexl[] = "0123456789abcdef";
    static const char hexu[] = "0123456789ABCDEF";
    const char       *hex    = NULL;

    switch (flags->case_)
    {
      case FLAG_CASE_LOWER :
	hex = hexl;
	break;
      case FLAG_CASE_UPPER :
	hex = hexu;
	break;
      default :
	err ("bad flags->case_ %d; report this bug", flags->case_);
	exit (1);
    }

    for (bitnum = 4 * (digits - 1); bitnum >= 0; bitnum -= 4)
      if (bufputc (buf, hex[(value >> bitnum) & 0xf]) != 0)
	return 1;
  }

  /* Right padding. Never pad with zeros. */
  if (flags->width >= 0 && flags->width > digits && flags->minus)
  {
    char pad;

    switch (flags->pad)
    {
      case FLAG_PAD_SPACE :
      case FLAG_PAD_ZERO :
	pad = ' ';
	break;
      case FLAG_PAD_UNDERSCORE :
	pad = '_';
	break;
      case FLAG_PAD_ :
      default :
	err ("bad flags->pad %d; report this bug", flags->pad);
	exit (1);
    }
    if (bufputcc (buf, pad, flags->width - digits) != 0)
      return 1;
  }

  return 0;
}


/*
 *	bufputstr - append a string conversion to a buf_t
 *
 *	Return 0 on success, non-zero on failure.
 */
static int bufputstr (buf_t *buf, const char *str, const flags_t *flags)
{
  size_t len;
  char pad;

  len = strlen (str);
  if (flags->precision >= 0 && flags->precision < len)
    len = flags->precision;

  switch (flags->pad)
  {
    case FLAG_PAD_ZERO :
    case FLAG_PAD_SPACE :
      pad = ' ';
      break;
    case FLAG_PAD_UNDERSCORE :
      pad = '_';
      break;
    case FLAG_PAD_ :
    default :
      err ("bad flags->pad %d; report this bug", flags->pad);
      exit (1);
  }

  /* Left padding */
  if (flags->width >= 0 && flags->width > len && ! flags->minus)
    if (bufputcc (buf, pad, flags->width - len) != 0)
      return 1;

  {
    int n;

    for (n = 0; n < len; n++)
    {
      char c = str[n];
      switch (flags->case_)
      {
	case FLAG_CASE_LOWER :
	  c = tolower (c);
	  break;
	case FLAG_CASE_UPPER :
	  c = toupper (c);
	  break;
	case FLAG_CASE_PRESERVE :
	  break;
	case FLAG_CASE_ :
	default :
	  err ("bad flags->case_ %d; report this bug", flags->case_);
	  exit (1);
      }
      if (bufputc (buf, c) != 0)
	return 1;
    }
  }

  /* Right padding */
  if (flags->width >= 0 && flags->width > len && flags->minus)
    if (bufputcc (buf, pad, flags->width - len) != 0)
      return 1;

  return 0;
}


/*
 *	bufputcc - append repeats of a character to a buf_t
 *
 *	Return 0 on success, non-zero on failure.
 */
static int bufputcc (buf_t *buf, char c, int repeats)
{
  while (repeats-- > 0)
    if (bufputc (buf, c) != 0)
      return 1;
  return 0;
}


/*
 *	bufputc - append a character to a buf_t
 *
 *	Return 0 on success, non-zero on failure.
 */
static int bufputc (buf_t *buf, char c)
{
  /* Attempt to grow if needed */
  if (buf->len + 1 >= buf->size)
  {
    char *newbuf;
    size_t newsize;

    if (! buf->dyn)
    {
      err ("expansion of format does not fit in buffer");
      return 1;
    }

    newsize = buf->size == 0 ? 10 : buf->size * 2;
    newbuf  = realloc (buf->buf, newsize);
    if (newbuf == NULL)
    {
      err ("%s", strerror (ENOMEM));
      return 1;
    }
    buf->buf  = newbuf;
    buf->size = newsize;
  }

  /* Append character */
  buf->buf[buf->len++] = c;
  buf->buf[buf->len]   = '\0';
  return 0;
}


/*
 *	flags_init - initialise a flags_t
 */
static void flags_init (flags_t *flags)
{
  flags->minus      = 0;
  flags->width      = -1;
  flags->precision  = -1;
  flags->base       = FLAG_BASE_;
  flags->case_      = FLAG_CASE_;
  flags->pad        = FLAG_PAD_;
}


/*
 *	dectoi - convert a character [0-9] to its numeric value
 *
 *	Locale-insensitive.
 *
 *	Return a negative number if the character is not numeric.
 */
static int dectoi (char c)
{
  switch (c)
  {
    case '0' : return 0;
    case '1' : return 1;
    case '2' : return 2;
    case '3' : return 3;
    case '4' : return 4;
    case '5' : return 5;
    case '6' : return 6;
    case '7' : return 7;
    case '8' : return 8;
    case '9' : return 9;
    default  : return -1;
  }
}


