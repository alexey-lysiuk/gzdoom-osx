/************************************************************************/
/*      Copyright (C) 1999-2000 by Udo Munk (munkudo@aol.com)           */
/*      Copyright (C) 2001      by André Majorel                        */
/*                              (http://www.teaser.fr/~amajorel/)       */
/*                                                                      */
/*      Permission to use, copy, modify, and distribute this software   */
/*      and its documentation for any purpose and without fee is        */
/*      hereby granted, provided that the above copyright notice        */
/*      appears in all copies and that both that copyright notice and   */
/*      this permission notice appear in supporting documentation.      */
/*      The author and contibutors make no representations about the    */
/*      suitability of this software for any purpose. It is provided    */
/*      "as is" without express or implied warranty.                    */
/************************************************************************/

/*
 *      Tool to extract sprites from a WAD file into ppm files
 */

/*
 * 1.2 (UM 1999-02-15)
 * - Added support for Heretic and Hexen.
 *
 * 1.3 (UM 1999-03-25)
 * - Added support for Strife.
 *
 * 1.4 (UM 2000-01-09)
 * - Stupid Windows can't create files with \ in filename. So don't
 *   abort on write errors immediately, just count the errors and
 *   go on.
 *
 * 1.5 (AYM 2001-10-01)
 * - Recognise SS_START.
 * - Between S_START/SS_START and S_END, ignore any spurious S_START
 *   or SS_START.
 * - Outside S_START/SS_START and S_END, ignore any spurious S_END.
 * - Case-insensitive lump name comparison (E.G. works with
 *   "ss_start").
 * - Use get_lump_by_num() instead of get_lump_by_name() (safer and
 *   faster).
 *
 * 1.6 (AYM 2002-11-11)
 * - New -w option.
 * - Usage goes to standard output, not standard error.
 * - Put options in alphabetical order.
 * - Create the sprites/ directory with mode 777 and let umask do the
 *   rest.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fnmatch.h>

#include "sysdep.h"
#include "strfunc.h"
#include "wad.h"
#include "lump_dir.h"
#include "wadfile.h"

#define VERSION "1.6"

extern unsigned char doom_rgb[];
extern unsigned char heretic_rgb[];
extern unsigned char hexen_rgb[];
extern unsigned char strife_rgb[];
unsigned char *palette = doom_rgb;
static unsigned char img_buf[320 * 200];
static int preserve_case = 0;
static char *wildcard = NULL;
static int errors = 0;

void usage(char *name, char *option)
{
	if (option)
		fprintf(stderr, "%s: Unkown option: %s\n", name, option);

	printf("Usage: %s [-c pal] [-p] [-w expr] wadfile\n", name);
	printf("\t-c pal: use color palette pal, where pal can be\n");
	printf("\t        doom, heretic, hexen or strife\n");
	printf("\t-p: preserve case of generated files\n");
	printf("\t-w expr: only extract sprites matching wildcard expr\n");
	exit(1);
}

void write_ppm(char *name, short width, short height)
{
	FILE		*fp;
	char		fn[50];
	int		i, j;
	unsigned char	pixel;

	strcpy(&fn[0], "sprites/");
	if (preserve_case)
		strcat(&fn[0], name);
	else
		strlcat(&fn[0], name);
	strcat(&fn[0], ".ppm");
	if ((fp = fopen(&fn[0], "wb")) == NULL) {
		fprintf(stderr, "can't open %s for writing\n", &fn[0]);
		errors++;
		return;
	}

	fprintf(fp, "P6\n");
	fprintf(fp, "# CREATOR: wadsprit Release %s\n", VERSION);
	fprintf(fp, "%d %d\n", width, height);
	fprintf(fp, "255\n");

	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			pixel = img_buf[j * width + i];
			fputc((int) palette[pixel * 3],     fp);
			fputc((int) palette[pixel * 3 + 1], fp);
			fputc((int) palette[pixel * 3 + 2], fp);
		}
	}

	fclose(fp);
}

void decompile(wadfile_t *wf, int num)
{
  	char		name[9];
	unsigned char	*sprite;
	short		width;
	short		height;
	short		xoff;
	short		yoff;
	short		*p1;
	int		*columns;
	unsigned char	*post;
	int		x;
	int		y;
	int		n;
	int		i;

	name[0] = '\0';
	strncat(name, wf->lp->lumps[num]->name, 8);

	/* get sprite lump */
	if ((sprite = (unsigned char *)get_lump_by_num(wf, num)) == NULL) {
		fprintf(stderr, "can't find sprite lump %s\n", name);
		exit(1);
	}

	/* clear the pixel buffer */
	memset(img_buf, 255, 320 * 200);

	/* get width, height and offsets from the lump header */
	p1 = (short *) sprite;
	width = *p1++;
	swapint(&width);
	height = *p1++;
	swapint(&height);
	xoff = *p1++;
	swapint(&xoff);
	yoff = *p1++;
	swapint(&yoff);
	
	printf("  got sprite %s, width=%d, height=%d\n", name,
		width, height);

	/* copy picture data into pixel buffer */
	columns = (int *) (sprite + 8);
	for (x = 0; x < width; x++) {
	  swaplong(&columns);
	  post = sprite + *columns++;
	  y = (int) *post++;
	  while (y != 255) {
	    n = (int) *post++;
	    post++;	/* first byte unused */
	    for (i = 0; i < n; i++) {
		img_buf[(y+i) * width + x] = *post;
		post++;
	    }
	    post++;	/* last byte unused */
	    y = (int) *post++;
	  }
	}

	/* free sprite lump memory */
	free(sprite);

	/* write image buffer into ppm file */
	write_ppm(name, width, height);
}

