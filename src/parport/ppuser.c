/*****************************************************************************/

/*
 *      ppuser.c  -- Parport access via Linux PPUSER device.
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

#include "ppuser.h"

/* ---------------------------------------------------------------------- */

int pp_user_fd = -1;

/* ---------------------------------------------------------------------- */

void parport_user_exit(void)
{
	if (pp_user_fd != -1) {
		ioctl(pp_user_fd, PPRELEASE, 0);
		close(pp_user_fd);
	}
	pp_user_fd = -1;
}

/* ---------------------------------------------------------------------- */

unsigned char parport_user_read_data(void)
{
	unsigned char r;
	
	ioctl(pp_user_fd, PPRDATA, &r);
	return r;
}

void parport_user_write_data(unsigned char d)
{
	ioctl(pp_user_fd, PPWDATA, &d);
}

unsigned char parport_user_read_status(void)
{
	unsigned char r;
	
	ioctl(pp_user_fd, PPRSTATUS, &r);
	return r;
}


unsigned char parport_user_read_control(void)
{
	unsigned char r;
	
	ioctl(pp_user_fd, PPRCONTROL, &r);
	return r;
}

void parport_user_write_control(unsigned char d)
{
	ioctl(pp_user_fd, PPWCONTROL, &d);
}

void parport_user_frob_control(unsigned char mask, unsigned char val)
{
	unsigned char d;
	
	ioctl(pp_user_fd, PPRCONTROL, &d);
	d = (d & (~mask)) ^ val;
	ioctl(pp_user_fd, PPWCONTROL, &d);
}

/* ---------------------------------------------------------------------- */

