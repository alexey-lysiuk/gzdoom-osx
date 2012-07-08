/*
  WADSpy v1.0a (10/21/2000) Estimate a Doom level's difficulty
  Copyright (c) 2000  Oliver Brakmann <obrakmann@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  WADSpy is an adaption of
  WADWHAT v1.1 (released 8/28/1994) (c) by Randall R. Spangler,
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#if defined MSDOS || defined __MSDOS__
#include <limits.h>
#include <unistd.h>
#include <dir.h>
#include <dos.h>
#endif

/* just for looks. I'm an old Pascal geek ;) */

typedef unsigned char  Byte;
typedef unsigned short Word;
typedef unsigned long DWord;

typedef struct {                /* Thing data */
    Word buf[3];                /* Some fields we don't need */
    Word type;                  /* Type of thing */
    Word attr;                  /* Thing attributes */
} THING;

typedef struct {
    DWord thoff, thlen;         /* Position and length of thing data */
    DWord ldoff, ldlen;         /* Position and length of linedef data */
    DWord seoff, selen;         /* Position and length of sector data */
    DWord rjlen;                /* Length of reject data */
} t_lvdata;

typedef struct {
    DWord bullets[6];           /* Total number of given items per skill */
    DWord shells[6];
    DWord rockets[6];
    DWord cells[6];
    DWord health[6];
    DWord armor[6];
} t_exp;

/******************************************************************************/

/* Valid sub-entry names for a level */
char contents[10][9] =
{"THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES", "SEGS", "SSECTORS",
"NODES", "SECTORS", "REJECT", "BLOCKMAP"};

char   map[9] = "\0";       /* if the user wants one map only, here it is */

int    brief;               /* 0 for long output, else holds skill */
Byte   indx;                /* array index to use for output based on 'brief' */

THING *th;                  /* Thing data for a level */
DWord  numth = 0;           /* Number of things */

DWord  savefilepos;         /* Save position in WAD file (where we were
                             * before we started looking at THINGS,
                             * LINEDEFS, or SECTORS) */

static int need_header;

void usage(char *exe)
{
    /* Print the help screen */

    printf("Provides difficulty estimates of a WAD file or files.\n");
    printf("Usage:\n%s [-bn[M]] [-m map] file1 [file2 ...]\n", exe);
    printf("\n\t-bn\tbrief output at skill level n only\n");
    printf("\t-bnM\tbrief output at skill level n only, multiplayer\n");
    printf("\t-m\tdo output for level \"map\" only. (note the space)\n");
    printf("\t\t\"map\" can be either ExMy or MAPxy format.\n\n");
    exit(1);
}


int loadthings(FILE *f, DWord offs, DWord len)
{
    /* Loads a map's THINGS data at offset <offs>, length <len>. */
    /* Returns zero if error. */

    savefilepos = ftell(f);             /* Save position in WAD file */

    fseek(f, offs, SEEK_SET);
    th = (THING *) malloc(len);
    if (!th) {
        fprintf(stderr, "Not enough memory to hold THINGS\n");
        return(0);
    }
    fread(th, 1, len, f);
    numth = len / 10;

    return(1);                           /* Success */
}


void freethings(FILE *f)
{
    /* Frees a map's THINGS data loaded by loadthings(). */

    free(th);
    numth = 0;

    fseek(f, savefilepos, SEEK_SET);

    return;
}


void countlinedefs(FILE *f, DWord offs, DWord len)
{
    /* Counts the interesting linedefs in a map's LINEDEFS data. */

    DWord nlines = len / 14;      /* Total number of linedefs */
    DWord ntrig = 0;              /* Number of linedefs which do something */

    DWord l;
    Word  ibuf[7];

    /** Seek to the LINEDEFS data and loop through the linedefs **/
    savefilepos = ftell(f);       /* Save position in WAD file */

    fseek(f, offs, SEEK_SET);

    for (l = 0; l < nlines; l++) {
        fread(ibuf, sizeof(Word), 7, f); /* Read a linedef */

        if (ibuf[3])
            ntrig++;
    }

    /** Print information **/

    printf("\tTriggers / linedefs    %4ld / %4ld\n", ntrig, nlines);

    /** Seek to original file position and return **/

    fseek(f, savefilepos, SEEK_SET);
    return;
}


