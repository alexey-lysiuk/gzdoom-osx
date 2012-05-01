/*
 *	wadext2.h
 *	AYM 2001-08-11
 */


#ifndef WADEXT2_H
#define WADEXT2_H


typedef struct				/* Wad directory entry */
{
  unsigned long num;
  long ofs;
  long size;
  char name[8];
} waddirent_t;


void err (const char *fmt, ...);
void warn (const char *fmt, ...);


extern const char version[];		/* Defined in version.c */


#endif
