/*
 *	objects.cc
 *	Object handling routines.
 *	BW & RQ sometime in 1993 or 1994.
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
#include "drawmap.h"
#include "gfx.h"
#include "l_vertices.h"
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "s_vertices.h"
#include "selectn.h"
#include "things.h"


/*
   highlight the selected objects
*/
void HighlightSelection (int objtype, SelPtr list) /* SWAP! */
{
SelPtr cur;

if (! list)
   return;
for (cur = list; cur; cur = cur->next)
   HighlightObject (objtype, cur->objnum, GREEN);
}



/*
   get the number of objects of a given type minus one
*/
obj_no_t GetMaxObjectNum (int objtype)
{
switch (objtype)
   {
   case OBJ_THINGS:
      return NumThings - 1;
   case OBJ_LINEDEFS:
      return NumLineDefs - 1;
   case OBJ_SIDEDEFS:
      return NumSideDefs - 1;
   case OBJ_VERTICES:
      return NumVertices - 1;
   case OBJ_SECTORS:
      return NumSectors - 1;
   }
return -1;
}


/*
   highlight the selected object
*/
void HighlightObject (int objtype, int objnum, int colour) /* SWAP! */
{
int  n, m;

/* use XOR mode : drawing any line twice erases it */
SetDrawingMode (1);
set_colour (colour);
switch (objtype)
   {
   case OBJ_THINGS:
      ObjectsNeeded (OBJ_THINGS, 0);
      m = (get_thing_radius (Things[objnum].type) * 3) / 2;
      DrawMapLine (Things[objnum].xpos - m, Things[objnum].ypos - m,
		   Things[objnum].xpos - m, Things[objnum].ypos + m);
      DrawMapLine (Things[objnum].xpos - m, Things[objnum].ypos + m,
		   Things[objnum].xpos + m, Things[objnum].ypos + m);
      DrawMapLine (Things[objnum].xpos + m, Things[objnum].ypos + m,
		   Things[objnum].xpos + m, Things[objnum].ypos - m);
      DrawMapLine (Things[objnum].xpos + m, Things[objnum].ypos - m,
		   Things[objnum].xpos - m, Things[objnum].ypos - m);
      DrawMapArrow (Things[objnum].xpos, Things[objnum].ypos,
		    Things[objnum].angle * 182);
      break;

   case OBJ_LINEDEFS:
      ObjectsNeeded (OBJ_LINEDEFS, OBJ_VERTICES, 0);
      n = (Vertices[LineDefs[objnum].start].x
	 + Vertices[LineDefs[objnum].end].x) / 2;
      m = (Vertices[LineDefs[objnum].start].y
	 + Vertices[LineDefs[objnum].end].y) / 2;
      DrawMapLine (n, m, n + (Vertices[LineDefs[objnum].end].y
			    - Vertices[LineDefs[objnum].start].y) / 3,
			 m + (Vertices[LineDefs[objnum].start].x
			    - Vertices[LineDefs[objnum].end].x) / 3);
      SetLineThickness (1);
      DrawMapVector (Vertices[LineDefs[objnum].start].x,
		     Vertices[LineDefs[objnum].start].y,
		     Vertices[LineDefs[objnum].end].x,
		     Vertices[LineDefs[objnum].end].y);
      if (colour != LIGHTRED && LineDefs[objnum].tag > 0)
	 {
	 for (m = 0; m < NumSectors; m++)
	    if (Sectors[m].tag == LineDefs[objnum].tag)
	       HighlightObject (OBJ_SECTORS, m, LIGHTRED);
	 }
      SetLineThickness (0);
      break;

   case OBJ_VERTICES:
      ObjectsNeeded (OBJ_VERTICES, 0);
      {
      int r = vertex_radius (Scale) * 3 / 2;
      int scrx0 = SCREENX (Vertices[objnum].x) - r;
      int scrx9 = SCREENX (Vertices[objnum].x) + r;
      int scry0 = SCREENY (Vertices[objnum].y) - r;
      int scry9 = SCREENY (Vertices[objnum].y) + r;
      DrawScreenLine (scrx0, scry0, scrx9, scry0);
      DrawScreenLine (scrx9, scry0, scrx9, scry9);
      DrawScreenLine (scrx9, scry9, scrx0, scry9);
      DrawScreenLine (scrx0, scry9, scrx0, scry0);
      }
      break;

   case OBJ_SECTORS:
      {
      ObjectsNeeded (OBJ_LINEDEFS, OBJ_SIDEDEFS, OBJ_VERTICES, 0);
      SetLineThickness (1);
      const int mapx0 = MAPX (0);
      const int mapy0 = MAPY (ScrMaxY);
      const int mapx1 = MAPX (ScrMaxX);
      const int mapy1 = MAPY (0);
      for (n = 0; n < NumLineDefs; n++)
	 if (LineDefs[n].sidedef1 != -1
	     && SideDefs[LineDefs[n].sidedef1].sector == objnum
	  || LineDefs[n].sidedef2 != -1
	     && SideDefs[LineDefs[n].sidedef2].sector == objnum)
	 {
	    const struct Vertex *v1 = Vertices + LineDefs[n].start;
	    const struct Vertex *v2 = Vertices + LineDefs[n].end;
	    if (v1->x < mapx0 && v2->x < mapx0
	     || v1->x > mapx1 && v2->x > mapx1
	     || v1->y < mapy0 && v2->y < mapy0
	     || v1->y > mapy1 && v2->y > mapy1)
	       continue;  // Off-screen
	    DrawMapLine (v1->x, v1->y, v2->x, v2->y);
	 }
      if (colour != LIGHTRED && Sectors[objnum].tag > 0)
	 {
	 for (m = 0; m < NumLineDefs; m++)
	    if (LineDefs[m].tag == Sectors[objnum].tag)
	       HighlightObject (OBJ_LINEDEFS, m, LIGHTRED);
	 }
      SetLineThickness (0);
      }
      break;
   }
/* restore normal write mode */
SetDrawingMode (0);
}