void countsectors(FILE *f, DWord offs, DWord len)
{
    /* Counts the interesting sectors in a map's SECTORS data. */

    DWord nsec = len / 26;/* Total number of sectors */
    DWord bright = 0;     /* Average brightness */
    DWord nnuke = 0;      /* Sectors with nukeage */
    DWord nsecret = 0;    /* Secret sectors */

    DWord l;
    Word  ibuf[13];

    /** Seek to the SECTORS data and loop through the sectors **/
    savefilepos = ftell(f);     /* Save position in WAD file */
    fseek(f, offs, SEEK_SET);

    for (l = 0; l < nsec; l++) {
        fread(ibuf, sizeof(Word), 13, f);/* Read a sector */

        bright += ibuf[10];             /* Accumulate brightness */

        switch (ibuf[11]) {             /* Check for specials */
          case 4:                       /* -20%, blinking */
          case 5:                       /* -10% */
          case 7:                       /* -5% */
          case 11:                      /* -20%, end of level */
          case 16:                      /* -20% */
            nnuke++;
            break;
          case 9:                       /* Secret */
            nsecret++;
            break;
        }
    }

    /** Print information **/

    printf("\tAverage brightness            %4ld (0=dark, 255=bright)\n", bright / nsec);

    printf("\tSecrets                       %4ld\n", nsecret);
    printf("\tNukeage / sectors      %4ld / %4ld\n", nnuke, nsec);

    /** Seek to original file position and return **/

    fseek(f, savefilepos, SEEK_SET);

    return;
}


void printthing(DWord *n, char *desc, Byte output)
{
    /* writes an item's data to stdout. Output is controlled by 'output',     */
    /* which can be:                                                          */
    /* 0 = no output; 1 = in brief output, use two digits; 2 = long form only */
    /* anything else is the character that should be used                     */
    /* (for weapons & Equipment in brief format                               */

    Byte i;

    if((!output) || (brief && (output==2)))
        return;

    if(!brief) {                        /* long output */
        printf("\t%-21s", desc);

        for (i = 0; i < 6; i++) {
            if (i == 3)
                printf(" |");
            if (i == 0 || i == 3)       /* little hack, but what gives */
                printf(" %5ld", n[i]);
            printf(" %5ld", n[i]);
        }
        printf("\n");
    } else {                            /* brief output */
        if (output == 1) {              /* print how often the thing appears */
            if (n[indx] > 99)
                printf("++ ");
            else
                printf("%2ld ", n[indx]);
        } else if (output > 2) {        /* print the thing's abbreviation only */
            if (n[indx])
                printf("%c", output);
            else
                putchar('.');
        }
    }

    return;
}


void dodiff(t_exp *exp, DWord *mhp)
{
    /* calculates the difficulty ratio */


    Byte i;
    DWord wdam[6] = {0, 0, 0, 0, 0, 0};
    float ratio[6];

    /* collect all ammo */
    for (i = 0; i < 6; i++) {
        wdam[i] += exp->bullets[i] +
                   exp->shells[i] * 7 +
                   exp->rockets[i] * 20 +
                   exp->cells[i] * 2;
        ratio[i] = (wdam[i] ? ((float) mhp[i] / (float) wdam[i]) : 0.0);
    }

    switch (brief) {
      case 0:  printf("Difficulty:\n");
               printthing(mhp, "Total monster hp", 2);
               printthing(wdam, "Max weapon damage", 2);
               printf("\t%-21s", "RATIO");
               printf(" %0.3f %0.3f %0.3f %0.3f", 0.5 * ratio[0], ratio[0], ratio[1], ratio[2]);
               printf(" | %0.3f %0.3f %0.3f %0.3f\n", 0.5 * ratio[3], ratio[3], ratio[4], ratio[5]);
               break;
      case 1:
      case 5:  printf("  %0.3f\n", 0.5 * ratio[indx]);
               break;
      default: printf("  %0.3f\n", ratio[indx]);
    }

    return;
}


