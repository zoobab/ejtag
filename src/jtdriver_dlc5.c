/*****************************************************************************/

/*
 *      jtdriver.c  --  JTAG simple cable driver.
 *
 *      Copyright (C) 1998-2005  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *      Copyright (C) 2005  Abhijit Bhopatkar (bainonline@gmail.com)
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ejtag.h"
#include "parport.h"
#include "util.h"

/* ---------------------------------------------------------------------- */
/*                       Xilix DLC5 III cable                             */

/* data reg */
#define DLC5_LPDATA_SHIFT_TDI  0
#define DLC5_LPDATA_SHIFT_TCK  1
#define DLC5_LPDATA_SHIFT_TMS  2
#define DLC5_LPDATA_SHIFT_CTR  3
#define DLC5_LPDATA_SHIFT_PROG 4

#define DLC5_LPDATA_TDI       (1<<(DLC5_LPDATA_SHIFT_TDI))
#define DLC5_LPDATA_TCK       (1<<(DLC5_LPDATA_SHIFT_TCK))
#define DLC5_LPDATA_TMS	       (1<<(DLC5_LPDATA_SHIFT_TMS))
#define DLC5_LPDATA_PROG       (1<<(DLC5_LPDATA_SHIFT_PROG))

/* status reg */
#define DLC5_LPSTAT_SHIFT_TDO  4
#define DLC5_LPSTAT_TDO       (1<<(DLC5_LPSTAT_SHIFT_TDO))

/* ---------------------------------------------------------------------- */

static void dlc5_jt_shiftout(unsigned int num, uint32_t tdi, uint32_t tms)
{
	uint8_t v;

	v = ( DLC5_LPDATA_PROG | (tdi << DLC5_LPDATA_SHIFT_TDI) & DLC5_LPDATA_TDI) | ((tms << DLC5_LPDATA_SHIFT_TMS) & DLC5_LPDATA_TMS);
	parport_write_data(v);
	while (num > 0) {
		parport_write_data(v | DLC5_LPDATA_TCK);
		tdi >>= 1;
		tms >>= 1;
		v = DLC5_LPDATA_PROG | ((tdi << DLC5_LPDATA_SHIFT_TDI) & DLC5_LPDATA_TDI) | ((tms << DLC5_LPDATA_SHIFT_TMS) & DLC5_LPDATA_TMS);
		parport_write_data(v);
		num--;
	}
}

static uint32_t dlc5_jt_shift(unsigned int num, uint32_t tdi, uint32_t tms)
{
	uint8_t v;
	uint32_t in = 0, mask = 2;

	v = DLC5_LPDATA_PROG | ((tdi << DLC5_LPDATA_SHIFT_TDI) & DLC5_LPDATA_TDI) | ((tms << DLC5_LPDATA_SHIFT_TMS) & DLC5_LPDATA_TMS);
	parport_write_data(v);
	if (parport_read_status() & DLC5_LPSTAT_TDO)
		in |= 1;
	while (num > 0) {
		parport_write_data(v | DLC5_LPDATA_TCK);
		tdi >>= 1;
		tms >>= 1;
		v = DLC5_LPDATA_PROG | ((tdi << DLC5_LPDATA_SHIFT_TDI) & DLC5_LPDATA_TDI) | ((tms << DLC5_LPDATA_SHIFT_TMS) & DLC5_LPDATA_TMS);
		parport_write_data(v);
		num--;
		if (parport_read_status() & DLC5_LPSTAT_TDO)
			in |= mask;
		mask <<= 1;
	}
	return in;
}

/* ---------------------------------------------------------------------- */

static void dlc5_jt_close(void)
{
	parport_write_data(0);
	parport_write_control(PARPORT_CONTROL_STROBE);	
}

int dlc5_jtag_simple_open(const struct parport_params *pp)
{
        uint8_t b;

#ifdef HAVE_PPUSER
        if (pp->ppkdrv) {
                if (parport_init_ppkdrv(pp->ppkdrv)) {
                        fprintf(stderr, "no kernel interface %s driver found\n", pp->ppkdrv);
                        return -1;
                }
        } else
#endif
#ifdef HAVE_PPUSER
                if (pp->ppdev) {
                        if (parport_init_ppdev(pp->ppdev)) {
                                fprintf(stderr, "no ppdev driver found at %s\n", pp->ppdev);
                                return -1;
                        }
                } else if (pp->ppuser) {
                        if (parport_init_ppuser(pp->ppuser)) {
                                fprintf(stderr, "no ppuser driver found at %s\n", pp->ppuser);
                                return -1;
                        }
                } else
#endif
#ifdef WIN32
                        if (pp->ntdrv) {
                                if (parport_init_win_flags(pp->ntdrv-1, pp->ppflags)) {
                                        fprintf(stderr, "no eppflex.sys/vxd driver found\n");
                                        return -1;
                                }
                        } else if (pp->ntddkgenport) {
                                if (parport_init_ntddkgenport()) {
                                        fprintf(stderr, "no NTDDK genport.sys driver found\n");
                                        return -1;
                                }
                        } else if (pp->w9xring0) {
                                if (parport_init_xw9xring0_flags(pp->iobase, pp->ppflags)) {
                                        fprintf(stderr, "no parport found at 0x%x\n", pp->iobase);
                                        return -1;
                                }
                        } else
#endif
                                if (parport_init_direct_flags(pp->iobase, pp->ppflags)) {
                                        fprintf(stderr, "no parport found at 0x%x\n", pp->iobase);
                                        return -1;
                                }
	jtag_driver.close = dlc5_jt_close;
	jtag_driver.shift = dlc5_jt_shift;
	jtag_driver.shiftout = dlc5_jt_shiftout;

#if 0
	/* test cable */
        parport_write_control(PARPORT_CONTROL_STROBE | PARPORT_CONTROL_DIRECTION);
        b = parport_read_status();
        if (!(b & PARPORT_STATUS_BUSY)) {
                fprintf(stderr, "Parport JTAG Cable not connected (BUSY high)\n");
                goto errout;
        }
        if (b & PARPORT_STATUS_ACK) {
                fprintf(stderr, "Parport JTAG Cable not connected (ACK stuck high)\n");
                goto errout;
        }
        parport_write_control(PARPORT_CONTROL_DIRECTION);
        b = parport_read_status();
	/* if (!(b & PARPORT_STATUS_ACK)) {
                fprintf(stderr, "Parport JTAG Cable not connected (ACK stuck low)\n");
                goto errout;
		}*/
        if (!(b & PARPORT_STATUS_SELECT)) {
                fprintf(stderr, "Target not powered (VIO low)\n");
                goto errout;
        }
	parport_write_data(0);
	/* release RST (not connected on the Pb1000 anyway */
	parport_write_control(PARPORT_CONTROL_STROBE | PARPORT_CONTROL_INIT);
	/* release TAP reset */
	parport_write_data(LPTDATA_TRST);
#endif

	return 0;

  errout:
	jtag_close();
	return -1;
}

/* ---------------------------------------------------------------------- */
