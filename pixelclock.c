/* vim:ts=8
 * $Id: pixelclock.c,v 1.6 2008/08/21 21:34:54 jcs Exp $
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

/* default hours to highlight (9am, noon, 5pm) */
const float defhours[3] = { 9.0, 12.0, 17.0 };

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
	int c, i, y;
	int hourtick, lastpos = -1, newpos = 0;
	struct timeval tv[2];
	time_t now;
	struct tm *t;

	float *hihours;
	int nhihours;

	XEvent event;

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

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		/* use default times */
		nhihours = sizeof(defhours) / sizeof(defhours[0]);
		if ((hihours = alloca(sizeof(defhours))) == NULL)
			err(1, NULL);

		for (i = 0; i < nhihours; i++)
			hihours[i] = defhours[i];
	} else {
		/* get times from args */
		nhihours = argc;
		if ((hihours = alloca(nhihours * sizeof(float))) == NULL)
			err(1, NULL);

		for (i = 0; i < argc; ++i) {
			int h, m;
			char *p = argv[i];

			/* parse times like 14:12 */
			h = atoi(p);
			if ((p = strchr(p, ':')) == NULL)
				errx(1, "Invalid time %s", argv[i]);
			m = atoi(p + 1);

			if (h > 23 || h < 0 || m > 59 || m < 0)
				errx(1, "Invalid time %s", argv[i]);

			hihours[i] = h + (m / 60.0);
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

		/* check if we just got exposed */
		bzero(&event, sizeof(XEvent));
		XCheckWindowEvent(x.dpy, x.win, ExposureMask, &event);

		/* only redraw if our time changed enough to move the box or if
		 * we were just exposed */
		if ((newpos != lastpos) || (event.type == Expose)) {
			XClearWindow(x.dpy, x.win);

			/* draw the current time */
			XSetForeground(x.dpy, x.gc, getcolor("yellow"));
			XFillRectangle(x.dpy, x.win, x.gc,
				0, newpos,
				x.width, 6);

			/* draw the hour ticks */
			XSetForeground(x.dpy, x.gc, getcolor("blue"));
			for (y = 1; y <= 23; y++) {
				XFillRectangle(x.dpy, x.win, x.gc,
					0, (y * hourtick),
					x.width, 2);
			}

			/* highlight requested times */
			XSetForeground(x.dpy, x.gc, getcolor("green"));
			for (i = 0; i < nhihours; i++) {
				XFillRectangle(x.dpy, x.win, x.gc,
					0, (hihours[i] * hourtick),
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
	XSetWindowAttributes attributes;
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

	if (!(rc = XStringListToTextProperty(&win_name, 1, &win_name_prop)))
		errx(1, "XStringListToTextProperty");
		/* NOTREACHED */

	XSetWMName(x.dpy, x.win, &win_name_prop);

	/* remove all window manager decorations and force our position/size */
	/* XXX: apparently this is not very nice */
	attributes.override_redirect = True;
	XChangeWindowAttributes(x.dpy, x.win, CWOverrideRedirect, &attributes);

	if (!(x.gc = XCreateGC(x.dpy, x.win, 0, &values)))
		errx(1, "XCreateGC");
		/* NOTREACHED */

	XMapWindow(x.dpy, x.win);

	/* we want to know when we're exposed */
	XSelectInput(x.dpy, x.win, ExposureMask);

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
		"[-display host:dpy] [-width <pixels>] [time time2 ...]");
	exit(1);
}