Word countthing(DWord *n, Word type)
{
    /* counts how many things of type 'type' are on each skill and store */
    /* the information in 'n'.                                           */
    /* Retunrs the total number of things found                          */

    DWord i;
    Word found = 0;

    THING *t = th;

    for (i = 0; i < 6; i++)                     /* reset 'n' */
        n[i] = 0;

    for (i = 0; i < numth; i++, t++) {
        if (t->type == type) {                  /* it's the thing we want */
            found++;
            if (t->attr & 0x10) {               /* multi-player only */
                if (t->attr & 0x01) n[3]++;
                if (t->attr & 0x02) n[4]++;
                if (t->attr & 0x04) n[5]++;
            } else {
                if (t->attr & 0x01) n[0]++, n[3]++;
                if (t->attr & 0x02) n[1]++, n[4]++;
                if (t->attr & 0x04) n[2]++, n[5]++;
            }
        }
    }

    return(found);
}


void dothing(DWord *n, char *desc, Word type, DWord *exp, Byte numexp,
             DWord *mhp, Word hp, Byte output)
{
    /* puts all the information on a thing into the right places.        */

    Byte i;

    Word found = countthing(n, type);       /* Count the thing */

    if (exp != NULL)                        /* thing provides ammo, health etc. */
        for(i = 0; i < 6; i++)
            exp[i] += n[i] * numexp;

    if (mhp != NULL)                        /* if thing is a monster, add */
        for (i = 0; i < 6; i++)             /* its hp to the total */
            mhp[i] += n[i] * hp;

    if (output && (brief || found))
        printthing(n, desc, output);

    return;
}


void doplayerstarts(DWord *n)
{
    /* checks the presence of all player starts */

    Word numstarts = 0;
    Byte i;

    printf("    Play modes:\n");

    if (countthing(n, 1))              /* Single player */
        printf("\tSingle player\n");

    for (i = 1; i <= 4; i++) {         /* Find player 1-4 starts */
        if (countthing(n, i))
            numstarts++;
    }
    if (numstarts > 1)
        printf("\tCooperative (%d player)\n", numstarts);

    numstarts = countthing(n, 11);     /* Find deathmatch starts */
    if (numstarts)
        printf("\tDeathmatch (%d starts)\n", numstarts);

    return;
}