/*
   delete an object
*/
void DeleteObject (const Objid& obj) /* SWAP! */
{
SelPtr list;

list = 0;
SelectObject (&list, obj.num);
DeleteObjects (obj.type, &list);
}



/*
   delete a group of objects (*recursive*)
*/
void DeleteObjects (int objtype, SelPtr *list) /* SWAP! */
{
int    n, objnum;
SelPtr cur;

MadeChanges = 1;
switch (objtype)
   {
   case OBJ_THINGS:
      ObjectsNeeded (OBJ_THINGS, 0);
      if (*list)
	 {
	 things_angles++;
	 things_types++;
	 }
      while (*list)
	 {
	 objnum = (*list)->objnum;
	 if (objnum < 0 || objnum >= NumThings)  // Paranoia
	 {
	    nf_bug ("attempt to delete non-existent thing #%d", objnum);
	    goto next_thing;
	 }
	 // Delete the thing
	 NumThings--;
	 if (NumThings > 0)
	    {
	    for (n = objnum; n < NumThings; n++)
	       Things[n] = Things[n + 1];
	    Things = (TPtr) ResizeFarMemory (Things,
	       NumThings * sizeof (struct Thing));
	    }
	 else
	    {
	    FreeFarMemory (Things);
	    Things = 0;
	    }
	 for (cur = (*list)->next; cur; cur = cur->next)
	    if (cur->objnum > objnum)
	       cur->objnum--;
	 next_thing:
	 UnSelectObject (list, objnum);
	 }
      break;

   case OBJ_VERTICES:
      if (*list)
         MadeMapChanges = 1;
      while (*list)
	 {
	 objnum = (*list)->objnum;
	 if (objnum < 0 || objnum >= NumVertices)  // Paranoia
	 {
	    nf_bug ("attempt to delete non-existent vertex #%d", objnum);
	    goto next_vertex;
	 }
	 // Delete the linedefs bound to this vertex and change the references
	 ObjectsNeeded (OBJ_LINEDEFS, 0);
	 for (n = 0; n < NumLineDefs; n++)
	    {
	    if (LineDefs[n].start == objnum || LineDefs[n].end == objnum)
	       DeleteObject (Objid (OBJ_LINEDEFS, n--));
	    else
	       {
	       if (LineDefs[n].start >= objnum)
		  LineDefs[n].start--;
	       if (LineDefs[n].end >= objnum)
		  LineDefs[n].end--;
	       }
	    }
	 // Delete the vertex
	 ObjectsNeeded (OBJ_VERTICES, 0);
	 NumVertices--;
	 if (NumVertices > 0)
	    {
	    for (n = objnum; n < NumVertices; n++)
	       Vertices[n] = Vertices[n + 1];
	    Vertices = (VPtr) ResizeFarMemory (Vertices,
	      NumVertices * sizeof (struct Vertex));
	    }
	 else
	    {
	    FreeFarMemory (Vertices);
	    Vertices = 0;
	    }
	 for (cur = (*list)->next; cur; cur = cur->next)
	    if (cur->objnum > objnum)
	       cur->objnum--;
	 next_vertex:
	 UnSelectObject (list, objnum);
	 }
      break;

   case OBJ_LINEDEFS:
      /* In DEU, deleting a linedef was not considered to be a
	 map change. Deleting a _sidedef_ was. In Yadex,
	 sidedefs are not automatically deleted when the linedef
	 is because some sidedefs are shared by more than one
	 linedef. So we need to set MadeMapChanges here. */
      if (*list)
         MadeMapChanges = 1;
      /* AYM 19980203 I've removed the deletion of sidedefs
         because if several linedefs use the same sidedef, this
         would lead to trouble. Instead, I let the xref checking
         take care of that. */
      while (*list)
	 {
	 ObjectsNeeded (OBJ_LINEDEFS, 0);
	 objnum = (*list)->objnum;
	 if (objnum < 0 || objnum >= NumLineDefs)  // Paranoia
	 {
	    nf_bug ("attempt to delete non-existent linedef #%d", objnum);
	    goto next_linedef;
	 }
	 // delete the linedef
	 NumLineDefs--;
	 if (NumLineDefs > 0)
	    {
	    for (n = objnum; n < NumLineDefs; n++)
	       LineDefs[n] = LineDefs[n + 1];
	    LineDefs = (LDPtr) ResizeFarMemory (LineDefs,
	      NumLineDefs * sizeof (struct LineDef));
	    }
	 else
	    {
	    FreeFarMemory (LineDefs);
	    LineDefs = 0;
	    }
	 for (cur = (*list)->next; cur; cur = cur->next)
	    if (cur->objnum > objnum)
	       cur->objnum--;
	 next_linedef:
	 UnSelectObject (list, objnum);
	 }
      break;

   case OBJ_SIDEDEFS:
      if (*list)
         MadeMapChanges = 1;
      while (*list)
	 {
	 objnum = (*list)->objnum;
	 if (objnum < 0 || objnum >= NumSideDefs)  // Paranoia
	 {
	    nf_bug ("attempt to delete non-existent sidedef #%d", objnum);
	    goto next_sidedef;
	 }
	 /* change the linedefs references */
	 ObjectsNeeded (OBJ_LINEDEFS, 0);
	 for (n = 0; n < NumLineDefs; n++)
	    {
	    if (LineDefs[n].sidedef1 == objnum)
	       LineDefs[n].sidedef1 = -1;
	    else if (LineDefs[n].sidedef1 >= objnum)
	       LineDefs[n].sidedef1--;
	    if (LineDefs[n].sidedef2 == objnum)
	       LineDefs[n].sidedef2 = -1;
	    else if (LineDefs[n].sidedef2 >= objnum)
	       LineDefs[n].sidedef2--;
	    }
	 /* delete the sidedef */
	 ObjectsNeeded (OBJ_SIDEDEFS, 0);
	 NumSideDefs--;
	 if (NumSideDefs > 0)
	    {
	    for (n = objnum; n < NumSideDefs; n++)
	       SideDefs[n] = SideDefs[n + 1];
	    SideDefs = (SDPtr) ResizeFarMemory (SideDefs,
	       NumSideDefs * sizeof (struct SideDef));
	    }
	 else
	    {
	    FreeFarMemory (SideDefs);
	    SideDefs = 0;
	    }
	 for (cur = (*list)->next; cur; cur = cur->next)
	    if (cur->objnum > objnum)
	       cur->objnum--;
	 next_sidedef:
	 UnSelectObject (list, objnum);
	 }
      break;
   case OBJ_SECTORS:
      while (*list)
	{
	objnum = (*list)->objnum;
	 if (objnum < 0 || objnum >= NumSectors)  // Paranoia
	 {
	    nf_bug ("attempt to delete non-existent sector #%d", objnum);
	    goto next_sector;
	 }
	// Delete the sidedefs bound to this sector and change the references
	// AYM 19980203: Hmm, hope this is OK with multiply used sidedefs...
	ObjectsNeeded (OBJ_SIDEDEFS, 0);
	for (n = 0; n < NumSideDefs; n++)
	   if (SideDefs[n].sector == objnum)
	      DeleteObject (Objid (OBJ_SIDEDEFS, n--));
	   else if (SideDefs[n].sector >= objnum)
	      SideDefs[n].sector--;
	/* delete the sector */
	ObjectsNeeded (OBJ_SECTORS, 0);
	NumSectors--;
	if (NumSectors > 0)
	   {
	   for (n = objnum; n < NumSectors; n++)
	      Sectors[n] = Sectors[n + 1];
	   Sectors = (SPtr) ResizeFarMemory (Sectors,
	      NumSectors * sizeof (struct Sector));
	   }
	else
	   {
	   FreeFarMemory (Sectors);
	   Sectors = 0;
	   }
	for (cur = (*list)->next; cur; cur = cur->next)
	   if (cur->objnum > objnum)
	      cur->objnum--;
	next_sector:
	UnSelectObject (list, objnum);
	}
      break;
   default:
      nf_bug ("DeleteObjects: bad objtype %d", (int) objtype);
   }
}



