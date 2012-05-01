#
#	Makefile for wadext2
#	AYM 2001-08-11
#


PREFIX = /usr/local


VERSION = `cat VERSION`
ARC     = wadext2-$(VERSION)


default: all

all: wadext2

wadext2: format.o version.o wadext2.o

format.o: format.c format.h wadext2.h

version.c: VERSION
	printf 'const char version[] = "%s";\n' $(VERSION) >$@

version.o: version.c

wadext2.o: format.h wadext2.c wadext2.h

install:
	cp wadext2 $(PREFIX)/bin
	touch -r wadext2 $(PREFIX)/bin/wadext2
	if [ "$(PREFIX)" = /usr ]; then					\
	  echo $(PREFIX)/share/man >.mandir;				\
	else								\
	  echo $(PREFIX)/man >.mandir;					\
	fi
	cp wadext2.6 "`cat .mandir`/man6"
	touch -r wadext2.6 "`cat .mandir`/man6/wadext2.6"

clean:
	rm -f .mandir format.o wadext2 wadext2.o

dist:
	mkdir $(ARC)
	cp -p								\
	  CHANGES							\
	  COPYING							\
	  Makefile							\
	  README							\
	  VERSION							\
	  format.c							\
	  format.h							\
	  version.c							\
	  wadext2.6							\
	  wadext2.c							\
	  wadext2.h							\
	  $(ARC)
	tar -cf $(ARC).tar $(ARC)
	rm -rf $(ARC)
	gzip -f $(ARC).tar