void PrintAllStuff(FILE *f, t_lvdata *lv)
{
    /* Prints all information for a map. */

    DWord n[6], mhp[6] = {0, 0, 0, 0, 0, 0};
    t_exp exp = {{0, 0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0, 0}};
    Byte i;

    /** Print things **/
    if (lv->thlen) {
        if (loadthings(f, lv->thoff, lv->thlen)) {      /* Load things */
            if (!brief) {
              doplayerstarts(n);
              printf("    Monsters:\n");
            }
            dothing(n, "Trooper"         , 3004, exp.bullets,  10,  mhp,   2,   1);
            dothing(n, "Sergeant"        ,    9, exp.shells ,   4,  mhp,   3,   1);
            dothing(n, "Wolfenstein guy" ,   84, exp.bullets,   5,  mhp,   5,   2);
            dothing(n, "Chaingunner"     ,   65, exp.bullets,  10,  mhp,   7,   1);
            dothing(n, "Imp"             , 3001, NULL       ,   0,  mhp,   6,   1);
            dothing(n, "Demon"           , 3002, NULL       ,   0,  mhp,  15,   1);
            dothing(n, "Spectre"         ,   58, NULL       ,   0,  mhp,  15,   1);
            dothing(n, "Lost Soul"       , 3006, NULL       ,   0,  mhp,  10,   1);
            dothing(n, "Cacodemon"       , 3005, NULL       ,   0,  mhp,  40,   1);
            dothing(n, "Hell Knight"     ,   69, NULL       ,   0,  mhp,  50,   1);
            dothing(n, "Revenant"        ,   66, NULL       ,   0,  mhp,  30,   1);
            dothing(n, "Arachnotron"     ,   68, NULL       ,   0,  mhp,  50,   1);
            dothing(n, "Pain Elemental"  ,   71, NULL       ,   0,  mhp,  40,   1);
            dothing(n, "Mancubus"        ,   67, NULL       ,   0,  mhp,  60,   1);
            dothing(n, "Arch-Vile"       ,   64, NULL       ,   0,  mhp,  70,   1);
            dothing(n, "Baron"           , 3003, NULL       ,   0,  mhp, 100,   1);
            dothing(n, "Spiderdemon"     ,    7, NULL       ,   0,  mhp, 300,   1);
            dothing(n, "Cyberdemon"      ,   16, NULL       ,   0,  mhp, 400,   1);
            dothing(n, "Icon Shooter"    ,   88, NULL       ,   0,  mhp,  25,   1);

            if (!brief)
                printf("    Weapons:\n");
            dothing(n, "Chainsaw"        , 2005, NULL       ,   0, NULL,   0, '1');
            dothing(n, "Shotgun"         , 2001, exp.shells ,   8, NULL,   0, '3');
            dothing(n, "Super shotgun"   ,   82, exp.shells ,   8, NULL,   0, 'D');
            dothing(n, "Chaingun"        , 2002, exp.bullets,  20, NULL,   0, '4');
            dothing(n, "Rocket launcher" , 2003, exp.rockets,   2, NULL,   0, '5');
            dothing(n, "Plasma rifle"    , 2004, exp.cells  ,  40, NULL,   0, '6');
            dothing(n, "BFG-9000"        , 2006, exp.cells  ,  40, NULL,   0, '7');

            if (!brief)
                printf("    Equipment:\n");
            else
                putchar(' ');
            dothing(n, "Backpack"        ,    8, NULL       ,   0, NULL,   0, 'B');
            for (i = 0; i < 6; i++) {                  /* oops, a hack */
                exp.bullets[i] += n[i] * 10;
                exp.shells[i]  += n[i] *  4;
                exp.rockets[i] += n[i] *  1;
                exp.cells[i]   += n[i] * 20;
            }
            dothing(n, "Invulnerability" , 2022, NULL       ,   0, NULL,   0, 'V');
            dothing(n, "Berserk"         , 2023, exp.health , 100, NULL,   0, 'S');
            dothing(n, "Invisibility"    , 2024, NULL       ,   0, NULL,   0, 'I');
            dothing(n, "Radiation suit"  , 2025, NULL       ,   0, NULL,   0, 'R');
            dothing(n, "Computer map"    , 2026, NULL       ,   0, NULL,   0, 'A');
            dothing(n, "Lite amp goggles", 2045, NULL       ,   0, NULL,   0, 'L');

            if (!brief)
                printf("    Expendibles:\n");
            dothing(n, NULL              , 2007, exp.bullets,  10, NULL,   0,   0);
            dothing(n, NULL              , 2048, exp.bullets,  50, NULL,   0,   0);
            dothing(n, NULL              , 2008, exp.shells ,   4, NULL,   0,   0);
            dothing(n, NULL              , 2049, exp.shells ,  20, NULL,   0,   0);
            dothing(n, NULL              , 2010, exp.rockets,   1, NULL,   0,   0);
            dothing(n, NULL              , 2046, exp.rockets,   5, NULL,   0,   0);
            dothing(n, NULL              , 2047, exp.cells  ,  20, NULL,   0,   0);
            dothing(n, NULL              ,   17, exp.cells  , 100, NULL,   0,   0);

            dothing(n, NULL              , 2015, exp.armor  ,   1, NULL,   0,   0);
            dothing(n, NULL              , 2018, exp.armor  , 100, NULL,   0,   0);
            dothing(n, NULL              , 2019, exp.armor  , 200, NULL,   0,   0);

            dothing(n, NULL              , 2014, exp.health ,   1, NULL,   0,   0);
            dothing(n, NULL              , 2011, exp.health ,  10, NULL,   0,   0);
            dothing(n, NULL              , 2012, exp.health ,  25, NULL,   0,   0);
            dothing(n, NULL              , 2013, exp.health , 100, NULL,   0,   0);
            dothing(n, NULL              ,   83, exp.health , 200, NULL,   0,   0);
            for (i = 0; i < 6; i++)                    /* oh, another hack */
                exp.armor[i] += n[i] * 200;

            if (!brief) {
                printthing(exp.bullets, "Bullets"      , 2);
                printthing(exp.shells , "Shells"       , 2);
                printthing(exp.rockets, "Rockets"      , 2);
                printthing(exp.cells  , "Cells"        , 2);
                printthing(exp.armor  , "Armor points" , 2);
                printthing(exp.health , "Health points", 2);
                dothing(n, "Barrels"         , 2035, NULL       ,   0, NULL,   0,   2);
            }
            dodiff(&exp, mhp);

            freethings(f);              /* Free array, restore original file
                                         * position */
        }
    }

    /** If brief info, return now (no extended info) **/
    if (brief)
        return;

    /** Print other info **/
    printf("    Other info:\n");

    if (lv->selen)
        countsectors(f, lv->seoff, lv->selen);
    if (lv->ldlen)
        countlinedefs(f, lv->ldoff, lv->ldlen);

    printf("\tReject resource                %s\n", (lv->rjlen > 0 ? "YES" : " NO"));

    return;
}