/*
 *	InsertObject
 *	Insert a new object of type <objtype> at map coordinates
 *	(<xpos>, <ypos>).
 *
 *	If <copyfrom> is a valid object number, the other properties
 *	of the new object are set from the properties of that object,
 *	with the exception of sidedef numbers, which are forced
 *	to OBJ_NO_NONE.
 *
 *	The object is inserted at the exact coordinates given.
 *	No snapping to grid is done.
 */
void InsertObject (obj_type_t objtype, obj_no_t copyfrom, int xpos, int ypos)
								/* SWAP! */
{
int last;

ObjectsNeeded (objtype, 0);
MadeChanges = 1;
switch (objtype)
   {
   case OBJ_THINGS:
      last = NumThings++;
      if (last > 0)
	 Things = (TPtr) ResizeFarMemory (Things,
	   (unsigned long) NumThings * sizeof (struct Thing));
      else
	 Things = (TPtr) GetFarMemory (sizeof (struct Thing));
      Things[last].xpos = xpos;
      Things[last].ypos = ypos;
      things_angles++;
      things_types++;
      if (is_obj (copyfrom))
	 {
	 Things[last].type  = Things[copyfrom].type;
	 Things[last].angle = Things[copyfrom].angle;
	 Things[last].when  = Things[copyfrom].when;
	 }
      else
	 {
	 Things[last].type = default_thing;
	 Things[last].angle = 0;
	 Things[last].when  = 0x07;
	 }
      break;

   case OBJ_VERTICES:
      last = NumVertices++;
      if (last > 0)
	 Vertices = (VPtr) ResizeFarMemory (Vertices,
	   (unsigned long) NumVertices * sizeof (struct Vertex));
      else
	 Vertices = (VPtr) GetFarMemory (sizeof (struct Vertex));
      Vertices[last].x = xpos;
      Vertices[last].y = ypos;
      if (Vertices[last].x < MapMinX)
	 MapMinX = Vertices[last].x;
      if (Vertices[last].x > MapMaxX)
	 MapMaxX = Vertices[last].x;
      if (Vertices[last].y < MapMinY)
	 MapMinY = Vertices[last].y;
      if (Vertices[last].y > MapMaxY)
	 MapMaxY = Vertices[last].y;
      MadeMapChanges = 1;
      break;

   case OBJ_LINEDEFS:
      last = NumLineDefs++;
      if (last > 0)
	 LineDefs = (LDPtr) ResizeFarMemory (LineDefs,
	   (unsigned long) NumLineDefs * sizeof (struct LineDef));
      else
	 LineDefs = (LDPtr) GetFarMemory (sizeof (struct LineDef));
      if (is_obj (copyfrom))
	 {
	 LineDefs[last].start = LineDefs[copyfrom].start;
	 LineDefs[last].end   = LineDefs[copyfrom].end;
	 LineDefs[last].flags = LineDefs[copyfrom].flags;
	 LineDefs[last].type  = LineDefs[copyfrom].type;
	 LineDefs[last].tag   = LineDefs[copyfrom].tag;
	 }
      else
	 {
	 LineDefs[last].start = 0;
	 LineDefs[last].end   = NumVertices - 1;
	 LineDefs[last].flags = 1;
	 LineDefs[last].type  = 0;
	 LineDefs[last].tag   = 0;
	 }
      LineDefs[last].sidedef1 = OBJ_NO_NONE;
      LineDefs[last].sidedef2 = OBJ_NO_NONE;
      break;

   case OBJ_SIDEDEFS:
      last = NumSideDefs++;
      if (last > 0)
	 SideDefs = (SDPtr) ResizeFarMemory (SideDefs,
	   (unsigned long) NumSideDefs * sizeof (struct SideDef));
      else
	 SideDefs = (SDPtr) GetFarMemory (sizeof (struct SideDef));
      if (is_obj (copyfrom))
	 {
	 SideDefs[last].xoff = SideDefs[copyfrom].xoff;
	 SideDefs[last].yoff = SideDefs[copyfrom].yoff;
	 strncpy (SideDefs[last].tex1, SideDefs[copyfrom].tex1, WAD_TEX_NAME);
	 strncpy (SideDefs[last].tex2, SideDefs[copyfrom].tex2, WAD_TEX_NAME);
	 strncpy (SideDefs[last].tex3, SideDefs[copyfrom].tex3, WAD_TEX_NAME);
	 SideDefs[last].sector = SideDefs[copyfrom].sector;
	 }
      else
	 {
	 SideDefs[last].xoff = 0;
	 SideDefs[last].yoff = 0;
	 strcpy (SideDefs[last].tex1, "-");
	 strcpy (SideDefs[last].tex2, "-");
	 strcpy (SideDefs[last].tex3, default_middle_texture);
	 SideDefs[last].sector = NumSectors - 1;
	 }
      MadeMapChanges = 1;
      break;

   case OBJ_SECTORS:
      last = NumSectors++;
      if (last > 0)
	 Sectors = (SPtr) ResizeFarMemory (Sectors,
			  (unsigned long) NumSectors * sizeof (struct Sector));
      else
	 Sectors = (SPtr) GetFarMemory (sizeof (struct Sector));
      if (is_obj (copyfrom))
	 {
	 Sectors[last].floorh  = Sectors[copyfrom].floorh;
	 Sectors[last].ceilh   = Sectors[copyfrom].ceilh;
	 strncpy (Sectors[last].floort, Sectors[copyfrom].floort, WAD_FLAT_NAME);
	 strncpy (Sectors[last].ceilt, Sectors[copyfrom].ceilt, WAD_FLAT_NAME);
	 Sectors[last].light   = Sectors[copyfrom].light;
	 Sectors[last].special = Sectors[copyfrom].special;
	 Sectors[last].tag     = Sectors[copyfrom].tag;
	 }
      else
	 {
	 Sectors[last].floorh  = default_floor_height;
	 Sectors[last].ceilh   = default_ceiling_height;
	 strncpy (Sectors[last].floort, default_floor_texture, WAD_FLAT_NAME);
	 strncpy (Sectors[last].ceilt, default_ceiling_texture, WAD_FLAT_NAME);
	 Sectors[last].light   = default_light_level;
	 Sectors[last].special = 0;
	 Sectors[last].tag     = 0;
	 }
      break;

   default:
      nf_bug ("InsertObject: bad objtype %d", (int) objtype);
   }
}



