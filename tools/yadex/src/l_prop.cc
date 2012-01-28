/*
 *	l_prop.c
 *	Linedefs properties
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
#include "gfx.h"
#include "levels.h"
#include "menudata.h"
#include "objects.h"
#include "objid.h"
#include "oldmenus.h"
#include "game.h"
#include "selectn.h"
#include "textures.h"


/*
 *	Menu_data_ldt - Menu_data class for the linedef type
 */
class Menu_data_ldt : public Menu_data
{
  public :
    Menu_data_ldt (al_llist_t *list);
    virtual size_t nitems () const;
    virtual const char *operator[] (size_t n) const;

  private :
    mutable char buf[100];
    al_llist_t *list;
};


/*
 *	Menu_data_ldt::Menu_data_ldt - ctor
 */
Menu_data_ldt::Menu_data_ldt (al_llist_t *list) : list (list)
{
  al_lrewind (this->list);
}


/*
 *	Menu_data_ldt::nitems - return the number of items
 */
size_t Menu_data_ldt::nitems () const
{
  return al_lcount (list);
}


/*
 *	Menu_data_ldt::operator[] - return the nth item
 */
const char *Menu_data_ldt::operator[] (size_t n) const
{
  if (al_lseek (list, n, SEEK_SET) != 0)
  {
    sprintf (buf, "BUG: al_lseek(%p, %lu): %s",
      (void *) list, 
      (unsigned long) n,
      al_astrerror (al_aerrno));
    return buf;
  }
  const ldtdef_t **pptr = (const ldtdef_t **) al_lptr (list);
  if (pptr == NULL)
    sprintf (buf, "BUG: al_lptr(%p): %s",
      (void *) list,
      al_astrerror (al_aerrno));
  else
    sprintf (buf, "%3d - %.70s", (*pptr)->number, (*pptr)->longdesc);
  return buf;
}


/*
 *	Prototypes of private functions
 */
static char *GetTaggedLineDefFlag (int linedefnum, int flagndx);
int InputLinedefType (int x0, int y0, int *number);
static const char *PrintLdtgroup (void *ptr);