void PrintHeader(char *map)
{
    if (brief) {
        switch(map[0]) {     /* One-line output */
          case 'M': printf("%d%d ", map[3]-'0', map[4]-'0');
                    break;
          case 'E': printf("%d%d ", map[1]-'0', map[3]-'0');
                    break;
        }
    } else {                    /* Verbose output */
        printf("-------------------------------------------------------------------------------\n");
        printf("%-5s                            S1    S2    S3   S45 |    M1    M2    M3   M45\n", map);
        printf("-------------------------------------------------------------------------------\n");
    }
}


void SeekLevels(FILE *f)
{
    /* browses the directory and invokes PrintAllStuff() if a level is found */

    DWord ndirent;        /* Number of entries in WAD directory */
    DWord diroffs;        /* Offset of directory in WAD file */

    DWord eoffs, elen;    /* Offset and length of a directory entry */

    t_lvdata lv = {0, 0, 0, 0, 0, 0, 0};

    char  ename[9] = "\0";            /* Name of the entry */

    char  cur_map[9] = "\0";          /* name of the map we're working on */
    Word  i;
    Byte  j, found = 0;

    fread(&ndirent, sizeof(DWord), 1, f); /* Number of entries in WAD dir */
    fread(&diroffs, sizeof(DWord), 1, f); /* Offset of directory in WAD */
    fseek(f, diroffs, SEEK_SET);          /* Go to the directory */

    for (i = 0; i < ndirent; i++) {

        /** Read entry **/

        fread(&eoffs, sizeof(DWord), 1, f);      /* Offset of entry's data */
        fread(&elen, sizeof(DWord), 1, f);       /* Length of entry's data */
        fread(&ename, sizeof(char), 8, f);       /* Name of entry */
        {
            int n;
            for (n = 0; n < 8; n++)
                ename[n] = toupper(ename[n]);
        }

        /** Look what kind of entry we've found **/

        for (j = 0; j < 10; j++) {
            if (!strcmp(ename, contents[j]))
                break;                  /* Matched valid contents for a level */
        }

        switch (j) {
          case 10:                      /* Not level contents */

            /** Look at all the stuff we've found **/
            if (*cur_map && (!(*map) || found)) {
                PrintHeader(cur_map);
                PrintAllStuff(f, &lv);

                /** Reset status **/
                *cur_map = 0;
                lv.thlen = lv.ldlen = lv.selen = lv.rjlen = 0;
                lv.thoff = lv.ldoff = lv.seoff = 0;
            }

            /** got stats of the specified map (-M), wanna quit now **/
            if (found)
                return;

            /** does the entry label a new level? **/
            if ((!strncmp("MAP", ename, 3)
                 && isdigit(ename[3]) && isdigit(ename[4]) && ename[5]=='\0')
                || (ename[0]=='E' && isdigit(ename[1])
                 && ename[2]=='M' && isdigit(ename[3]) && ename[4]=='\0')) {
                strcpy(cur_map, ename);

                if (!strcmp(cur_map, map))  /* is this the map we're looking for? */
                    found = 1;
            }
            break;

          case 0:                       /* THINGS */
            lv.thoff = eoffs;
            lv.thlen = elen;
            break;

          case 1:                       /* LINEDEFS */
            lv.ldoff = eoffs;
            lv.ldlen = elen;
            break;
          case 7:                       /* SECTORS */
            lv.seoff = eoffs;
            lv.selen = elen;
            break;
          case 8:                       /* REJECT */
            lv.rjlen = elen;
            break;
        }
    }

    /** Look at all the stuff we found from the last map, if any **/

    if (*cur_map) {
        PrintHeader(cur_map);
        PrintAllStuff(f, &lv);
    }

    return;
}


