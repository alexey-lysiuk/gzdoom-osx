/*
 *	oldmenus.h
 *	AYM 1998-12-04
 */


#ifndef YH_OLDMENUS  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_OLDMENUS


class Menu_data;


int vDisplayMenu (int, int, const char *, ...);

int DisplayMenuList (
   int		x0,
   int		y0,
   const char	*title,
   al_llist_t	*list,
   const char	*(*getstr)(void *),
   int		*item_no);

int DisplayMenuList (
  int		x0,
  int		y0,
  const char	*menutitle,
  Menu_data&    menudata,
  int		*item_no);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
