# $Id: Makefile,v 1.1 2005/06/28 21:05:19 jcs Exp $
# vim:ts=8

CC	= cc
CFLAGS	= -O2 -Wall -Wunused -Wmissing-prototypes -Wstrict-prototypes

PREFIX	= /usr/local
BINDIR	= $(DESTDIR)$(PREFIX)/bin

INSTALL_PROGRAM = install -s

X11BASE	= /usr/X11R6
INCLUDES= -I$(X11BASE)/include
LDPATH	= -L$(X11BASE)/lib
LIBS	= -lX11

PROG	= pixelclock
OBJS	= pixelclock.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): *.o
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: all
	$(INSTALL_PROGRAM) $(PROG) $(BINDIR)

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: all install clean
