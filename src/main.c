/*****************************************************************************/

/*
 *      main.c  --  EJTAG Tool.
 *
 *      Copyright (C) 2003, 2004, 2005  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include "ejtag.h"
#include "parport.h"

/* ---------------------------------------------------------------------- */

static unsigned int verboselevel = 0;
static unsigned int syslogmsg = 0;
static void *memarea = NULL;
int quit = 0;
struct jtag_driver jtag_driver;

/* ---------------------------------------------------------------------- */

void jtag_close(void)
{
	jtag_driver.close();
	memset(&jtag_driver, 0, sizeof(jtag_driver));
}

/* ---------------------------------------------------------------------- */

int lprintf(unsigned vl, const char *format, ...)
{
        va_list ap;
        int r;

        if (vl > verboselevel)
                return 0;
	va_start(ap, format);
#ifdef HAVE_VSYSLOG
        if (syslogmsg) {
		static const int logprio[] = { LOG_ERR, LOG_INFO };
		vsyslog((vl > 1) ? LOG_DEBUG : logprio[vl], format, ap);
		r = 0;
	} else
#endif
                r = vfprintf(stderr, format, ap);
	va_end(ap);
        return r;
}

/* ---------------------------------------------------------------------- */

static void signal_quit(int signum)
{
	quit = 1;
}

/* ---------------------------------------------------------------------- */

int action_reset(void)
{
	reset_cpu();
	return 0;
}

int action_debugsrv(void)
{
	return cpu_debug_server(memarea);
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
        static const struct option long_options[] = {
#ifdef HAVE_PPUSER
                { "ppuser", 1, 0, 0x400 },
                { "ppdev", 1, 0, 0x402 },
#endif
#ifdef HAVE_PPKDRV
                { "ppkdrv", 1, 0, 0x401 },
#endif
#ifdef WIN32
                { "ppgenport", 0, 0, 0x403 },
                { "ppwin", 1, 0, 0x404 },
                { "pplpt", 1, 0, 0x404 },
                { "ppring0", 0, 0, 0x405 },
#endif
                { "ppforcehwepp", 0, 0, 0x410 },
                { "ppswemulepp", 0, 0, 0x411 },
                { "ppswemulecp", 0, 0, 0x412 },
		{ "reset", 0, 0, 0x500 },
		{ "debug", 0, 0, 0x501 },
		{ "hex", 1, 0, 'r' },
		{ "dlc5", 0, 0, 0x601 },
		{ 0, 0, 0, 0 }
        };
	int c, err = 0;
	struct parport_params pp = {
		.iobase = 0x378,
		.ppdev = "/dev/parport0"
	};
	int (*action)(void) = action_reset;
	int (*jtag_open)(const struct parport_params *) = jtag_simple_open;

	printf(PACKAGE " v" VERSION " (c) 1998-2005 by Thomas Sailer, HB9JNX/AE4WA\n");
	/* allocate debug memory area */
	memarea = mmap(NULL, 0x200000, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (memarea == MAP_FAILED) {
		memarea = NULL;
		fprintf(stderr, "Cannot allocate debug memory\n");
	}
	while ((c = getopt_long(argc, argv, "svp:r:", long_options, NULL)) != EOF) {
		switch (c) {
#if defined(HAVE_SYSLOG) && defined(HAVE_VSYSLOG)
		case 's':
			if (syslogmsg)
				break;
			openlog("pppicprog", LOG_PID, LOG_USER);
			syslogmsg = 1;
			break;
#endif

		case 'v':
			verboselevel++;
			break;

                case 'p':
                        pp.iobase = strtoul(optarg, NULL, 0);
                        if (pp.iobase <= 0 || pp.iobase >= 0x3f8)
                                err++;
                        pp.ppuser = NULL;
                        pp.ppkdrv = NULL;
                        pp.ppdev = NULL;
                        break;

                case 0x400:
                        pp.ppuser = optarg;
                        pp.ppkdrv = NULL;
                        pp.ppdev = NULL;
                        pp.ntddkgenport = 0;
                        pp.ntdrv = 0;
                        pp.w9xring0 = 0;
                        break;

                case 0x401:
                        pp.ppkdrv = optarg;
                        pp.ppuser = NULL;
                        pp.ppdev = NULL;
                        pp.ntddkgenport = 0;
                        pp.ntdrv = 0;
                        pp.w9xring0 = 0;
                        break;

                case 0x402:
                        pp.ppkdrv = NULL;
                        pp.ppuser = NULL;
                        pp.ppdev = optarg;
                        pp.ntddkgenport = 0;
                        pp.ntdrv = 0;
                        pp.w9xring0 = 0;
                        break;

                case 0x403:
                        pp.ppkdrv = NULL;
                        pp.ppuser = NULL;
                        pp.ppdev = NULL;
                        pp.ntddkgenport = 1;
                        pp.ntdrv = 0;
                        pp.w9xring0 = 0;
                        break;

                case 0x404:
                        pp.ppkdrv = NULL;
                        pp.ppuser = NULL;
                        pp.ppdev = NULL;
                        pp.ntddkgenport = 0;
                        pp.ntdrv = strtoul(optarg, NULL, 0);
                        pp.w9xring0 = 0;
                        break;

                case 0x405:
                        pp.ppkdrv = NULL;
                        pp.ppuser = NULL;
                        pp.ppdev = NULL;
                        pp.ntddkgenport = 0;
                        pp.ntdrv = 0;
                        pp.w9xring0 = 1;
                        break;

                case 0x410:
                        pp.ppflags |= PPFLAG_FORCEHWEPP;
                        break;

                case 0x411:
                        pp.ppflags |= PPFLAG_SWEMULEPP;
                        break;

                case 0x412:
                        pp.ppflags |= PPFLAG_SWEMULECP;
                        break;

		case 0x500:
			action = action_reset;
			break;

		case 0x501:
			action = action_debugsrv;
			break;

		case 'r':
			if (read_hex_file(optarg, memarea))
				err++;
			break;
			
		case 0x601:
			jtag_open = dlc5_jtag_simple_open;
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		lprintf(0, "usage: %s [-v] [-s] [-p <ioaddr>]"
#ifdef HAVE_PPUSER
			" [--ppuser] [--ppdev]"
#endif
#ifdef HAVE_PPKDRV
			" [--ppkdrv]"
#endif
#ifdef WIN32
			" [--ppgenport] [--ppwin] [--pplpt] [--ppring0]"
#endif
			" [--ppforcehwepp] [--ppswemulepp] [--ppswemulecp]\n"
			"  [--reset] [--debug] [-r <hexfile>]"
			"\n", argv[0]);
		exit(1);
	}

#ifdef SIGHUP
	signal(SIGHUP, signal_quit);
#endif
	signal(SIGINT, signal_quit);
#ifdef SIGQUIT
	signal(SIGQUIT, signal_quit);
#endif
	signal(SIGTERM, signal_quit);

	c = jtag_open(&pp);
	if (c)
		return 1;
	c = detect_cpu();
	if (c)
		fprintf(stderr, "CPU not detected or invalid\n");
	else {
		c = action();
	}
	release_cpu();
	jtag_close();
	exit(!!c);
}