/*
   check if a (part of a) LineDef is inside a given block
*/
bool IsLineDefInside (int ldnum, int x0, int y0, int x1, int y1) /* SWAP - needs Vertices & LineDefs */
{
int lx0 = Vertices[LineDefs[ldnum].start].x;
int ly0 = Vertices[LineDefs[ldnum].start].y;
int lx1 = Vertices[LineDefs[ldnum].end].x;
int ly1 = Vertices[LineDefs[ldnum].end].y;
int i;

/* do you like mathematics? */
if (lx0 >= x0 && lx0 <= x1 && ly0 >= y0 && ly0 <= y1)
   return 1; /* the linedef start is entirely inside the square */
if (lx1 >= x0 && lx1 <= x1 && ly1 >= y0 && ly1 <= y1)
   return 1; /* the linedef end is entirely inside the square */
if ((ly0 > y0) != (ly1 > y0))
   {
   i = lx0 + (int) ((long) (y0 - ly0) * (long) (lx1 - lx0) / (long) (ly1 - ly0));
   if (i >= x0 && i <= x1)
      return true; /* the linedef crosses the y0 side (left) */
   }
if ((ly0 > y1) != (ly1 > y1))
   {
   i = lx0 + (int) ((long) (y1 - ly0) * (long) (lx1 - lx0) / (long) (ly1 - ly0));
   if (i >= x0 && i <= x1)
      return true; /* the linedef crosses the y1 side (right) */
   }
if ((lx0 > x0) != (lx1 > x0))
   {
   i = ly0 + (int) ((long) (x0 - lx0) * (long) (ly1 - ly0) / (long) (lx1 - lx0));
   if (i >= y0 && i <= y1)
      return true; /* the linedef crosses the x0 side (down) */
   }
if ((lx0 > x1) != (lx1 > x1))
   {
   i = ly0 + (int) ((long) (x1 - lx0) * (long) (ly1 - ly0) / (long) (lx1 - lx0));
   if (i >= y0 && i <= y1)
      return true; /* the linedef crosses the x1 side (up) */
   }
return false;
}



