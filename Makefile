# $Id: Makefile,v 1.2 2005/06/28 21:27:55 jcs Exp $
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

VERS	:= `grep Id pixelclock.c | sed -e 's/.*,v //' -e 's/ .*//'`

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): *.o: *.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: all
	$(INSTALL_PROGRAM) $(PROG) $(BINDIR)

clean:
	rm -f $(PROG) $(OBJS) 

release: all
	@mkdir $(PROG)-${VERS}
	@cp *.c $(PROG)-$(VERS)/
	@tar -czf $(PROG)-$(VERS).tar.gz $(PROG)-$(VERS)
	@rm -rf $(PROG)-$(VERS)/
	@echo "made release ${VERS}"

.PHONY: all install clean
