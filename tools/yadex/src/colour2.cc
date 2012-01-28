/*
 *	colour2.cc
 *	rgb2irgb()
 *	AYM 1998-06-28
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Rapha�l Quinet and Brendon Wyber.

The rest of Yadex is Copyright � 1997-2003 Andr� Majorel and others.

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


#include "yadex.h"


/*
 *	rgb2irgb
 *	Convert an RGB colour to an IRGB (16-colour VGA) colour.
 */
int rgb2irgb (int r, int g, int b)
{
  int c;

  c = 4*!!r + 2*!!g + 1*!!b;
  if (r > 128 || g > 128 || b > 128)
    c += 8;  // Set high intensity bit
  return c;
}