/*
   get the sector number of the sidedef opposite to this sidedef
   (returns -1 if it cannot be found)
*/
int GetOppositeSector (int ld1, bool firstside) /* SWAP! */
{
int x0, y0, dx0, dy0;
int x1, y1, dx1, dy1;
int x2, y2, dx2, dy2;
int ld2, dist;
int bestld, bestdist, bestmdist;

/* get the coords for this LineDef */
ObjectsNeeded (OBJ_LINEDEFS, OBJ_VERTICES, 0);
x0  = Vertices[LineDefs[ld1].start].x;
y0  = Vertices[LineDefs[ld1].start].y;
dx0 = Vertices[LineDefs[ld1].end].x - x0;
dy0 = Vertices[LineDefs[ld1].end].y - y0;

/* find the normal vector for this LineDef */
x1  = (dx0 + x0 + x0) / 2;
y1  = (dy0 + y0 + y0) / 2;
if (firstside)
   {
   dx1 = dy0;
   dy1 = -dx0;
   }
else
   {
   dx1 = -dy0;
   dy1 = dx0;
   }

bestld = -1;
/* use a parallel to an axis instead of the normal vector (faster method) */
if (abs (dy1) > abs (dx1))
   {
   if (dy1 > 0)
      {
      /* get the nearest LineDef in that direction (increasing Y's: North) */
      bestdist = 32767;
      bestmdist = 32767;
      for (ld2 = 0; ld2 < NumLineDefs; ld2++)
	 if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].x > x1)
			 != (Vertices[LineDefs[ld2].end].x > x1)))
	    {
	    x2  = Vertices[LineDefs[ld2].start].x;
	    y2  = Vertices[LineDefs[ld2].start].y;
	    dx2 = Vertices[LineDefs[ld2].end].x - x2;
	    dy2 = Vertices[LineDefs[ld2].end].y - y2;
	    dist = y2 + (int) ((long) (x1 - x2) * (long) dy2 / (long) dx2);
	    if (dist > y1 && (dist < bestdist
	     || (dist == bestdist && (y2 + dy2 / 2) < bestmdist)))
	       {
	       bestld = ld2;
	       bestdist = dist;
	       bestmdist = y2 + dy2 / 2;
	       }
	    }
      }
   else
      {
      /* get the nearest LineDef in that direction (decreasing Y's: South) */
      bestdist = -32767;
      bestmdist = -32767;
      for (ld2 = 0; ld2 < NumLineDefs; ld2++)
	 if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].x > x1)
			 != (Vertices[LineDefs[ld2].end].x > x1)))
	    {
	    x2  = Vertices[LineDefs[ld2].start].x;
	    y2  = Vertices[LineDefs[ld2].start].y;
	    dx2 = Vertices[LineDefs[ld2].end].x - x2;
	    dy2 = Vertices[LineDefs[ld2].end].y - y2;
	    dist = y2 + (int) ((long) (x1 - x2) * (long) dy2 / (long) dx2);
	    if (dist < y1 && (dist > bestdist
	     || (dist == bestdist && (y2 + dy2 / 2) > bestmdist)))
	       {
	       bestld = ld2;
	       bestdist = dist;
	       bestmdist = y2 + dy2 / 2;
	       }
	    }
      }
   }
else
   {
   if (dx1 > 0)
      {
      /* get the nearest LineDef in that direction (increasing X's: East) */
      bestdist = 32767;
      bestmdist = 32767;
      for (ld2 = 0; ld2 < NumLineDefs; ld2++)
	 if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].y > y1)
			 != (Vertices[LineDefs[ld2].end].y > y1)))
	    {
	    x2  = Vertices[LineDefs[ld2].start].x;
	    y2  = Vertices[LineDefs[ld2].start].y;
	    dx2 = Vertices[LineDefs[ld2].end].x - x2;
	    dy2 = Vertices[LineDefs[ld2].end].y - y2;
	    dist = x2 + (int) ((long) (y1 - y2) * (long) dx2 / (long) dy2);
	    if (dist > x1 && (dist < bestdist
	     || (dist == bestdist && (x2 + dx2 / 2) < bestmdist)))
	       {
	       bestld = ld2;
	       bestdist = dist;
	       bestmdist = x2 + dx2 / 2;
	       }
	    }
      }
   else
      {
      /* get the nearest LineDef in that direction (decreasing X's: West) */
      bestdist = -32767;
      bestmdist = -32767;
      for (ld2 = 0; ld2 < NumLineDefs; ld2++)
	 if (ld2 != ld1 && ((Vertices[LineDefs[ld2].start].y > y1)
			 != (Vertices[LineDefs[ld2].end].y > y1)))
	    {
	    x2  = Vertices[LineDefs[ld2].start].x;
	    y2  = Vertices[LineDefs[ld2].start].y;
	    dx2 = Vertices[LineDefs[ld2].end].x - x2;
	    dy2 = Vertices[LineDefs[ld2].end].y - y2;
	    dist = x2 + (int) ((long) (y1 - y2) * (long) dx2 / (long) dy2);
	    if (dist < x1 && (dist > bestdist
	     || (dist == bestdist && (x2 + dx2 / 2) > bestmdist)))
	       {
	       bestld = ld2;
	       bestdist = dist;
	       bestmdist = x2 + dx2 / 2;
	       }
	    }
      }
   }

