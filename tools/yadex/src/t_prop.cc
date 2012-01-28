/*
 *	t_prop.c
 *	Thing properties
 *	Some of this was originally in editobj.c. It was moved here to
 *	improve overlay granularity (therefore memory consumption).
 *	AYM 1998-02-07
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel and others.

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
#include "entry.h"
#include "game.h"
#include "gfx.h"
#include "levels.h"
#include "oldmenus.h"
#include "objid.h"
#include "objects.h"
#include "selectn.h"
#include "things.h"

/*
 *	Private functions prototypes
 */
static const char *PrintThinggroup (void *ptr);
static const char *PrintThingdef (void *ptr);
int InputThingType (int x0, int y0, int *number);


/*
 *	ThingProperties
 *	Thing properties dialog. Called by EditObjectsInfo. Was part of
 *	EditObjectsInfo in editobj.c
 */
void ThingProperties (int x0, int y0, SelPtr obj)
{
char  *menustr[30];
int    n, val;
SelPtr cur;
int    subwin_y0;

for (n = 0; n < 6; n++)
   menustr[n] = (char *) GetMemory (60);
sprintf (menustr[5], "Edit thing #%d", obj->objnum);
sprintf (menustr[0], "Change type          (Current: %s)",
         get_thing_name (Things[obj->objnum].type));
sprintf (menustr[1], "Change angle         (Current: %s)",
         GetAngleName (Things[obj->objnum].angle));
sprintf (menustr[2], "Change flags         (Current: %s)",
         GetWhenName (Things[obj->objnum].when));
sprintf (menustr[3], "Change X position    (Current: %d)",
         Things[obj->objnum].xpos);
sprintf (menustr[4], "Change Y position    (Current: %d)",
         Things[obj->objnum].ypos);
val = vDisplayMenu (x0, y0, menustr[5],
   menustr[0], YK_, 0,
   menustr[1], YK_, 0,
   menustr[2], YK_, 0,
   menustr[3], YK_, 0,
   menustr[4], YK_, 0,
   NULL);
for (n = 0; n < 6; n++)
   FreeMemory (menustr[n]);
subwin_y0 = y0 + BOX_BORDER + (2 + val) * FONTH;
switch (val)
  {
  case 1:
     if (! InputThingType (x0, subwin_y0, &val))
	{
	for (cur = obj; cur; cur = cur->next)
	   Things[cur->objnum].type = val;
	things_types++;
	MadeChanges = 1;
	}
     break;

  case 2:
     switch (vDisplayMenu (x0 + 42, subwin_y0, "Select angle",
			  "North",	YK_, 0,
			  "NorthEast",	YK_, 0,
			  "East",	YK_, 0,
			  "SouthEast",	YK_, 0,
			  "South",	YK_, 0,
			  "SouthWest",	YK_, 0,
			  "West",	YK_, 0,
			  "NorthWest",	YK_, 0,
			  NULL))
	{
	case 1:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 90;
	   things_angles++;
	   MadeChanges = 1;
	   break;

	case 2:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 45;
	   things_angles++;
	   MadeChanges = 1;
	   break;

	case 3:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 0;
	   things_angles++;
	   MadeChanges = 1;
	   break;

	case 4:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 315;
	   things_angles++;
	   MadeChanges = 1;
	   break;

	case 5:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 270;
	   things_angles++;
	   MadeChanges = 1;
	   break;

	case 6:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 225;
	   things_angles++;
	   MadeChanges = 1;
	   break;

	case 7:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 180;
	   things_angles++;
	   MadeChanges = 1;
	   break;

	case 8:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].angle = 135;
	   things_angles++;
	   MadeChanges = 1;
	   break;
	}
     break;

  case 3:
     val = vDisplayMenu (x0 + 42, subwin_y0, "Choose the difficulty level(s)",
			"D12          (Easy only)",		YK_, 0,
			"D3           (Medium only)",		YK_, 0,
			"D12, D3      (Easy and Medium)",	YK_, 0,
			"D45          (Hard only)",		YK_, 0,
			"D12, D45     (Easy and Hard)",		YK_, 0,
			"D3, D45      (Medium and Hard)",	YK_, 0,
			"D12, D3, D45 (Easy, Medium, Hard)",	YK_, 0,
			"Toggle \"Deaf/Ambush\" bit",		YK_, 0,
			"Toggle \"Multi-player only\" bit",	YK_, 0,
			"(Enter number)",			YK_, 0,
			NULL);
     switch (val)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].when = (Things[cur->objnum].when & 0x18) | val;
	   MadeChanges = 1;
	   break;

	case 8:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].when ^= 0x08;
	   MadeChanges = 1;
	   break;

	case 9:
	   for (cur = obj; cur; cur = cur->next)
	      Things[cur->objnum].when ^= 0x10;
	   MadeChanges = 1;
	   break;

	case 10:
	   val = InputIntegerValue (x0 + 84,
              subwin_y0 + BOX_BORDER + (3 + val) * FONTH, 0, 65535,
	      Things[obj->objnum].when);
	   if (val != IIV_CANCEL)
	      {
	      for (cur = obj; cur; cur = cur->next)
		 Things[cur->objnum].when = val;
	      MadeChanges = 1;
	      }
	   break;
	}
     break;

  case 4:
     val = InputIntegerValue (x0 + 42, subwin_y0, MapMinX, MapMaxX,
                              Things[obj->objnum].xpos);
     if (val != IIV_CANCEL)
        {
	n = val - Things[obj->objnum].xpos;
	for (cur = obj; cur; cur = cur->next)
	   Things[cur->objnum].xpos += n;
	MadeChanges = 1;
        }
     break;

  case 5:
     val = InputIntegerValue (x0 + 42, subwin_y0, MapMinY, MapMaxY,
                              Things[obj->objnum].ypos);
     if (val != IIV_CANCEL)
        {
	n = val - Things[obj->objnum].ypos;
	for (cur = obj; cur; cur = cur->next)
	   Things[cur->objnum].ypos += n;
	MadeChanges = 1;
        }
     break;
  }
}


