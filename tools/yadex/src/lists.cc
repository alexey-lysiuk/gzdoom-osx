/*
 *	lists.cc
 *	Pick an item from a list.
 *	AYM 1998-08-22
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
#include "gfx.h"
#include "lists.h"
#include "wadfile.h"


// FIXME Move this in a more public place
void lump_loc_string (char *buf, size_t buf_size, const Lump_loc& lump_loc)
{
  if (buf_size < 1)
    return;
  int len = buf_size - 1 - (1 + 8 + 1 + 1);  // %08lXh
  if (len < 1)
  {
    *buf = '\0';
    return;
  }
  y_filename (buf, len + 1, lump_loc.wad->pathname ());
  sprintf (buf + strlen (buf), "(%08lXh)",
    (unsigned long) lump_loc.ofs & 0xffffffff);
}


/*
   ask for a name in a given list and call a function (for displaying objects,
   etc.)

   Arguments:
      x0, y0  : where to draw the box.
      prompt  : text to be displayed.
      listsize: number of elements in the list.
      list    : list of names (picture names, level names, etc.).
      listdisp: minimum number of names that should be displayed.
      name    : what we are editing...
      width   : \ width and height of an optional window where a picture
      height  : / can be displayed (used to display textures, sprites, etc.).
      hookfunc: function that should be called to display a picture.
		(x1, y1, x2, y2 = coordinates of the window in which the
		 picture must be drawn, name = name of the picture).
AYM 1998-02-12 : if hookfunc is <> NULL, a message "Press shift-F1 to save
  image to file" is displayed and shift-F1 does just that.
*/

#ifdef DEBUG
static bool disp_lump_loc = false;
#endif