/* no intersection: the LineDef was pointing outwards! */
if (bestld < 0)
   return -1;

/* now look if this LineDef has a SideDef bound to one sector */
if (abs (dy1) > abs (dx1))
   {
   if ((Vertices[LineDefs[bestld].start].x
	< Vertices[LineDefs[bestld].end].x) == (dy1 > 0))
      x0 = LineDefs[bestld].sidedef1;
   else
      x0 = LineDefs[bestld].sidedef2;
   }
else
   {
   if ((Vertices[LineDefs[bestld].start].y
      < Vertices[LineDefs[bestld].end].y) != (dx1 > 0))
      x0 = LineDefs[bestld].sidedef1;
   else
      x0 = LineDefs[bestld].sidedef2;
   }

/* there is no SideDef on this side of the LineDef! */
if (x0 < 0)
   return -1;

/* OK, we got it -- return the Sector number */
ObjectsNeeded (OBJ_SIDEDEFS, 0);
return SideDefs[x0].sector;
}



/*
   copy a group of objects to a new position
*/
void CopyObjects (int objtype, SelPtr obj) /* SWAP! */
{
int        n, m;
SelPtr     cur;
SelPtr     list1, list2;
SelPtr     ref1, ref2;

if (! obj)
   return;
ObjectsNeeded (objtype, 0);
/* copy the object(s) */
switch (objtype)
   {
   case OBJ_THINGS:
      for (cur = obj; cur; cur = cur->next)
	 {
	 InsertObject (OBJ_THINGS, cur->objnum, Things[cur->objnum].xpos,
						Things[cur->objnum].ypos);
	 cur->objnum = NumThings - 1;
	 }
      MadeChanges = 1;
      break;

   case OBJ_VERTICES:
      for (cur = obj; cur; cur = cur->next)
	 {
	 InsertObject (OBJ_VERTICES, cur->objnum, Vertices[cur->objnum].x,
						  Vertices[cur->objnum].y);
	 cur->objnum = NumVertices - 1;
	 }
      MadeChanges = 1;
      MadeMapChanges = 1;
      break;

   case OBJ_LINEDEFS:
      list1 = 0;
      list2 = 0;

      // Create the linedefs and maybe the sidedefs
      for (cur = obj; cur; cur = cur->next)
	 {
         int old = cur->objnum;	// No. of original linedef
         int New;		// No. of duplicate linedef

	 InsertObject (OBJ_LINEDEFS, old, 0, 0);
         New = NumLineDefs - 1;

         if (copy_linedef_reuse_sidedefs)
	    {
	    /* AYM 1997-07-25: not very orthodox (the New linedef and 
	       the old one use the same sidedefs). but, in the case where
	       you're copying into the same sector, it's much better than
	       having to create the New sidedefs manually. plus it saves
	       space in the .wad and also it makes editing easier (editing
	       one sidedef impacts all linedefs that use it). */
	    LineDefs[New].sidedef1 = LineDefs[old].sidedef1; 
	    LineDefs[New].sidedef2 = LineDefs[old].sidedef2; 
	    }
         else
            {
            /* AYM 1998-11-08: duplicate sidedefs too.
               DEU 5.21 just left the sidedef references to -1. */
            if (is_sidedef (LineDefs[old].sidedef1))
	       {
	       InsertObject (OBJ_SIDEDEFS, LineDefs[old].sidedef1, 0, 0);
	       LineDefs[New].sidedef1 = NumSideDefs - 1;
	       }
            if (is_sidedef (LineDefs[old].sidedef2))
	       {
	       InsertObject (OBJ_SIDEDEFS, LineDefs[old].sidedef2, 0, 0);
	       LineDefs[New].sidedef2 = NumSideDefs - 1; 
	       }
            }
	 cur->objnum = New;
	 if (!IsSelected (list1, LineDefs[New].start))
	    {
	    SelectObject (&list1, LineDefs[New].start);
	    SelectObject (&list2, LineDefs[New].start);
	    }
	 if (!IsSelected (list1, LineDefs[New].end))
	    {
	    SelectObject (&list1, LineDefs[New].end);
	    SelectObject (&list2, LineDefs[New].end);
	    }
	 }

      // Create the vertices
      CopyObjects (OBJ_VERTICES, list2);
      ObjectsNeeded (OBJ_LINEDEFS, 0);

      // Update the references to the vertices
      for (ref1 = list1, ref2 = list2;
	   ref1 && ref2;
	   ref1 = ref1->next, ref2 = ref2->next)
	 {
	 for (cur = obj; cur; cur = cur->next)
	    {
	    if (ref1->objnum == LineDefs[cur->objnum].start)
	      LineDefs[cur->objnum].start = ref2->objnum;
	    if (ref1->objnum == LineDefs[cur->objnum].end)
	      LineDefs[cur->objnum].end = ref2->objnum;
	    }
	 }
      ForgetSelection (&list1);
      ForgetSelection (&list2);
      break;

   case OBJ_SECTORS:
      ObjectsNeeded (OBJ_LINEDEFS, OBJ_SIDEDEFS, 0);
      list1 = 0;
      list2 = 0;
      // Create the linedefs (and vertices)
      for (cur = obj; cur; cur = cur->next)
	 {
	 for (n = 0; n < NumLineDefs; n++)
	    if  ((((m = LineDefs[n].sidedef1) >= 0
		       && SideDefs[m].sector == cur->objnum)
		|| ((m = LineDefs[n].sidedef2) >= 0
		       && SideDefs[m].sector == cur->objnum))
		       && ! IsSelected (list1, n))
	       {
	       SelectObject (&list1, n);
	       SelectObject (&list2, n);
	       }
	 }
      CopyObjects (OBJ_LINEDEFS, list2);
      /* create the sidedefs */
      ObjectsNeeded (OBJ_LINEDEFS, 0);
      for (ref1 = list1, ref2 = list2;
	   ref1 && ref2;
	   ref1 = ref1->next, ref2 = ref2->next)
	 {
	 if ((n = LineDefs[ref1->objnum].sidedef1) >= 0)
	    {
	    InsertObject (OBJ_SIDEDEFS, n, 0, 0);
	    n = NumSideDefs - 1;
	    ObjectsNeeded (OBJ_LINEDEFS, 0);
	    LineDefs[ref2->objnum].sidedef1 = n;
	    }
	 if ((m = LineDefs[ref1->objnum].sidedef2) >= 0)
	    {
	    InsertObject (OBJ_SIDEDEFS, m, 0, 0);
	    m = NumSideDefs - 1;
	    ObjectsNeeded (OBJ_LINEDEFS, 0);
	    LineDefs[ref2->objnum].sidedef2 = m;
	    }
	 ref1->objnum = n;
	 ref2->objnum = m;
	 }
      /* create the Sectors */
      for (cur = obj; cur; cur = cur->next)
	 {
	 InsertObject (OBJ_SECTORS, cur->objnum, 0, 0);
	 ObjectsNeeded (OBJ_SIDEDEFS, 0);
	 for (ref1 = list1, ref2 = list2;
	      ref1 && ref2;
	      ref1 = ref1->next, ref2 = ref2->next)
	    {
	    if (ref1->objnum >= 0
               && SideDefs[ref1->objnum].sector == cur->objnum)
	       SideDefs[ref1->objnum].sector = NumSectors - 1;
	    if (ref2->objnum >= 0
               && SideDefs[ref2->objnum].sector == cur->objnum)
	       SideDefs[ref2->objnum].sector = NumSectors - 1;
	    }
	 cur->objnum = NumSectors - 1;
	 }
      ForgetSelection (&list1);
      ForgetSelection (&list2);
      break;
   }
}