void LinedefProperties (int x0, int y0, SelPtr obj)
{
  char  *menustr[8];
  char   texname[WAD_TEX_NAME + 1];
  int    n, val;
  SelPtr cur, sdlist;
  int objtype = OBJ_LINEDEFS;
  int    subwin_y0;
  int    subsubwin_y0;

  {
    bool sd1 = LineDefs[obj->objnum].sidedef1 >= 0;
    bool sd2 = LineDefs[obj->objnum].sidedef2 >= 0;
    val = vDisplayMenu (x0, y0, "Choose the object to edit:",
       "Edit the linedef",					YK_, 0,
       sd1 ? "Edit the 1st sidedef" : "Add a 1st sidedef",	YK_, 0,
       sd2 ? "Edit the 2nd sidedef" : "Add a 2nd sidedef",	YK_, 0,
       NULL);
  }
  subwin_y0 = y0 + BOX_BORDER + (2 + val) * FONTH;
  switch (val)
  {
    case 1:
      for (n = 0; n < 8; n++)
	menustr[n] = (char *) GetMemory (60);
      sprintf (menustr[7], "Edit linedef #%d", obj->objnum);
      sprintf (menustr[0], "Change flags            (Current: %d)",
	LineDefs[obj->objnum].flags);
      sprintf (menustr[1], "Change type             (Current: %d)",
	LineDefs[obj->objnum].type);
      sprintf (menustr[2], "Change sector tag       (Current: %d)",
	LineDefs[obj->objnum].tag);
      sprintf (menustr[3], "Change starting vertex  (Current: #%d)",
	LineDefs[obj->objnum].start);
      sprintf (menustr[4], "Change ending vertex    (Current: #%d)",
	LineDefs[obj->objnum].end);
      sprintf (menustr[5], "Change 1st sidedef ref. (Current: #%d)",
	LineDefs[obj->objnum].sidedef1);
      sprintf (menustr[6], "Change 2nd sidedef ref. (Current: #%d)",
	LineDefs[obj->objnum].sidedef2);
      val = vDisplayMenu (x0 + 42, subwin_y0, menustr[7],
	menustr[0], YK_, 0,
	menustr[1], YK_, 0,
	menustr[2], YK_, 0,
	menustr[3], YK_, 0,
	menustr[4], YK_, 0,
	menustr[5], YK_, 0,
	menustr[6], YK_, 0,
	NULL);
      for (n = 0; n < 8; n++)
	FreeMemory (menustr[n]);
      subsubwin_y0 = subwin_y0 + BOX_BORDER + (2 + val) * FONTH;
      switch (val)
      {
	case 1:
	  val = vDisplayMenu (x0 + 84, subsubwin_y0, "Toggle the flags:",
	    GetTaggedLineDefFlag (obj->objnum, 1),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 2),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 3),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 4),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 5),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 6),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 7),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 8),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 9),  YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 10), YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 11), YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 12), YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 13), YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 14), YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 15), YK_, 0,
	    GetTaggedLineDefFlag (obj->objnum, 16), YK_, 0,
	    "(Enter a decimal value)", YK_, 0,
	    NULL);
	  if (val >= 1 && val <= 16)
	     {
	     for (cur = obj; cur; cur = cur->next)
		LineDefs[cur->objnum].flags ^= 0x01 << (val - 1);
	     MadeChanges = 1;
	     }
	  else if (val == 17)
	     {
	     val = InputIntegerValue (x0 + 126, subsubwin_y0 + 12 * FONTH,
		0, 65535, LineDefs[obj->objnum].flags);
	     if (val != IIV_CANCEL)
		{
		for (cur = obj; cur; cur = cur->next)
		   LineDefs[cur->objnum].flags = val;
		MadeChanges = 1;
		}
	     }
	  break;

	case 2:
	  if (! InputLinedefType (x0, subsubwin_y0, &val))
	  {
	    for (cur = obj; cur; cur = cur->next)
	      LineDefs[cur->objnum].type = val;
	    MadeChanges = 1;
	  }
	  break;

	case 3:
	  val = InputIntegerValue (x0 + 84, subsubwin_y0,
	    -32768, 32767, LineDefs[obj->objnum].tag);
	  if (val != IIV_CANCEL)  // Not [esc]
	  {
	    for (cur = obj; cur; cur = cur->next)
	      LineDefs[cur->objnum].tag = val;
	    MadeChanges = 1;
	  }
	  break;

	case 4:
	  val = InputObjectXRef (x0 + 84, subsubwin_y0,
	    OBJ_VERTICES, 0, LineDefs[obj->objnum].start);
	  if (val >= 0)
	  {
	    for (cur = obj; cur; cur = cur->next)
	      LineDefs[cur->objnum].start = val;
	    MadeChanges = 1;
	    MadeMapChanges = 1;
	  }
	  break;

	case 5:
	  val = InputObjectXRef (x0 + 84, subsubwin_y0,
	    OBJ_VERTICES, 0, LineDefs[obj->objnum].end);
	  if (val >= 0)
	  {
	    for (cur = obj; cur; cur = cur->next)
	      LineDefs[cur->objnum].end = val;
	    MadeChanges = 1;
	    MadeMapChanges = 1;
	  }
	  break;

	case 6:
	  val = InputObjectXRef (x0 + 84, subsubwin_y0,
	    OBJ_SIDEDEFS, 1, LineDefs[obj->objnum].sidedef1);
	  if (val >= -1)
	  {
	    for (cur = obj; cur; cur = cur->next)
	      LineDefs[cur->objnum].sidedef1 = val;
	    MadeChanges = 1;
	    MadeMapChanges = 1;
	  }
	  break;

	case 7:
	  val = InputObjectXRef (x0 + 84, subsubwin_y0,
	    OBJ_SIDEDEFS, 1, LineDefs[obj->objnum].sidedef2);
	  if (val >= -1)
	  {
	    for (cur = obj; cur; cur = cur->next)
	      LineDefs[cur->objnum].sidedef2 = val;
	    MadeChanges = 1;
	    MadeMapChanges = 1;
	  }
	  break;
     }
     break;

    // Edit or add the first sidedef
    case 2:
      ObjectsNeeded (OBJ_LINEDEFS, OBJ_SIDEDEFS, 0);
      if (LineDefs[obj->objnum].sidedef1 >= 0)
      {
	// Build a new selection list with the first sidedefs
	objtype = OBJ_SIDEDEFS;
	sdlist = 0;
	for (cur = obj; cur; cur = cur->next)
	  if (LineDefs[cur->objnum].sidedef1 >= 0)
	    SelectObject (&sdlist, LineDefs[cur->objnum].sidedef1);
      }
      else
      {
	// Add a new first sidedef
	for (cur = obj; cur; cur = cur->next)
	  if (LineDefs[cur->objnum].sidedef1 == -1)
	  {
	    InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
	    LineDefs[cur->objnum].sidedef1 = NumSideDefs - 1;
	  }
	break;
      }
      // FALL THROUGH

    // Edit or add the second sidedef
    case 3:
      if (objtype != OBJ_SIDEDEFS)
      {
	if (LineDefs[obj->objnum].sidedef2 >= 0)
	{
	  // Build a new selection list with the second (or first) SideDefs
	  objtype = OBJ_SIDEDEFS;
	  sdlist = 0;
	  for (cur = obj; cur; cur = cur->next)
	    if (LineDefs[cur->objnum].sidedef2 >= 0)
	      SelectObject (&sdlist, LineDefs[cur->objnum].sidedef2);
	    else if (LineDefs[cur->objnum].sidedef1 >= 0)
	      SelectObject (&sdlist, LineDefs[cur->objnum].sidedef1);
	}
	else
	{
	  // Add a new second (or first) sidedef
	  for (cur = obj; cur; cur = cur->next)
	    if (LineDefs[cur->objnum].sidedef1 == -1)
	    {
	      InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
	      ObjectsNeeded (OBJ_LINEDEFS, 0);
	      LineDefs[cur->objnum].sidedef1 = NumSideDefs - 1;
	    }
	    else if (LineDefs[cur->objnum].sidedef2 == -1)
	    {
	      n = LineDefs[cur->objnum].sidedef1;
	      InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
	      strncpy (SideDefs[NumSideDefs - 1].tex3, "-", WAD_TEX_NAME);
	      strncpy (SideDefs[n].tex3, "-", WAD_TEX_NAME);
	      ObjectsNeeded (OBJ_LINEDEFS, 0);
	      LineDefs[cur->objnum].sidedef2 = NumSideDefs - 1;
	      LineDefs[cur->objnum].flags ^= 4;  // Set the 2S bit
	      LineDefs[cur->objnum].flags &= ~1;  // Clear the Im bit
	    }
	  break;
	}
      }
      ObjectsNeeded (OBJ_SIDEDEFS, 0);
      for (n = 0; n < 7; n++)
	menustr[n] = (char *) GetMemory (60);
      sprintf (menustr[6], "Edit sidedef #%d", sdlist->objnum);
      texname[WAD_TEX_NAME] = '\0';
      strncpy (texname, SideDefs[sdlist->objnum].tex3, WAD_TEX_NAME);
      sprintf (menustr[0], "Change middle texture   (Current: %s)", texname);
      strncpy (texname, SideDefs[sdlist->objnum].tex1, WAD_TEX_NAME);
      sprintf (menustr[1], "Change upper texture    (Current: %s)", texname);
      strncpy (texname, SideDefs[sdlist->objnum].tex2, WAD_TEX_NAME);
      sprintf (menustr[2], "Change lower texture    (Current: %s)", texname);
      sprintf (menustr[3], "Change texture X offset (Current: %d)",
	SideDefs[sdlist->objnum].xoff);
      sprintf (menustr[4], "Change texture Y offset (Current: %d)",
	SideDefs[sdlist->objnum].yoff);
      sprintf (menustr[5], "Change sector ref.      (Current: #%d)",
	SideDefs[sdlist->objnum].sector);
      val = vDisplayMenu (x0 + 42, subwin_y0, menustr[6],
	menustr[0], YK_, 0,
	menustr[1], YK_, 0,
	menustr[2], YK_, 0,
	menustr[3], YK_, 0,
	menustr[4], YK_, 0,
	menustr[5], YK_, 0,
	NULL);
      for (n = 0; n < 7; n++)
	FreeMemory (menustr[n]);
      subsubwin_y0 = subwin_y0 + BOX_BORDER + (2 + val) * FONTH;
      switch (val)
      {
	case 1:
	  strncpy (texname, SideDefs[sdlist->objnum].tex3, WAD_TEX_NAME);
	  ObjectsNeeded (0);
	  ChooseWallTexture (x0 + 84, subsubwin_y0 ,
	    "Choose a wall texture", NumWTexture, WTexture, texname);
	  ObjectsNeeded (OBJ_SIDEDEFS, 0);
	  if (strlen (texname) > 0)
	  {
	    for (cur = sdlist; cur; cur = cur->next)
	      if (cur->objnum >= 0)
		strncpy (SideDefs[cur->objnum].tex3, texname, WAD_TEX_NAME);
	    MadeChanges = 1;
	  }
	  break;

	case 2:
	  strncpy (texname, SideDefs[sdlist->objnum].tex1, WAD_TEX_NAME);
	  ObjectsNeeded (0);
	  ChooseWallTexture (x0 + 84, subsubwin_y0,
	     "Choose a wall texture", NumWTexture, WTexture, texname);
	  ObjectsNeeded (OBJ_SIDEDEFS, 0);
	  if (strlen (texname) > 0)
	  {
	    for (cur = sdlist; cur; cur = cur->next)
	      if (cur->objnum >= 0)
		strncpy (SideDefs[cur->objnum].tex1, texname, WAD_TEX_NAME);
	    MadeChanges = 1;
	  }
	  break;

	case 3:
	  strncpy (texname, SideDefs[sdlist->objnum].tex2, WAD_TEX_NAME);
	  ObjectsNeeded (0);
	  ChooseWallTexture (x0 + 84, subsubwin_y0,
	    "Choose a wall texture", NumWTexture, WTexture, texname);
	  ObjectsNeeded (OBJ_SIDEDEFS, 0);
	  if (strlen (texname) > 0)
	  {
	    for (cur = sdlist; cur; cur = cur->next)
	      if (cur->objnum >= 0)
		strncpy (SideDefs[cur->objnum].tex2, texname, WAD_TEX_NAME);
	    MadeChanges = 1;
	  }
	  break;

	case 4:
	  val = InputIntegerValue (x0 + 84, subsubwin_y0,
	    -32768, 32767, SideDefs[sdlist->objnum].xoff);
	  if (val != IIV_CANCEL)
	  {
	    for (cur = sdlist; cur; cur = cur->next)
	      if (cur->objnum >= 0)
		SideDefs[cur->objnum].xoff = val;
	    MadeChanges = 1;
	  }
	  break;

	case 5:
	  val = InputIntegerValue (x0 + 84, subsubwin_y0,
	    -32768, 32767, SideDefs[sdlist->objnum].yoff);
	  if (val != IIV_CANCEL)
	  {
	    for (cur = sdlist; cur; cur = cur->next)
	      if (cur->objnum >= 0)
		SideDefs[cur->objnum].yoff = val;
	    MadeChanges = 1;
	  }
	  break;

	case 6:
	  val = InputObjectXRef (x0 + 84, subsubwin_y0,
	    OBJ_SECTORS, 0, SideDefs[sdlist->objnum].sector);
	  if (val >= 0)
	  {
	    for (cur = sdlist; cur; cur = cur->next)
	      if (cur->objnum >= 0)
		SideDefs[cur->objnum].sector = val;
	    MadeChanges = 1;
	  }
	  break;
      }
      ForgetSelection (&sdlist);
      break;
  }
}


