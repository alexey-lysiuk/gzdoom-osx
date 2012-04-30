/*
 *	macsup.c - macro processing support functions for cawf(1)
 */

/*
 *	Copyright (c) 1991 Purdue University Research Foundation,
 *	West Lafayette, Indiana 47907.  All rights reserved.
 *
 *	Written by Victor A. Abell <abe@mace.cc.purdue.edu>,  Purdue
 *	University Computing Center.  Not derived from licensed software;
 *	derived from awf(1) by Henry Spencer of the University of Toronto.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to alter it and redistribute
 *	it freely, subject to the following restrictions:
 *
 *	1. The author is not responsible for any consequences of use of
 *	   this software, even if they arise from flaws in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *	   by explicit claim or by omission.  Credits must appear in the
 *	   documentation.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *	   be misrepresented as being the original software.  Credits must
 *	   appear in the documentation.
 *
 *	4. This notice may not be removed or altered.
 */

#include "cawf.h"
#ifdef	UNIX
#else
#include <malloc.h>
#endif


/*
 * Delmacro(mx) - delete macro
 */

Delmacro(mx)
	int mx;				/* macro index */
{
	char buf[MAXLINE];		/* error message buffer */
	int i, j;			/* temporary indexes */

	if (mx >= Nmac) {
		(void) sprintf(buf, " bad Delmacro(%d) index", mx);
		Error(FATAL, LINE, buf, NULL);
	}
	for (i = Macrotab[mx].bx, j = i + Macrotab[mx].ct; i < j; i++) {
		Free(&Macrotxt[i]);
	}
	for (i = mx; i < (Nmac - 1); i++) {
		Macrotab[i] = Macrotab[i+1];
	}
	Nmac--;
}


/*
 * Field(n, p, c) - skip to field n in p and optionally return a copy
 */

char *
Field(n, p, c)
	int n;				/* field number */
	char *p;			/* point to line containing fields */
	int c;				/* 1: make a copy of the field */
{
	char *fs, *fe, *s;

	if (c)
		Free(&F);
	fe = p;
	while (n) {
		while (*fe == ' ' || *fe == '\t')
			fe++;
		fs = fe;
		while (*fe && *fe != ' ' && *fe != '\t')
			fe++;
		if (fs == fe)
			return(NULL);
		if (n == 1) {
			if ( ! c)
				return(fs);
			if ((F = malloc((size_t)(fe - fs + 1))) == NULL)
				Error(FATAL, LINE, " Field out of string space",
					NULL);
			(void) strncpy(F, fs, (fe - fs));
			F[fe -fs] = '\0';
			return(F);
		}
		n--;
	}
	return(NULL);
}

/*
 * Findmacro(p, e) - find macro and optionally enter it
 *
 * return = Macrotab[] index or -1 if not found
 */


Findmacro(p, e)
	char *p;                /* pointer to 2 character macro name  */
	int e;			/* 0 = find, don't enter
				 * 1 = enter, don't find */
{
	char c[3];
	int cmp, hi, low, mid;

	c[0] = p[0];
	c[1] = (p[1] == ' ' || p[1] == '\t') ? '\0' : p[1];
	c[2] = '\0';
	low = mid = 0;
	hi = Nmac - 1;
	while (low <= hi) {
		mid = (low + hi) / 2;
		if ((cmp = strncmp(c, Macrotab[mid].name, 2)) < 0)
			hi = mid - 1;
		else if (cmp > 0)
			low = mid + 1;
		else {
			if ( ! e)
				return(mid);
			 Error(WARN, LINE, " duplicate macro ", c);
			 hi = Macrotab[mid].bx + Macrotab[mid].ct;
			 for (low = Macrotab[mid].bx; low < hi; low++) {
				Free(&Macrotxt[low]);
			 }
			 goto new_macro;
		}
	}
	if ( ! e)
		return(-1);
	if (Nmac >= MAXMACRO)
		Error(FATAL, LINE, " macro table full at ", c);
	if (Nmac) {
		if (cmp > 0)
			mid++;
		for (hi = Nmac - 1; hi >= mid; hi--)
			Macrotab[hi+1] = Macrotab[hi];
	}
	Nmac++;
	Macrotab[mid].name[0] = c[0];
	Macrotab[mid].name[1] = c[1];

new_macro:

	Macrotab[mid].bx = -1;
	Macrotab[mid].ct = 0;
	return(mid);
}

Free(p)
	char **p;
{
	if (*p != NULL) {
		(void) free(*p);
		*p = NULL;
	}
}

/*
 * Newstr(s) - allocate space for string
 */

char *
Newstr(s)
	char *s;
{
	char *ns;

	if ((ns = malloc((size_t)(strlen(s) + 1))) == NULL)
		Error(FATAL, LINE, " Newstr out of malloc space at ", s);
	(void) strcpy(ns, s);
	return(ns);
}