/*
 *	MoveObjectsToCoords
 *	Move a group of objects to a new position
 *
 *	You must first call it with obj == NULL and newx and newy
 *	set to the coordinates of the reference point (E.G. the
 *	object being dragged).
 *	Then, every time the object being dragged has changed its
 *	coordinates, call the it again with newx and newy set to
 *	the new position and obj set to the selection.
 *
 *	Returns <>0 iff an object was moved.
 */
bool MoveObjectsToCoords (
   int objtype,
   SelPtr obj,
   int newx,
   int newy,
   int grid) /* SWAP! */
{
int        dx, dy;
SelPtr     cur, vertices;
static int refx, refy; /* previous position */

ObjectsNeeded (objtype, 0);
if (grid > 0)
   {
   newx = (newx + grid / 2) & ~(grid - 1);
   newy = (newy + grid / 2) & ~(grid - 1);
   }

// Only update the reference point ?
if (! obj)
   {
   refx = newx;
   refy = newy;
   return true;
   }

/* compute the displacement */
dx = newx - refx;
dy = newy - refy;
/* nothing to do? */
if (dx == 0 && dy == 0)
   return false;

/* move the object(s) */
switch (objtype)
   {
   case OBJ_THINGS:
      for (cur = obj; cur; cur = cur->next)
	 {
	 Things[cur->objnum].xpos += dx;
	 Things[cur->objnum].ypos += dy;
	 }
      refx = newx;
      refy = newy;
      MadeChanges = 1;
      break;

   case OBJ_VERTICES:
      for (cur = obj; cur; cur = cur->next)
	 {
	 Vertices[cur->objnum].x += dx;
	 Vertices[cur->objnum].y += dy;
	 }
      refx = newx;
      refy = newy;
      MadeChanges = 1;
      MadeMapChanges = 1;
      break;

   case OBJ_LINEDEFS:
      vertices = list_vertices_of_linedefs (obj);
      MoveObjectsToCoords (OBJ_VERTICES, vertices, newx, newy, grid);
      ForgetSelection (&vertices);
      break;

   case OBJ_SECTORS:
      ObjectsNeeded (OBJ_LINEDEFS, OBJ_SIDEDEFS, 0);
      vertices = list_vertices_of_sectors (obj);
      MoveObjectsToCoords (OBJ_VERTICES, vertices, newx, newy, grid);
      ForgetSelection (&vertices);
      break;
   }
return true;
}



