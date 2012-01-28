/*
 *	editloop.h
 *	AYM 1998-09-06
 */


void EditorLoop (const char *); /* SWAP! */
const char *SelectLevel (int levelno);
extern int InputSectorType(int x0, int y0, int *number);
extern int InputLinedefType(int x0, int y0, int *number);
extern int InputThingType(int x0, int y0, int *number);




