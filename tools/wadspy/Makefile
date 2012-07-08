#
#	Makefile for Wadspy
#	AYM 2000-07-20
#

PREFIX = /usr/local
CC     = cc
CFLAGS = -Os

all: wadspy

wadspy: wadspy.c Makefile
	$(CC) $(CFLAGS) wadspy.c -o wadspy

install:
	cp -p wadspy $(PREFIX)/bin
	[ ! -e $(PREFIX)/share/man ] || cp -p wadspy.6 $(PREFIX)/share/man/man6
	[ ! -e $(PREFIX)/man       ] || cp -p wadspy.6 $(PREFIX)/man/man6