/*
   get the coordinates (approx.) of an object
*/
void GetObjectCoords (int objtype, int objnum, int *xpos, int *ypos) /* SWAP! */
{
int  n, v1, v2, sd1, sd2;
long accx, accy, num;

switch (objtype)
   {
   case OBJ_THINGS:
      if (! is_thing (objnum))		// Can't happen
	 {
	 nf_bug ("GetObjectCoords: bad thing# %d", objnum);
	 *xpos = 0;
	 *ypos = 0;
	 return;
	 }
      ObjectsNeeded (OBJ_THINGS, 0);
      *xpos = Things[objnum].xpos;
      *ypos = Things[objnum].ypos;
      break;

   case OBJ_VERTICES:
      if (! is_vertex (objnum))		// Can't happen
	 {
	 nf_bug ("GetObjectCoords: bad vertex# %d", objnum);
	 *xpos = 0;
	 *ypos = 0;
	 return;
	 }
      ObjectsNeeded (OBJ_VERTICES, 0);
      *xpos = Vertices[objnum].x;
      *ypos = Vertices[objnum].y;
      break;

   case OBJ_LINEDEFS:
      if (! is_linedef (objnum))	// Can't happen
	 {
	 nf_bug ("GetObjectCoords: bad linedef# %d", objnum);
	 *xpos = 0;
	 *ypos = 0;
	 return;
	 }
      ObjectsNeeded (OBJ_LINEDEFS, 0);
      v1 = LineDefs[objnum].start;
      v2 = LineDefs[objnum].end;
      ObjectsNeeded (OBJ_VERTICES, 0);
      *xpos = (Vertices[v1].x + Vertices[v2].x) / 2;
      *ypos = (Vertices[v1].y + Vertices[v2].y) / 2;
      break;

   case OBJ_SIDEDEFS:
      if (! is_sidedef (objnum))	// Can't happen
	 {
	 nf_bug ("GetObjectCoords: bad sidedef# %d", objnum);
	 *xpos = 0;
	 *ypos = 0;
	 return;
	 }
      ObjectsNeeded (OBJ_LINEDEFS, 0);
      for (n = 0; n < NumLineDefs; n++)
	 if (LineDefs[n].sidedef1 == objnum || LineDefs[n].sidedef2 == objnum)
	    {
	    v1 = LineDefs[n].start;
	    v2 = LineDefs[n].end;
	    ObjectsNeeded (OBJ_VERTICES, 0);
	    *xpos = (Vertices[v1].x + Vertices[v2].x) / 2;
	    *ypos = (Vertices[v1].y + Vertices[v2].y) / 2;
	    return;
	    }
      *xpos = (MapMinX + MapMaxX) / 2;
      *ypos = (MapMinY + MapMaxY) / 2;
      // FIXME is the fall through intentional ? -- AYM 2000-11-08

   case OBJ_SECTORS:
      if (! is_sector (objnum))		// Can't happen
	 {
	 nf_bug ("GetObjectCoords: bad sector# %d", objnum);
	 *xpos = 0;
	 *ypos = 0;
	 return;
	 }
      accx = 0L;
      accy = 0L;
      num = 0L;
      for (n = 0; n < NumLineDefs; n++)
	 {
	 ObjectsNeeded (OBJ_LINEDEFS, 0);
	 sd1 = LineDefs[n].sidedef1;
	 sd2 = LineDefs[n].sidedef2;
	 v1 = LineDefs[n].start;
	 v2 = LineDefs[n].end;
	 ObjectsNeeded (OBJ_SIDEDEFS, 0);
	 if ((sd1 >= 0 && SideDefs[sd1].sector == objnum)
	       || (sd2 >= 0 && SideDefs[sd2].sector == objnum))
	    {
	    ObjectsNeeded (OBJ_VERTICES, 0);
	    /* if the Sector is closed, all Vertices will be counted twice */
	    accx += (long) Vertices[v1].x;
	    accy += (long) Vertices[v1].y;
	    num++;
	    accx += (long) Vertices[v2].x;
	    accy += (long) Vertices[v2].y;
	    num++;
	    }
	 }
      if (num > 0)
	 {
	 *xpos = (int) ((accx + num / 2L) / num);
	 *ypos = (int) ((accy + num / 2L) / num);
	 }
      else
	 {
	 *xpos = (MapMinX + MapMaxX) / 2;
	 *ypos = (MapMinY + MapMaxY) / 2;
	 }
      break;

   default:
      nf_bug ("GetObjectCoords: bad objtype %d", objtype);  // Can't happen
      *xpos = 0;
      *ypos = 0;
   }
}



/*
   find a free tag number
*/
int FindFreeTag () /* SWAP! */
{
int  tag, n;
bool ok;

ObjectsNeeded (OBJ_LINEDEFS, OBJ_SECTORS, 0);
tag = 1;
ok = false;
while (! ok)
   {
   ok = true;
   for (n = 0; n < NumLineDefs; n++)
      if (LineDefs[n].tag == tag)
	 {
	 ok = false;
	 break;
	 }
   if (ok)
      for (n = 0; n < NumSectors; n++)
	 if (Sectors[n].tag == tag)
	    {
	    ok = false;
	    break;
	    }
   tag++;
   }
return tag - 1;
}