/*
 *	InputThingType
 *	Let the user select a thing number and return it.
 *	Returns 0 if OK, <>0 if cancelled
 */
int InputThingType (int x0, int y0, int *number)
{
int         r;
int         tgno = 0;
char        tg; 
al_llist_t *list = NULL;

for (;;)
   {
   /* First let user select a thinggroup */
   if (DisplayMenuList (x0+42, y0, "Select group", thinggroup,
    PrintThinggroup, &tgno) < 0)
      return 1;
   if (al_lseek (thinggroup, tgno, SEEK_SET))
      fatal_error ("%s ITT1 (%s)", msg_unexpected, al_astrerror (al_aerrno));
   tg = CUR_THINGGROUP->thinggroup;

   /* KLUDGE: Special thinggroup THING_FREE means "enter number".
      Don't look for this thinggroup in the .ygd file : LoadGameDefs()
      creates it manually. */
   if (tg == THING_FREE)
      {
      /* FIXME should be unsigned! should accept hex. */
      *number = InputIntegerValue (x0+84, y0 + BOX_BORDER + (3 + tgno) * FONTH,
	 -32768, 32767, 0);
      if (*number != IIV_CANCEL)
	 break;
      goto again;
      }
     
   /* Then build a list of pointers on all things that have this
      thinggroup and let user select one. */
   list = al_lcreate (sizeof (void *));
   for (al_lrewind (thingdef); ! al_leol (thingdef); al_lstep (thingdef))
      if (CUR_THINGDEF->thinggroup == tg)
	 {
	 void *ptr = CUR_THINGDEF;
	 al_lwrite (list, &ptr);
	 }
   r = DisplayMenuList (x0+84, y0 + BOX_BORDER + (3 + tgno) * FONTH,
      "Select thing", list, PrintThingdef, NULL);
   if (r < 0)
      goto again;
   if (al_lseek (list, r, SEEK_SET))
      fatal_error ("%s ITT2 (%s)", msg_unexpected, al_astrerror (al_aerrno));
   *number = (*((thingdef_t **) al_lptr (list)))->number;
   al_ldiscard (list);
   break;

   again :
   ;
   /* DrawMap (OBJ_THINGS, 0, 0); FIXME! */
   }
return 0;
}


/*
 *	PrintThinggroup
 *	Used by DisplayMenuList when called by InputThingType
 */
static const char *PrintThinggroup (void *ptr)
{
if (ptr == NULL)
   return "PrintThinggroup: (null)";
return ((thinggroup_t *)ptr)->desc;
}


/*
 *	PrintThingdef
 *	Used by DisplayMenuList when called by InputThingType
 */
static const char *PrintThingdef (void *ptr)
{
if (ptr == NULL)
   return "PrintThingdef: (null)";
return (*((thingdef_t **)ptr))->desc;
}


/*
 *   TransferThingProperties
 *
 *   -AJA- 2001-05-27
 */
void TransferThingProperties (int src_thing, SelPtr things)
{
   SelPtr cur;

   for (cur=things; cur; cur=cur->next)
   {
      if (! is_obj(cur->objnum))
         continue;

      Things[cur->objnum].angle = Things[src_thing].angle;
      Things[cur->objnum].type  = Things[src_thing].type;
      Things[cur->objnum].when  = Things[src_thing].when;

      MadeChanges = 1;

      things_types++;
      things_angles++;
   }
}


/* end of file */
