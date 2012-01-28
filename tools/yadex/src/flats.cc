/*
 *	flats.cc
 *	AYM 1998-??-??
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
#ifdef Y_X11
#include <X11/Xlib.h>
#endif
#include "flats.h"
#include "gfx.h"
#include "img.h"
#include "levels.h"
#include "lists.h"
#include "sticker.h"
#include "wadfile.h"
#include "wads.h"


/*
   choose a floor or ceiling texture
   FIXME the <list> and <listsize> parameters are ignored since 1999-07-10.
*/

void ChooseFloorTexture (int x0, int y0, const char *prompt, int listsize, char **list, char *name)
{
  (void) list;
  (void) listsize;
  // Build a list of char *, for InputNameFromListWithFunc()'s sake
  char **flat_names = (char **) GetMemory (NumFTexture * sizeof *flat_names);
  for (size_t n = 0; n < NumFTexture; n++)
    flat_names[n] = flat_list[n].name;

  HideMousePointer ();

  SwitchToVGA256 ();
  /* If we only have a 320x200x256 VGA driver, we must change x0 and y0.
     Yuck! */
  if (GfxMode > -2)
  {
    x0 = -1;
    y0 = -1;
  }
  InputNameFromListWithFunc (x0, y0, prompt, (size_t) NumFTexture, flat_names,
    5, name, 64, 64, DisplayFloorTexture);

  SwitchToVGA16 ();

  ShowMousePointer ();
  FreeMemory (flat_names);
}


/*
 *	flat_list_entry_match
 *	Function used by bsearch() to locate a particular 
 *	flat in the FTexture.
 */
static int flat_list_entry_match (const void *key, const void *flat_list_entry)
{
  return y_strnicmp ((const char *) key,
    ((const flat_list_entry_t *) flat_list_entry)->name,
    WAD_FLAT_NAME);
}


/*
   display a floor or ceiling texture at coords c->x0, c->y0
   and not beyond c->x1, c->y1
*/

void DisplayFloorTexture (hookfunc_comm_t *c)
{
  c->width  = DOOM_FLAT_WIDTH;  // Big deal !
  c->height = DOOM_FLAT_HEIGHT;
  c->flags  = HOOK_SIZE_VALID;

  flat_list_entry_t *flat = (flat_list_entry_t *)
    bsearch (c->name, flat_list, NumFTexture, sizeof *flat_list,
	flat_list_entry_match);
  if (flat == 0)  // Not found in list
  {
    push_colour (WINBG);
    DrawScreenBox (c->x0, c->y0, c->x1, c->y1);
    set_colour (WINFG_DIM);
    DrawScreenLine (c->x0, c->y0, c->x1, c->y1);
    DrawScreenLine (c->x0, c->y1, c->x1, c->y0);
    pop_colour ();
    return;
  }
  c->lump_loc.wad = flat->wadfile;
  c->lump_loc.ofs = flat->offset;
  c->lump_loc.len = DOOM_FLAT_WIDTH * DOOM_FLAT_HEIGHT;  // Sorry.
  c->flags |= HOOK_LOC_VALID;
  const Wad_file *wadfile = flat->wadfile;
  wadfile->seek (flat->offset);
  if (wadfile->error ())
  {
    warn ("%s: can't seek to %lXh\n",
	wadfile->pathname (), (unsigned long) flat->offset);
    warn ("%.8s: seek error\n", c->name);
  }

  c->img.resize (c->width, c->height);
  c->img.set_opaque (true);
  long nbytes = (long) c->width * c->height;
  wadfile->read_bytes (c->img.wbuf (), nbytes);
  if (wadfile->error ())
  {
    warn ("%s: read error\n", wadfile->where ());
    warn ("%.8s: short read\n", c->name);
  }
  Sticker sticker (c->img, true);  // Use opaque because it's faster
  sticker.draw (drw, 't', c->x0, c->y0);

  c->disp_x0 = c->x0;
  c->disp_y0 = c->y0;
  c->disp_x1 = c->x1;
  c->disp_y1 = c->y1;

  c->flags |= HOOK_DRAWN;
}


/*
 *	display_flat_depressed
 *	Display a flat inside a hollow box
 */
void display_flat_depressed (hookfunc_comm_t *c)
{
  draw_box_border (c->x0, c->y0, c->x1 - c->x0 + 1, c->y1 - c->y0 + 1,
    HOLLOW_BORDER, 0);
  c->x0 += HOLLOW_BORDER;
  c->y0 += HOLLOW_BORDER;
  c->x1 -= HOLLOW_BORDER;
  c->y1 -= HOLLOW_BORDER;
  DisplayFloorTexture (c);
  c->x0 -= HOLLOW_BORDER;
  c->y0 -= HOLLOW_BORDER;
  c->x1 += HOLLOW_BORDER;
  c->y1 += HOLLOW_BORDER;
}