/*
*/

static char *GetTaggedLineDefFlag (int linedefnum, int flagndx)
{
  static char ldstr[16][50];

  if ((LineDefs[linedefnum].flags & (0x01 << (flagndx - 1))) != 0)
    strcpy (ldstr[flagndx - 1], "* ");
  else
    strcpy (ldstr[flagndx - 1], "  ");
  strcat (ldstr[flagndx - 1], GetLineDefFlagsLongName (0x01 << (flagndx - 1)));
  return ldstr[flagndx - 1];
}



/*
 *	InputLinedefType
 *	Let the user select a linedef type number and return it.
 *	Returns 0 if OK, <>0 if cancelled
 */
int InputLinedefType (int x0, int y0, int *number)
{
  int         r;
  int         ldtgno = 0;
  char        ldtg; 
  al_llist_t *list = 0;

  for (;;)
  {
    /* First let user select a ldtgroup */
    if (DisplayMenuList (x0+84, y0, "Select group", ldtgroup,
     PrintLdtgroup, &ldtgno) < 0)
      return 1;
    if (al_lseek (ldtgroup, ldtgno, SEEK_SET))
      fatal_error ("%s ILT1 (%s)", msg_unexpected, al_astrerror (al_aerrno));
    ldtg = CUR_LDTGROUP->ldtgroup;

    /* KLUDGE: Special ldtgroup LDT_FREE means "enter number"
       Don't look for this ldtgroup in the .ygd file :
       LoadGameDefs() creates it manually. */
    if (ldtg == LDT_FREE)
    {
      // FIXME should be unsigned
      *number = InputIntegerValue (x0+126, y0 + (3 + ldtgno) * FONTH,
	 -32768, 32767, 0);
      if (*number != IIV_CANCEL)
	break;
      goto again;
    }
      
    /* Then build a list of pointers on all ldt that have this
       ldtgroup and let user select one */
    list = al_lcreate (sizeof (void *));
    for (al_lrewind (ldtdef); ! al_leol (ldtdef); al_lstep (ldtdef))
      if (CUR_LDTDEF->ldtgroup == ldtg)
      {
	void *ptr = CUR_LDTDEF;
	al_lwrite (list, &ptr);
      }
    {
      Menu_data_ldt menudata (list);
      r = DisplayMenuList
       (x0+126, y0 + 2 * FONTH, "Select type", menudata, NULL);
    }
    if (r < 0)
      goto again;
    if (al_lseek (list, r, SEEK_SET))
      fatal_error ("%s ILT2 (%s)", msg_unexpected, al_astrerror (al_aerrno));
    *number = (*((ldtdef_t **) al_lptr (list)))->number;
    al_ldiscard (list);
    break;

    again :
    ;
    /* draw_map (OBJ_THINGS, 0, 0);  FIXME! */
  }

  return 0;
}


