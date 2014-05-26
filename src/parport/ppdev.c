/*****************************************************************************/

/*
 *      ppdev.c  -- Parport access via Linux PPDEV device.
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

#include <sys/ioctl.h>
#include <unistd.h>

#include "parport.h"

#include "ppdev.h"

/* ---------------------------------------------------------------------- */

int pp_dev_fd = -1;
int pp_dev_mode;

/* ---------------------------------------------------------------------- */

void parport_dev_exit(void)
{
	if (pp_dev_fd != -1) {
		ioctl(pp_dev_fd, PPRELEASE, 0);
		close(pp_dev_fd);
	}
	pp_dev_fd = -1;
}

/* ---------------------------------------------------------------------- */

static void setmode(int newmode)
{
	int datadir = 0;

	if (newmode == pp_dev_mode)
		return;
	pp_dev_mode = newmode;
	ioctl(pp_dev_fd, PPSETMODE, &pp_dev_mode);
	if (pp_dev_mode == IEEE1284_MODE_COMPAT)
		ioctl(pp_dev_fd, PPDATADIR, &datadir);
}

/* ---------------------------------------------------------------------- */

unsigned char parport_dev_read_data(void)
{
	unsigned char r;
	
	ioctl(pp_dev_fd, PPRDATA, &r);
	return r;
}

void parport_dev_write_data(unsigned char d)
{
	setmode(IEEE1284_MODE_COMPAT);
	ioctl(pp_dev_fd, PPWDATA, &d);
}

unsigned char parport_dev_read_status(void)
{
	unsigned char r;
	
	ioctl(pp_dev_fd, PPRSTATUS, &r);
	return r;
}


unsigned char parport_dev_read_control(void)
{
	unsigned char r;
	
	ioctl(pp_dev_fd, PPRCONTROL, &r);
	return r;
}

void parport_dev_write_control(unsigned char d)
{
	ioctl(pp_dev_fd, PPWCONTROL, &d);
}

void parport_dev_frob_control(unsigned char mask, unsigned char val)
{
	struct ppdev_frob_struct f = { mask, val };

	ioctl(pp_dev_fd, PPFCONTROL, &f);
}

/* ---------------------------------------------------------------------- */

unsigned parport_dev_epp_write_data(const void *buf, unsigned sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_EPP | IEEE1284_DATA);
	ret = write(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

unsigned parport_dev_epp_read_data(void *buf, unsigned sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_EPP | IEEE1284_DATA);
	ret = read(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

unsigned parport_dev_epp_write_addr(const void *buf, unsigned sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
	ret = write(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

unsigned parport_dev_epp_read_addr(void *buf, unsigned sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_EPP | IEEE1284_ADDR);
	ret = read(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_dev_ecp_write_data(const void *buf, unsigned sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_ECP | IEEE1284_DATA);
	ret = write(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

unsigned parport_dev_ecp_write_addr(const void *buf, unsigned sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_ECP | IEEE1284_ADDR);
	ret = write(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

unsigned parport_dev_ecp_read_data(void *buf, unsigned sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_ECP | IEEE1284_DATA);
	ret = read(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_dev_fpgaconfig_write(const void *buf, unsigned sz)
{
#if 1
	const unsigned char *bp = buf;
	unsigned u;

	for (u = 0; u < sz; u++, bp++)
		parport_dev_write_data(*bp);
	return sz;
#else
	ssize_t ret;

	setmode(IEEE1284_MODE_COMPAT);
	ret = write(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
#endif
}

/* ---------------------------------------------------------------------- */

unsigned int parport_dev_compat_write(const void *buf, unsigned int sz)
{
	ssize_t ret;

	setmode(IEEE1284_MODE_COMPAT);
	ret = write(pp_dev_fd, buf, sz);
	if (ret < 0)
		return 0;
	return ret;
}

/* ---------------------------------------------------------------------- */

const struct parport_ops parport_dev_ops = {
	parport_dev_read_data,
	parport_dev_write_data,
	parport_dev_read_status,
	parport_dev_read_control,
	parport_dev_write_control,
	parport_dev_frob_control,
	parport_dev_epp_write_data,
	parport_dev_epp_read_data,
	parport_dev_epp_write_addr,
	parport_dev_epp_read_addr,
	parport_dev_ecp_write_data,
	parport_dev_ecp_read_data,
	parport_dev_ecp_write_addr,
	parport_dev_fpgaconfig_write,
	parport_dev_compat_write
};

/* ---------------------------------------------------------------------- */
