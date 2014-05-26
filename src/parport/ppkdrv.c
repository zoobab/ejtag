/*****************************************************************************/

/*
 *      ppkdrv.c  -- Parport access via Linux kernel driver.
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
#include <linux/socket.h>
#include <net/if.h>

#include "parport.h"

/* ---------------------------------------------------------------------- */

int pp_kdrv_fd = -1;
struct ifreq pp_kdrv_ifr;

/* ---------------------------------------------------------------------- */

void parport_kdrv_exit(void)
{
	if (pp_kdrv_fd != -1)
		close(pp_kdrv_fd);
	pp_kdrv_fd = -1;
}

/* ---------------------------------------------------------------------- */

struct eppflex_ioctl_params {
	int cmd;
	int mode;
	union {
		unsigned char d;
		struct {
			unsigned size;
			unsigned char *data;
		} blk;
	} u;
};

#define EPPFLEX_PPRSTATUS       _IOR(0xbc, 0x81, struct eppflex_ioctl_params)
#define EPPFLEX_PPRCONTROL      _IOR(0xbc, 0x83, struct eppflex_ioctl_params)
#define EPPFLEX_PPWCONTROL      _IOW(0xbc, 0x84, struct eppflex_ioctl_params)
#define EPPFLEX_PPRDATA         _IOR(0xbc, 0x85, struct eppflex_ioctl_params)
#define EPPFLEX_PPWDATA         _IOW(0xbc, 0x86, struct eppflex_ioctl_params)
#define EPPFLEX_PPWFPGACFG      _IOW(0xbc, 0x87, struct eppflex_ioctl_params)
#define EPPFLEX_PPREPPDATA      _IOR(0xbc, 0x88, struct eppflex_ioctl_params)
#define EPPFLEX_PPWEPPDATA      _IOW(0xbc, 0x89, struct eppflex_ioctl_params)
#define EPPFLEX_PPREPPADDR      _IOR(0xbc, 0x8a, struct eppflex_ioctl_params)
#define EPPFLEX_PPWEPPADDR      _IOW(0xbc, 0x8b, struct eppflex_ioctl_params)
#define EPPFLEX_PPRECPDATA      _IOR(0xbc, 0x8c, struct eppflex_ioctl_params)
#define EPPFLEX_PPWECPDATA      _IOW(0xbc, 0x8d, struct eppflex_ioctl_params)
#define EPPFLEX_PPWECPCMD       _IOW(0xbc, 0x8e, struct eppflex_ioctl_params)
#define EPPFLEX_PPCHANGEMODE    _IOW(0xbc, 0x8f, struct eppflex_ioctl_params)

/* ---------------------------------------------------------------------- */

unsigned char parport_kdrv_read_data(void)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPRDATA, };

	ifr.ifr_data = (caddr_t)&i;
	ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr);
	return i.u.d;
}

void parport_kdrv_write_data(unsigned char d)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPWDATA, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.d = d;
	ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr);
}

unsigned char parport_kdrv_read_status(void)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPRSTATUS, };

	ifr.ifr_data = (caddr_t)&i;
	ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr);
	return i.u.d;
}


unsigned char parport_kdrv_read_control(void)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPRCONTROL, };

	ifr.ifr_data = (caddr_t)&i;
	ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr);
	return i.u.d;
}

void parport_kdrv_write_control(unsigned char d)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPWCONTROL, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.d = d;
	ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr);
}

void parport_kdrv_frob_control(unsigned char mask, unsigned char val)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPRCONTROL, };
	
	ifr.ifr_data = (caddr_t)&i;
	ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr);
	i.u.d = (i.u.d & (~mask)) ^ val;
	i.cmd = EPPFLEX_PPWCONTROL;
	ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr);
}

/* ---------------------------------------------------------------------- */

unsigned parport_kdrv_epp_write_data(const void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPWEPPDATA, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

unsigned parport_kdrv_epp_read_data(void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPREPPDATA, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

unsigned parport_kdrv_epp_write_addr(const void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPWEPPADDR, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

unsigned parport_kdrv_epp_read_addr(void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPREPPADDR, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

/* ---------------------------------------------------------------------- */

unsigned parport_kdrv_ecp_write_data(const void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPWECPDATA, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

unsigned parport_kdrv_ecp_write_addr(const void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPWECPCMD, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

unsigned parport_kdrv_ecp_read_data(void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPRECPDATA, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

/* ---------------------------------------------------------------------- */
#if 0

unsigned parport_kdrv_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned u;

	for (u = 0; u < sz; u++, bp++)
		parport_kdrv_write_data(*bp);
	return sz;
}

#else

unsigned parport_kdrv_fpgaconfig_write(const void *buf, unsigned sz)
{
	struct ifreq ifr = pp_kdrv_ifr;
	struct eppflex_ioctl_params i = { EPPFLEX_PPWFPGACFG, };

	ifr.ifr_data = (caddr_t)&i;
	i.u.blk.size = sz;
	i.u.blk.data = (void *)buf;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr))
		return 0;
	return sz;
}

#endif

/* ---------------------------------------------------------------------- */

const struct parport_ops parport_kdrv_ops = {
	parport_kdrv_read_data,
	parport_kdrv_write_data,
	parport_kdrv_read_status,
	parport_kdrv_read_control,
	parport_kdrv_write_control,
	parport_kdrv_frob_control,
	parport_kdrv_epp_write_data,
	parport_kdrv_epp_read_data,
	parport_kdrv_epp_write_addr,
	parport_kdrv_epp_read_addr,
	parport_kdrv_ecp_write_data,
	parport_kdrv_ecp_read_data,
	parport_kdrv_ecp_write_addr,
	parport_kdrv_fpgaconfig_write
};

/* ---------------------------------------------------------------------- */
