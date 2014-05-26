/*****************************************************************************/

/*
 *      ppdevinit.c  -- Parport access via Linux PPDEV device (init routines).
 *
 *      Copyright (C) 1998-1999  Thomas Sailer (sailer@ife.ee.ethz.ch)
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
 *  Please note that the GPL allows you to use the driver, NOT the radio.
 *  In order to use the radio, you need a license from the communications
 *  authority of your country.
 *
 */

/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "parport.h"

#include "ppdev.h"

/* ---------------------------------------------------------------------- */

extern int pp_dev_fd;
extern int pp_dev_mode;
extern const struct parport_ops parport_dev_ops;
extern void parport_dev_exit(void);

/* ---------------------------------------------------------------------- */

int parport_init_ppdev(const char *path)
{
	int datadir = 0;

	if ((pp_dev_fd = open(path, O_RDWR | O_NONBLOCK)) == -1) {
		lprintf(0, "Cannot open ppdev device %s (error %s)\n", path,
			strerror(errno));
		return -1;
	}
	if (ioctl(pp_dev_fd, PPEXCL, 0)) {
		lprintf(0, "Cannot set exclusive mode on ppdev device %s (error %s)\n", path,
			strerror(errno));
		goto err;
	}
	if (ioctl(pp_dev_fd, PPCLAIM, 0)) {
		lprintf(0, "Cannot claim ppdev device %s (error %s)\n", path,
			strerror(errno));
		goto err;
	}
	pp_dev_mode = IEEE1284_MODE_COMPAT;
	if (ioctl(pp_dev_fd, PPSETMODE, &pp_dev_mode)) {
		lprintf(0, "Cannot set mode on ppdev device %s (error %s)\n", path,
			strerror(errno));
		goto err;
	}
	if (ioctl(pp_dev_fd, PPDATADIR, &datadir)) {
		lprintf(0, "Cannot set data direction on ppdev device %s (error %s)\n", path,
			strerror(errno));
		goto err;
	}	
	if (atexit(parport_dev_exit)) {
		lprintf(0, "Cannot install atexit handler (error %s)\n", strerror(errno));
		goto errrelease;
	}
	parport_ops = parport_dev_ops;
	return 0;

errrelease:
	ioctl(pp_dev_fd, PPRELEASE, 0);
err:
	close(pp_dev_fd);
	pp_dev_fd = -1;
	return -1;
}
