/*
 *	solc.c
 *	al_strOLC
 */


/*
This file is part of Atclib.

Atclib is Copyright � 1995-1999 Andr� Majorel.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307, USA.
*/


#include "atclib.h"


int al_strOLC (const char *str, char chr)
{
register int offset;

for (offset = 0; *str; offset++)
  str++;
while (offset >= 0)
  {
  if (*str == chr)
    break;
  str--;
  offset--;
  }
return offset;
}

