/*****************************************************************************/

/*
 *      ppuserinit.c  -- Parport access via Linux PPUSER device (init routines).
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

#include "ppuser.h"

/* ---------------------------------------------------------------------- */

extern int pp_user_fd;
extern const struct parport_ops parport_user_ops;
extern void parport_user_exit(void);

/* ---------------------------------------------------------------------- */

int parport_init_ppuser(const char *path)
{
	unsigned char d;

	if ((pp_user_fd = open(path, O_RDWR | O_NONBLOCK)) == -1) {
		lprintf(0, "Cannot open ppuser device %s (error %s)\n", path,
			strerror(errno));
		return -1;
	}
	if (ioctl(pp_user_fd, PPCLAIM, 0)) {
		lprintf(0, "Cannot claim ppuser device %s (error %s)\n", path,
			strerror(errno));
		close(pp_user_fd);
		pp_user_fd = -1;
		return -1;
	}
	if (atexit(parport_user_exit)) {
		lprintf(0, "Cannot install atexit handler (error %s)\n", strerror(errno));
		ioctl(pp_user_fd, PPRELEASE, 0);
		close(pp_user_fd);
		pp_user_fd = -1;
		return -1;
	}
	d = 0x30;  /* set PS/2 mode */
	ioctl(pp_user_fd, PPWECONTROL, &d);
	parport_ops = parport_user_ops;
	return 0;
}