void InputNameFromListWithFunc (
   int x0,
   int y0,
   const char *prompt,
   size_t listsize,
   const char *const *list,
   size_t listdisp,
   char *name,
   int width,
   int height,
   void (*hookfunc)(hookfunc_comm_t *),
   char flags_to_pass_to_callback)
{
const char *msg1 = "Press Shift-F1 to";
const char *msg2 = "save image to file";
int    key;
size_t n;
size_t win_height;
int    win_columns;
int    win_width;
int    l0;
int    x1, y1, x2, y2;
size_t maxlen;
int    xlist;
bool   picture_size_drawn = false;
#ifdef DEBUG
bool   lump_loc_drawn = false;
#endif
bool   ok, firstkey;
int    entry_out_x0;	/* Edge of name entry widget including border */
int    entry_out_y0;
int    entry_out_x1;
int    entry_out_y1;
int    entry_text_x0;	/* Edge of text area of name entry widget */
int    entry_text_y0;
int    entry_text_x1;
int    entry_text_y1;

// Sanity
if (width < 0)
{
  nf_bug ("inflwf1");
  width = 0;
}
if (height < 0)
{
  nf_bug ("inflwf2");
  height = 0;
}

// Compute maxlen, the length of the longest item in the list
maxlen = 1;
for (n = 0; n < listsize; n++)
   if (strlen (list[n]) > maxlen)
      maxlen = strlen (list[n]);
for (n = strlen (name) + 1; n <= maxlen; n++)
   name[n] = '\0';
char *namedisp = new char[maxlen + 1];
memset (namedisp, '\xff', maxlen + 1);  // Always != from name

// Compute the minimum width of the dialog box
l0 = 12;
if (hookfunc != NULL)
   {
   if ((int) (strlen (msg1) + 2) > l0)  // (int) to prevent GCC warning
      l0 = strlen (msg1) + 2;
   if ((int) (strlen (msg2) + 2) > l0)  // (int) to prevent GCC warning
      l0 = strlen (msg2) + 2;
   }
xlist = 10 + l0 * FONTW;
win_columns = l0 + maxlen;
if ((int) (strlen (prompt)) > win_columns)  // (int) to prevent GCC warning
   win_columns = strlen (prompt);
win_width = 10 + FONTW * win_columns;
x1 = win_width + 8;
y1 = 10 + 1;
if (width > 0)
   win_width += 16 + width;
// (int) to prevent GCC warning
win_height = y_max (height + 20, (int) (listdisp * FONTH + 10 + 28));
if (x0 < 0)
   x0 = (ScrMaxX - win_width) / 2;
if (y0 < 0)
   y0 = (ScrMaxY - win_height) / 2;
x1 += x0;
y1 += y0;
if (x1 + width - 1 < ScrMaxX)
   x2 = x1 + width - 1;
else
   x2 = ScrMaxX;
if (y1 + height - 1 < ScrMaxY)
   y2 = y1 + height - 1;
else
   y2 = ScrMaxY;

entry_out_x0  = x0 + 10;
entry_text_x0 = entry_out_x0 + HOLLOW_BORDER + NARROW_HSPACING;
entry_text_x1 = entry_text_x0 + 10 * FONTW - 1;
entry_out_x1  = entry_text_x1 + HOLLOW_BORDER + NARROW_HSPACING;
entry_out_y0  = y0 + 28;
entry_text_y0 = entry_out_y0 + HOLLOW_BORDER + NARROW_VSPACING;
entry_text_y1 = entry_text_y0 + FONTH - 1;
entry_out_y1  = entry_text_y1 + HOLLOW_BORDER + NARROW_VSPACING;

listdisp = y_max (listdisp,
  (win_height - (entry_out_y0 - y0) - BOX_BORDER - WIDE_VSPACING) / FONTH);

// Draw the dialog box
DrawScreenBox3D (x0, y0, x0 + win_width, y0 + win_height);
DrawScreenBoxHollow (entry_out_x0, entry_out_y0, entry_out_x1, entry_out_y1,
		     BLACK);
set_colour (WINTITLE);
DrawScreenText (x0 + 10, y0 + 8, prompt);
set_colour (WINFG);
if (hookfunc != NULL)
   {
   DrawScreenText (x0 + 10,
		   y0 + win_height - BOX_BORDER - WIDE_VSPACING - 2 * FONTH,
		   msg1);
   DrawScreenText (x0 + 10,
		   y0 + win_height - BOX_BORDER - WIDE_VSPACING - FONTH,
		   msg2);
   }
if (width > 0)
   DrawScreenBoxHollow (x1 - 1, y1 - 1, x2 + 1, y2 + 1, BLACK);
firstkey = true;

// Another way of saying "nothing to rub out"
int disp_x0 = (x2 + x1) / 2;
int disp_y0 = (y2 + y1) / 2;
int disp_x1 = disp_x0 - 1;
int disp_y1 = disp_y0 - 1;

int maxpatches = 0;

// The event loop
for (;;)
   {
   hookfunc_comm_t c;

   // Reset maxpatches every time when change texture
   if (strcmp (name, namedisp) != 0)
     maxpatches = 0;

   // Is "name" in the list ?
   for (n = 0; n < listsize; n++)
      if (y_stricmp (name, list[n]) <= 0)
	 break;
   ok = n < listsize ? ! y_stricmp (name, list[n]) : false;
   if (n >= listsize)
      n = listsize - 1;

   // Display the <listdisp> next items in the list
   {
   size_t l;				// Current line
   int y = entry_out_y0;		// Y-coord of current line
   int xmin = x0 + xlist;
   int xmax = xmin + FONTW * maxlen - 1;
   for (l = 0; l < listdisp && n + l < listsize; l++)
      {
      if (false && has_input_event ())	// TEST
	 {
	 putchar ('.');		// TEST
	 fflush (stdout);	// TEST
	 goto shortcut;		// TEST
	 }
      set_colour (WINBG);
      DrawScreenBox (xmin, y, xmax, y + FONTH - 1);
      set_colour (WINFG);
      DrawScreenText (xmin, y, list[n+l]);
      y += FONTH;
      }
   if (l < listdisp)  // Less than <listdisp> names to display
      {
      set_colour (WINBG);
      DrawScreenBox (xmin, y, xmax, entry_out_y0 + listdisp * FONTH - 1);
      }
   }

   // Display the entry box and the current text
   set_colour (BLACK);
   DrawScreenBox (entry_text_x0, entry_text_y0, entry_text_x1, entry_text_y1);
   if (ok)  // FIXME this colour scheme should be changed.
      set_colour (WHITE);
   else
      set_colour (WINFG);
   DrawScreenText (entry_text_x0, entry_text_y0, name);

   // Call the function to display the picture, if any
   if (hookfunc)
      {
      // Display the picture name
      c.x0         = x1;
      c.y0         = y1;
      c.x1         = x2;
      c.y1         = y2;
      c.name       = name;
      c.xofs       = 0;
      c.yofs       = 0;
      c.flags      = flags_to_pass_to_callback;
      const int BAD_VALUE = INT_MAX;
      c.disp_x0    = BAD_VALUE;  // Catch faulty callbacks
      c.disp_y0    = BAD_VALUE;  // Catch faulty callbacks
      c.disp_x1    = BAD_VALUE;  // Catch faulty callbacks
      c.disp_y1    = BAD_VALUE;  // Catch faulty callbacks
      c.maxpatches = maxpatches;
      if (ok)
	 {
	 hookfunc (&c);
	 }
      else
	 {
	 // No picture. Null width & height. Erase everything.
	 c.disp_x0 = (x2 + x1) / 2;
	 c.disp_y0 = (y2 + y1) / 2;
	 c.disp_x1 = c.disp_x0 - 1;
	 c.disp_y1 = c.disp_y0 - 1;
	 }
      strcpy (namedisp, name);

      // Display the (unclipped) size of the picture
      {
      const size_t size_chars = 11;
      const int    size_x0    = x0 + 10;
      const int    size_y0    = y0 + 50;
      if (picture_size_drawn)
	 {
	 set_colour (WINBG);
	 DrawScreenBoxwh (size_x0, size_y0, size_chars * FONTW, FONTH);
	 picture_size_drawn = false;
	 }
      if ((c.flags & HOOK_SIZE_VALID) && (c.flags & HOOK_DISP_SIZE))
	 {
	 set_colour (WINFG);
	 char size_buf[100];  // Slack
	 y_snprintf (size_buf, sizeof size_buf, "%dx%d", c.width, c.height);
	 if (strlen (size_buf) > size_chars)
	   strcpy (size_buf + size_chars - 1, ">");
	 DrawScreenString (size_x0, size_y0, size_buf);
	 picture_size_drawn = true;
	 }
      }

#ifdef DEBUG
      // Display the file name and file offset of the picture
      {
      const size_t loc_chars = win_width / FONTW;
      const int    loc_x0    = x0;
      const int    loc_y0    = y0 + win_height;
      if (lump_loc_drawn)
	 {
	 set_colour (WINBG);
	 DrawScreenBoxwh (loc_x0, loc_y0, loc_chars * FONTW, FONTH);
	 lump_loc_drawn = false;
	 }
      if (disp_lump_loc && (c.flags & HOOK_LOC_VALID))
	 {
	 set_colour (WINFG);
	 char buf[150];  // Slack
	 lump_loc_string (buf, sizeof buf, c.lump_loc);
	 DrawScreenString (loc_x0, loc_y0, buf);
	 lump_loc_drawn = true;
	 }
      }
#endif

      /* If the new picture does not completely obscure the
	 previous one, rub out the old pixels. */
      set_colour (BLACK);
      if (c.disp_x0 == BAD_VALUE
	|| c.disp_y0 == BAD_VALUE
	|| c.disp_x1 == BAD_VALUE
	|| c.disp_y1 == BAD_VALUE)
	 nf_bug ("Callback %p did not set disp_", hookfunc);
      else
	 {
	 /* +-WINDOW------------------------+   Erase the dots...
            |                               |
	    |  +-OLD IMAGE---------------+  |   (this is for the case where
	    |  | . . : . . . . . . : . . |  |   the image is centred but the
	    |  |. . .:. . . 3 . . .:. . .|  |   principle is the same if it's
	    |  | . . : . . . . . . : . . |  |   E.G. in the top left corner)
	    |  |. . .+-NEW IMAGE---+. . .|  |
	    |  | . . |             | . . |  |
	    |  |. 1 .|             |. 2 .|  |
	    |  | . . |             | . . |  |
	    |  |. . .+-------------+. . .|  |
	    |  | . . : . . . . . . : . . |  |
	    |  |. . .:. . . 4 . . .:. . .|  |
	    |  | . . : . . . . . . : . . |  |
	    |  +-------------------------+  |
	    |                               |
	    +-------------------------------+ */
	 if (c.disp_x0 > disp_x0)
	    DrawScreenBox (disp_x0, disp_y0, c.disp_x0 - 1, disp_y1);  // (1)
	 if (c.disp_x1 < disp_x1)
	    DrawScreenBox (c.disp_x1 + 1, disp_y0, disp_x1, disp_y1);  // (2)
	 if (c.disp_y0 > disp_y0)
	    DrawScreenBox (y_max (c.disp_x0, disp_x0), disp_y0,
			   y_min (c.disp_x1, disp_x1), c.disp_y0 - 1); // (3)
	 if (c.disp_y1 < disp_y1)
	    DrawScreenBox (y_max (c.disp_x0, disp_x0), c.disp_y1 + 1,
			   y_min (c.disp_x1, disp_x1), disp_y1);       // (4)
	 }
      disp_x0 = c.disp_x0;
      disp_y0 = c.disp_y0;
      disp_x1 = c.disp_x1;
      disp_y1 = c.disp_y1;
      }

   // Process user input
shortcut:
   key = get_key ();
   if (firstkey && is_ordinary (key) && key != ' ')
      {
      for (size_t i = 0; i <= maxlen; i++)
	 name[i] = '\0';
      }
   firstkey = false;
   size_t len = strlen (name);
   if (len < maxlen && key >= 'a' && key <= 'z')
      {
      name[len] = key + 'A' - 'a';
      name[len + 1] = '\0';
      }
   else if (len < maxlen && is_ordinary (key) && key != ' ')
      {
      name[len] = key;
      name[len + 1] = '\0';
      }
   else if (len > 0 && key == YK_BACKSPACE)		// BS
      name[len - 1] = '\0';
   else if (key == 21 || key == 23)			// ^U, ^W
      *name = '\0';
   else if (key == YK_DOWN)				// [Down]
      {
      /* Look for the next item in the list that has a
	 different name. Why not just use the next item ?
	 Because sometimes the list has duplicates (for example
	 when editing a Doom II pwad in Doom mode) and then the
	 viewer gets "stuck" on the first duplicate. */
      size_t m = n + 1;
      while (m < listsize && ! y_stricmp (list[n], list[m]))
	 m++;
      if (m < listsize)
         strcpy (name, list[m]);
      else
	 Beep ();
      }
   else if (key == YK_UP)				// [Up]
      {
      // Same trick as for [Down]
      int m = n - 1;
      while (m >= 0 && ! y_stricmp (list[n], list[m]))
	 m--;
      if (m >= 0)
         strcpy (name, list[m]);
      else
         Beep ();
      }
   else if (key == YK_PD || key == 6 || key == 22)	// [Pgdn], ^F, ^V
      { 
      if (n < listsize - listdisp)
         strcpy (name, list[y_min (n + listdisp, listsize - 1)]);
      else
	 Beep ();
      }
   else if ((key == YK_PU || key == 2) && n > 0)	// [Pgup], ^B
      {
      if (n > listdisp)
	 strcpy (name, list[n - listdisp]);
      else
	 strcpy (name, list[0]);
      }
   else if (key == 14)					// ^N
      {
      if (n + 1 >= listsize)
	 {
	 Beep ();
         goto done_with_event;
	 }
      while (n + 1 < listsize)
	 {
	 n++;
	 if (y_strnicmp (list[n - 1], list[n], 4))
	    break;
	 }
      strcpy (name, list[n]);
      }
   else if (key == 16)					// ^P
      {
      if (n < 1)
	 {
	 Beep ();
	 goto done_with_event;
	 }
      // Put in <n> the index of the first entry of the current
      // group or, if already at the beginning of the current
      // group, the first entry of the previous group.
      if (n > 0)
	 {
	 if (y_strnicmp (list[n], list[n - 1], 4))
	    n--;
	 while (n > 0 && ! y_strnicmp (list[n], list[n - 1], 4))
	    n--;
	 }
      strcpy (name, list[n]);
      }
   else if (key == (YK_CTRL | YK_PD) || key == YK_END)	// [Ctrl][Pgdn], [End]
      {
      if (n + 1 >= listsize)
	 {
	 Beep ();
	 goto done_with_event;
	 }
      strcpy (name, list[listsize - 1]);
      }
   else if (key == (YK_CTRL | YK_PU) || key == YK_HOME)	// [Ctrl][Pgup], [Home]
      {
      if (n < 1)
	 {
	 Beep ();
	 goto done_with_event;
	 }
      strcpy (name, list[0]);
      }
   else if (key == YK_TAB)				// [Tab]
      strcpy (name, list[n]);
   else if (key == YK_F1 && c.flags & HOOK_LOC_VALID)	// [F1]: print location
      {
      printf ("%.8s: %s(%08lXh)\n",
	 name, c.lump_loc.wad->pathname (), (unsigned long) c.lump_loc.ofs);
      }
   else if (key == YK_F1 + YK_SHIFT	// [Shift][F1] : dump image to file
    && hookfunc != NULL
    && (c.flags & HOOK_DRAWN))
      {
      const size_t size = strlen (name) + 4 + 1;
      char *filename = new char[size];
      al_scpslower (filename, name,   size - 1);
      al_saps      (filename, ".ppm", size - 1);
      if (c.img.save (filename) != 0)
	 {
	 if (errno == ECHILD)
	    err ("Error loading PLAYPAL");
	 else
	    err ("%s: %s", filename, strerror (errno));
	 }
      else
	 {
	 printf ("Saved %s as %s\n", name, filename);
	 }
      delete[] filename;
      }
   else if (key == 1)					// ^A: more patches
      {
      if (maxpatches + 1 < c.npatches)
	maxpatches++;
      else
	maxpatches = 0;
      printf ("maxpatches %d\n", maxpatches);
      }
   else if (key == 24)					// ^X: less patches
      {
      if (maxpatches == 0)
	maxpatches = c.npatches - 1;
      else
	maxpatches--;
      printf ("maxpatches %d\n", maxpatches);
      }
   else if (ok && key == YK_RETURN)			// [Return]
      break; /* return "name" */
   else if (key == YK_ESC)				// [Esc]
      {
      name[0] = '\0'; /* return an empty string */
      break;
      }
   else
      Beep ();
done_with_event:
   ;
   }
delete[] namedisp;
}


/*
   ask for a name in a given list
*/

void InputNameFromList (
   int x0,
   int y0,
   const char *prompt,
   size_t listsize,
   const char *const *list,
   char *name)
{
HideMousePointer ();
InputNameFromListWithFunc (x0, y0, prompt, listsize, list, 5, name, 0, 0, NULL);
ShowMousePointer ();
}