unsigned parport_user_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;
	unsigned char d;

	d = LPTCTRL_PROGRAM | LPTCTRL_WRITE;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	for (; sz > 0; sz--, bp++) {
		ioctl(pp_user_fd, PPWDATA, bp);
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (d & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		d = LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_DATASTB;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (!(d & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		d = LPTCTRL_PROGRAM | LPTCTRL_WRITE;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		ret++;
	}
	return ret;
}

unsigned parport_user_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;
	unsigned char d;

	d = LPTCTRL_PROGRAM | LPTCTRL_DIRECTION;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (d & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		d = LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_DATASTB;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (!(d & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		ioctl(pp_user_fd, PPRDATA, bp);
		d = LPTCTRL_PROGRAM | LPTCTRL_DIRECTION;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		ret++;
	}
	return ret;
}

unsigned parport_user_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;
	unsigned char d;

	d = LPTCTRL_PROGRAM | LPTCTRL_WRITE;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	for (; sz > 0; sz--, bp++) {
		ioctl(pp_user_fd, PPWDATA, bp);
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (d & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		d = LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_ADDRSTB;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (!(d & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		d = LPTCTRL_PROGRAM | LPTCTRL_WRITE;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		ret++;
	}
	return ret;
}

unsigned parport_user_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;
	unsigned char d;

	d = LPTCTRL_PROGRAM | LPTCTRL_DIRECTION;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (d & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		d = LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_ADDRSTB;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		for (tmo = 0; ; tmo++) {
			ioctl(pp_user_fd, PPRSTATUS, &d);
			if (!(d & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		ioctl(pp_user_fd, PPRDATA, bp);
		d = LPTCTRL_PROGRAM | LPTCTRL_DIRECTION;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		ret++;
	}
	return ret;
}

/* ---------------------------------------------------------------------- */

static int ecp_forward(void)
{
	unsigned tmo = 0x10000;
	unsigned char d;

	ioctl(pp_user_fd, PPRSTATUS, &d);
	if (d & 0x20)
		return 0;
	/* Event 47: Set nInit high */
	d = 0x26;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	/* Event 49: PError goes high */
	for (;;) {
		ioctl(pp_user_fd, PPRSTATUS, &d);
		if (d & 0x20)
			break;
		if (!(--tmo))
			return -1;
	}
	/* start driving the bus */
	d = 0x04;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	return 0;
}

static int ecp_reverse(void)
{
	unsigned tmo = 0x10000;
	unsigned char d;

	ioctl(pp_user_fd, PPRSTATUS, &d);
	if (!(d & 0x20))
		return 0;
	d = 0x24;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	/* Event 39: Set nInit low to initiate bus reversal */
	d = 0x20;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	for (;;) {
		ioctl(pp_user_fd, PPRSTATUS, &d);
		if (!(d & 0x20))
			break;
		if (!(--tmo))
			return -1;
	}
	return 0;
}

unsigned parport_user_ecp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl, d;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ioctl(pp_user_fd, PPRCONTROL, &ctl);
	ctl &= ~PARPORT_CONTROL_AUTOFD;
	/* HostAck high (data, not command) */
	ioctl(pp_user_fd, PPWCONTROL, &ctl);
	for (ret = 0; ret < sz; ret++, bp++) {
		ioctl(pp_user_fd, PPWDATA, bp);
		d = ctl | PARPORT_CONTROL_STROBE;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		tmo = 0;
		do {
			if (tmo > 0x1000)
				return ret;
			tmo++;
			ioctl(pp_user_fd, PPRSTATUS, &d);
		} while (d & PARPORT_STATUS_BUSY);
		ioctl(pp_user_fd, PPWCONTROL, &ctl);
		tmo = 0;
		do {
			if (tmo > 0x1000)
				return ret;
			tmo++;
			ioctl(pp_user_fd, PPRSTATUS, &d);
		} while (!(d & PARPORT_STATUS_BUSY));
	}
	return ret;
}

unsigned parport_user_ecp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl, d;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ioctl(pp_user_fd, PPRCONTROL, &ctl);
	ctl |= PARPORT_CONTROL_AUTOFD;
	/* HostAck low (command, not data) */
	ioctl(pp_user_fd, PPWCONTROL, &ctl);
	for (ret = 0; ret < sz; ret++, bp++) {
		ioctl(pp_user_fd, PPWDATA, bp);
		d = ctl | PARPORT_CONTROL_STROBE;
		ioctl(pp_user_fd, PPWCONTROL, &d);
		tmo = 0;
		do {
			if (tmo > 0x1000)
				return ret;
			tmo++;
			ioctl(pp_user_fd, PPRSTATUS, &d);
		} while (d & PARPORT_STATUS_BUSY);
		ioctl(pp_user_fd, PPWCONTROL, &ctl);
		tmo = 0;
		do {
			if (tmo > 0x1000)
				return ret;
			tmo++;
			ioctl(pp_user_fd, PPRSTATUS, &d);
		} while (!(d & PARPORT_STATUS_BUSY));
	}
	return ret;
}

unsigned parport_user_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned char ctl, stat;
	int command;
	unsigned tmo;
	unsigned char d;

	if (ecp_reverse())
		return 0;
	ioctl(pp_user_fd, PPRCONTROL, &ctl);
	/* Set HostAck low to start accepting data. */
	d = ctl | PARPORT_CONTROL_AUTOFD;
	ioctl(pp_user_fd, PPWCONTROL, &d);
	while (ret < sz) {
		/* Event 43: Peripheral sets nAck low. It can take as
                   long as it wants. */
		tmo = 0;
		do {
			ioctl(pp_user_fd, PPRSTATUS, &stat);
			if ((++tmo) > 0x10000)
				goto out;
		} while (stat & PARPORT_STATUS_ACK);

		/* Is this a command? */
		command = stat & PARPORT_STATUS_BUSY;
		if (command) {
			//lprintf(3, "ECP: warning: emulation does not support RLE\n");
			goto out;
		}

		/* Read the data. */
		ioctl(pp_user_fd, PPRDATA, bp);

		/* Event 44: Set HostAck high, acknowledging handshake. */
		d = ctl & ~PARPORT_CONTROL_AUTOFD;
		ioctl(pp_user_fd, PPWCONTROL, &d);

		/* Event 45: The peripheral has 35ms to set nAck high. */
		tmo = 0;
		do {
			ioctl(pp_user_fd, PPRSTATUS, &stat);
			if ((++tmo) > 0x10000)
				goto out;
		} while (!(stat & PARPORT_STATUS_ACK));

		/* Event 46: Set HostAck low and accept the data. */
		d = ctl | PARPORT_CONTROL_AUTOFD;
		ioctl(pp_user_fd, PPWCONTROL, &d);

		/* Normal data byte. */
		bp++;
		ret++;
	}

 out:
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_user_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned u;

	for (u = 0; u < sz; u++, bp++)
		parport_user_write_data(*bp);
	return sz;
}

/* ---------------------------------------------------------------------- */

const struct parport_ops parport_user_ops = {
	parport_user_read_data,
	parport_user_write_data,
	parport_user_read_status,
	parport_user_read_control,
	parport_user_write_control,
	parport_user_frob_control,
	parport_user_epp_write_data,
	parport_user_epp_read_data,
	parport_user_epp_write_addr,
	parport_user_epp_read_addr,
	parport_user_ecp_write_data,
	parport_user_ecp_read_data,
	parport_user_ecp_write_addr,
	parport_user_fpgaconfig_write
};

/* ---------------------------------------------------------------------- */