int main(int argc, char **argv)
{
	char		*program;
	char		*s;
	wadfile_t	*wf;
	int		i;
	int		start_flag = 0;

	/* save program name for usage() */
	program = *argv;

	printf("%s version %s\n\n", program, VERSION);

	/* process options */
	while ((--argc > 0) && (**++argv == '-')) {
	  for (s = *argv+1; *s != '\0'; s++) {
		switch (*s) {
		case 'c':
			argc--;
			argv++;
			if (!strcmp(*argv, "doom"))
				palette = doom_rgb;
			else if (!strcmp(*argv, "heretic"))
				palette = heretic_rgb;
			else if (!strcmp(*argv, "hexen"))
				palette = hexen_rgb;
			else if (!strcmp(*argv, "strife"))
				palette = strife_rgb;
			else
				usage(program, NULL);
			break;
		case 'p':
			preserve_case++;
			break;
		case 'w':
			argc--;
			argv++;
			wildcard = *argv;
			{
				char *p;
				for (p = wildcard; *p != '\0'; p++)
					*p = toupper(*((unsigned char *) p));
			}
			break;
		default:
			usage(program, --s);
		}
	  }
	}

	/* have one argument left? */
	if (argc != 1)
		usage(program, NULL);

	/* open WAD file */
	wf = open_wad(*argv);

	/*
	 * make sprites directory, ignore errors, we'll handle that later
	 * when we try to write the graphics files into it
	 */
	mkdir("sprites", 0777);

	/* loop over all lumps and look for the sprites */
	for (i = 0; i < wf->wh.numlumps; i++) {
		/* start processing after S_START */
		if (!start_flag
		    && !lump_name_cmp(wf->lp->lumps[i]->name, "S_START")) {
			start_flag = 1;
			printf("found S_START, start decompiling sprites\n");
			continue;
		} else if (!start_flag
		    && !lump_name_cmp(wf->lp->lumps[i]->name, "SS_START")) {
			start_flag = 1;
			printf("found SS_START, start decompiling sprites\n");
			continue;
		} else if (start_flag
		    && !lump_name_cmp(wf->lp->lumps[i]->name, "S_END")) {
			start_flag = 0;
			printf("found S_END, done\n");
			break;
		} else {
			if (start_flag) {
			  char name[9];
			  size_t n;

			  for (n = 0; n < 8; n++) {
			    name[n] = toupper(wf->lp->lumps[i]->name[n]);
			  }
			  name[n] = '\0';
			  if (wildcard != NULL
			  && fnmatch(wildcard, name, 0) != 0)
			    continue;
			  decompile(wf, i);
			}
		}
	}

	/* print number of errors (for Windows) */
	printf("\n\nNumber of write errors: %d\n", errors);

	/* clean up and done */
	close_wad(wf);
	return 0;
}