/*
 *	PrintLdtgroup
 *	Used by DisplayMenuList when called by InputLinedefType
 */
static const char *PrintLdtgroup (void *ptr)
{
  if (! ptr)
    return "PrintLdtgroup: (null)";
  return ((ldtgroup_t *)ptr)->desc;
}

/*
 *   TransferLinedefProperties
 *
 *   Note: right now nothing is done about sidedefs.  Being able to
 *   (intelligently) transfer sidedef properties from source line to
 *   destination linedefs could be a useful feature -- though it is
 *   unclear the best way to do it.  OTOH not touching sidedefs might
 *   be useful too.
 *
 *   -AJA- 2001-05-27
 */
#define LINEDEF_FLAG_KEEP  (1 + 4)

void TransferLinedefProperties (int src_linedef, SelPtr linedefs)
{
   SelPtr cur;
   wad_ldflags_t src_flags = LineDefs[src_linedef].flags & ~LINEDEF_FLAG_KEEP;

   for (cur=linedefs; cur; cur=cur->next)
   {
      if (! is_obj(cur->objnum))
         continue;

      // don't transfer certain flags
      LineDefs[cur->objnum].flags &= LINEDEF_FLAG_KEEP;
      LineDefs[cur->objnum].flags |= src_flags;

      LineDefs[cur->objnum].type = LineDefs[src_linedef].type;
      LineDefs[cur->objnum].tag  = LineDefs[src_linedef].tag;

      MadeChanges = 1;
   }
}