void handlewad(char *fname)
{
    /* Handles a WAD file. */

    Byte  ispwad = 0;     /* Are we a PWAD? (or an IWAD) */
    char  buf[4];
    FILE *f;

    /*** Open the WAD file ***/

    f = fopen(fname, "rb");
    if (!f) {
        fprintf(stderr, "Can't open file %s (%s)\n", fname, strerror(errno));
        return;
    }

    if (need_header) {
        printf("## Monsters                                              Weapons Equip    RATIO\n");
        printf("---tr-sg-cg-im-de-sp-ls-ca-hk-rv-ar-pe-mc-av-br-sd-cy-ic-cs2mrpb-bvsiral-------\n");
        need_header = 0;
    }

    fread(buf, sizeof(char), 4, f); /* Read the file type designator */
    if (!strncmp(buf, "IWAD", 4)) {
        ispwad = 0;
    } else if (!strncmp(buf, "PWAD", 4)) {
        ispwad = 1;
    } else {
        fprintf(stderr, "%s is not a DOOM WAD file\n", fname);
        fclose(f);
        return;
    }

    if (brief) {                       /* Start a new filename in the output */
        printf("%s:\n", fname);
    } else {
        printf("===============================================================================\n");
        printf("%cWAD FILE %s:\n", (ispwad ? 'P' : 'I'), fname);
    }

    SeekLevels(f);

    fclose(f);
    return;
}


int main(int argc, char **argv)
{
    Word i;
    Byte skill[10] = {0, 0, 1, 2, 2, 3, 3, 4, 5, 5};

#if defined MSDOS || defined __MSDOS__
    struct ffblk ff;
    char olddir[PATH_MAX];
    getcwd(olddir, PATH_MAX);
#endif

    printf("WADSpy v1.0c (2003-01-04)            (c) by Oliver Brakmann <obrakmann@gmx.net>\n\n");

    /* checking cmdline args */

    if (argc < 2)
        usage(argv[0]);    /* show usage and quit */

    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "-b", 2)) {      /* brief output wanted */
            if ((atoi(argv[i] + 2) >= 1) && (atoi(argv[i] + 2) <= 5))
                brief = atoi(argv[i] + 2);
            else {
                fprintf(stderr, "Invalid skill level (%c)\n", (argv[i][2]));
                fprintf(stderr, "Defaulting to verbose output\n");
                brief = indx = 0;
                continue;
            }

            if (toupper(argv[i][3]) == 'M')
                indx = skill[brief - 1 + 5];
            else
                indx = skill[brief - 1];

            need_header = 1;
            continue;

        } else if (!strcmp(argv[i], "-m")) {

            char level[10] = "\0";
            strncat(level, argv[i + 1], sizeof(level) - 1);
            {
                char *p = level;
                while (*p)
                    *p = toupper(*p++);
            }

            if (((!strncmp("MAP", level, 3))                /* Doom II map */
                 && isdigit(level[3]) && isdigit(level[4])
                 && level[5]=='\0')
                || (level[0]=='E' && isdigit(level[1])      /* Doom I map */
                 && level[2]=='M' && isdigit(level[3])
                 && level[4]=='\0')) {
                strcpy(map, level);

                i++;                /* skip the next arg (the level name) */
            } else {
                fprintf(stderr, "Invalid level name \"%s\"\n", argv[i + 1]);
                exit(1);
            }
            continue;
        }

#if defined MSDOS || defined __MSDOS__
        chdir(dirname(argv[i]));
        if (findfirst(argv[i], &ff, FA_ARCH)) { /* No match for wildcard */
            fprintf(stderr, "Can't find file matching %s\n", argv[i]);
            continue;
        }

        do {
            handlewad(ff.ff_name);
            putchar('\n');
        } while (!findnext(&ff));
        chdir(olddir);
#else
        handlewad(argv[i]);
        putchar('\n');
#endif
    }

    return(0);
}
