/*****************************************************************************/

/*
 *      ppkdrvinit.c  -- Parport access via Linux kernel driver (init routines).
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

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/socket.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "parport.h"

/* ---------------------------------------------------------------------- */

extern int pp_kdrv_fd;
extern struct ifreq pp_kdrv_ifr;
extern const struct parport_ops parport_kdrv_ops;
extern void parport_kdrv_exit(void);

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

int parport_init_ppkdrv(const char *ifname)
{
	struct ifreq ifr;
	struct eppflex_ioctl_params i;

	strncpy(pp_kdrv_ifr.ifr_name, ifname, sizeof(pp_kdrv_ifr.ifr_name));
	if ((pp_kdrv_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_AX25))) == -1) {
		lprintf(0, "Cannot open socket (error %s)\n", strerror(errno));
		return -1;
	}
	if (ioctl(pp_kdrv_fd, SIOCGIFINDEX, &pp_kdrv_ifr)) {
		lprintf(0, "Cannot open interface %s (error %s)\n", ifname, strerror(errno));
		close(pp_kdrv_fd);
		return -1;
	}
	ifr = pp_kdrv_ifr;
	i.cmd = EPPFLEX_PPRCONTROL;
	ifr.ifr_data = (caddr_t)&i;
	if (ioctl(pp_kdrv_fd, SIOCDEVPRIVATE, &ifr)) {
		lprintf(0, "Cannot access parport through kernel driver %s (error %s)\n", ifname,
			strerror(errno));
		close(pp_kdrv_fd);
		return -1;
	}
	if (atexit(parport_kdrv_exit)) {
		lprintf(0, "Cannot install atexit handler (error %s)\n", strerror(errno));
		close(pp_kdrv_fd);
		pp_kdrv_fd = -1;
		return -1;
	}
	parport_ops = parport_kdrv_ops;
	return 0;
}
