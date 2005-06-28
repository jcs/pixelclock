/* vim:ts=8
 * $Id: pixelclock.c,v 1.3 2005/06/28 22:05:56 jcs Exp $
 *
 * pixelclock
 * a different way of looking at time
 *
 * Copyright (c) 2005 joshua stein <jcs@jcs.org>
 * Copyright (c) 2005 Federico G. Schwindt
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* default clock width */
#define DEFWIDTH 3

/* so our window manager knows us */
char* win_name = "pixelclock";

/* hours to highlight (start work, lunch, etc.) */
int hihours[] = { 9, 12, 17 };

struct xinfo {
	Display* dpy;
	int dpy_width, dpy_height;

	int screen;

	Window win;

	int width;

	GC gc;

	Colormap win_colormap;
} x;

const struct option longopts[] = {
	{ "display",	required_argument,	NULL,	'd' },
	{ "width",	required_argument,	NULL,	'w' },

	{ NULL,		0,			NULL,	0 }
};

extern char *__progname;

long	getcolor(const char *);
void	handler(int sig);
void	init_x(const char *);
void	usage(void);

int
main(int argc, char* argv[])
{
	char *display = NULL, *p;
	int c, hi, y, z;
	int hourtick, lastpos = -1, newpos = 0;
	struct timeval tv[2];
	time_t now;
	struct tm *t;

	bzero(&x, sizeof(struct xinfo));

	x.width = DEFWIDTH;

	while ((c = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
		switch (c) {
		case 'd':
			display = optarg;
			break;

		case 'w':
			x.width = strtol(optarg, &p, 10);
			if (*p || x.width < 1)
				errx(1, "illegal value -- %s", optarg);
				/* NOTREACHED */
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}

	init_x(display);

	signal(SIGINT, handler);
	signal(SIGTERM, handler);

	/* each hour will be this many pixels away */
	hourtick = x.dpy_height / 24;

	for (;;) {
        	if (gettimeofday(&tv[0], NULL))
			errx(1, "gettimeofday");
			/* NOTREACHED */

		now = tv[0].tv_sec;
		if ((t = localtime(&now)) == NULL)
			errx(1, "localtime");
			/* NOTREACHED */

		newpos = (hourtick * t->tm_hour) +
			(float)(((float)t->tm_min / 60.0) * hourtick) - 3;

		if (newpos != lastpos) {
			XClearWindow(x.dpy, x.win);

			XSetForeground(x.dpy, x.gc, getcolor("yellow"));
			XFillRectangle(x.dpy, x.win, x.gc,
				0, newpos,
				x.width, 6);

			/* draw the hour marks */
			for (y = 1; y <= 23; y++) {
				hi = 0;
				for (z = 0; z < sizeof(&hihours); z++)
					if (y == hihours[z]) {
						hi = 1;
						break;
					}

				if (hi)
					XSetForeground(x.dpy, x.gc,
					    getcolor("green"));
				else
					XSetForeground(x.dpy, x.gc,
					    getcolor("blue"));

				XFillRectangle(x.dpy, x.win, x.gc,
					0, (y * hourtick),
					x.width, 2);
			}

			lastpos = newpos;

			XFlush(x.dpy);
		}

		sleep(1);
	}

	exit(1);
}

void
init_x(const char *display)
{
	int rc;
	XGCValues values;
	XTextProperty win_name_prop;

	if (!(x.dpy = XOpenDisplay(display)))
		errx(1, "Unable to open display %s", XDisplayName(display));
		/* NOTREACHED */

	x.screen = DefaultScreen(x.dpy);

	x.dpy_width = DisplayWidth(x.dpy, x.screen);
	x.dpy_height = DisplayHeight(x.dpy, x.screen);

	x.win_colormap = DefaultColormap(x.dpy, DefaultScreen(x.dpy));

	x.win = XCreateSimpleWindow(x.dpy, RootWindow(x.dpy, x.screen),
			x.dpy_width - x.width, 0,
			x.width, x.dpy_height,
			0,
			BlackPixel(x.dpy, x.screen),
			BlackPixel(x.dpy, x.screen));

	if (!(x.gc = XCreateGC(x.dpy, x.win, 0, &values)))
		errx(1, "XCreateGC");
		/* NOTREACHED */

	if (!(rc = XStringListToTextProperty(&win_name, 1, &win_name_prop)))
		errx(1, "XStringListToTextProperty");
		/* NOTREACHED */

	XSetWMName(x.dpy, x.win, &win_name_prop);

	XMapWindow(x.dpy, x.win);

	XFlush(x.dpy);
	XSync(x.dpy, False);
}

long
getcolor(const char *color)
{
	int rc;

	XColor tcolor;

	if (!(rc = XAllocNamedColor(x.dpy, x.win_colormap, color, &tcolor,
	&tcolor)))
		errx(1, "can't allocate %s", color);

	return tcolor.pixel;
}

void
handler(int sig)
{
	XCloseDisplay(x.dpy);

	exit(0);
	/* NOTREACHED */
}

void
usage(void)
{
	fprintf(stderr, "usage: %s %s\n", __progname,
		"[-display host:dpy] [-width]");
	exit(1);
}
