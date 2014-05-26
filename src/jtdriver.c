/*****************************************************************************/

/*
 *      jtdriver.c  --  JTAG simple cable driver.
 *
 *      Copyright (C) 1998-2005  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

/* LPT data register */
#define LPTDATA_SHIFT_TDI     4
#define LPTDATA_SHIFT_TMS     6
#define LPTDATA_TDI           (1<<(LPTDATA_SHIFT_TDI))
#define LPTDATA_TCK           0x80
#define LPTDATA_TMS           (1<<(LPTDATA_SHIFT_TMS))
#define LPTDATA_TRST          0x08

#define LPTSTAT_SHIFT_TDO     5
#define LPTSTAT_TDO           (1<<(LPTSTAT_SHIFT_TDO))

/* ---------------------------------------------------------------------- */

static void jt_shiftout(unsigned int num, uint32_t tdi, uint32_t tms)
{
	uint8_t v;

	v = LPTDATA_TRST | ((tdi << LPTDATA_SHIFT_TDI) & LPTDATA_TDI) | ((tms << LPTDATA_SHIFT_TMS) & LPTDATA_TMS);
	parport_write_data(v);
	while (num > 0) {
		parport_write_data(v | LPTDATA_TCK);
		tdi >>= 1;
		tms >>= 1;
		v = LPTDATA_TRST | ((tdi << LPTDATA_SHIFT_TDI) & LPTDATA_TDI) |	((tms << LPTDATA_SHIFT_TMS) & LPTDATA_TMS);
		parport_write_data(v);
		num--;
	}
}

static uint32_t jt_shift(unsigned int num, uint32_t tdi, uint32_t tms)
{
	uint8_t v;
	uint32_t in = 0, mask = 2;

	v = LPTDATA_TRST | ((tdi << LPTDATA_SHIFT_TDI) & LPTDATA_TDI) | ((tms << LPTDATA_SHIFT_TMS) & LPTDATA_TMS);
	parport_write_data(v);
	if (parport_read_status() & LPTSTAT_TDO)
		in |= 1;
	while (num > 0) {
		parport_write_data(v | LPTDATA_TCK);
		tdi >>= 1;
		tms >>= 1;
		v = LPTDATA_TRST | ((tdi << LPTDATA_SHIFT_TDI) & LPTDATA_TDI) | ((tms << LPTDATA_SHIFT_TMS) & LPTDATA_TMS);
		parport_write_data(v);
		num--;
		if (parport_read_status() & LPTSTAT_TDO)
			in |= mask;
		mask <<= 1;
	}
	return in;
}

/* ---------------------------------------------------------------------- */

static void jt_close(void)
{
	parport_write_data(0);
	parport_write_control(PARPORT_CONTROL_STROBE);	
}

static void jt_close_win(void)
{
	jt_close();
#ifdef WIN32
	parport_stop_win();
#endif
}

int jtag_simple_open(const struct parport_params *pp)
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
	jtag_driver.close = jt_close;
        if (!pp->ppkdrv && !pp->ppuser && !pp->ppdev && !pp->ntddkgenport && pp->ntdrv)
		jtag_driver.close = jt_close_win;
	jtag_driver.shift = jt_shift;
	jtag_driver.shiftout = jt_shiftout;
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
        if (!(b & PARPORT_STATUS_ACK)) {
                fprintf(stderr, "Parport JTAG Cable not connected (ACK stuck low)\n");
                goto errout;
        }
        if (!(b & PARPORT_STATUS_SELECT)) {
                fprintf(stderr, "Target not powered (VIO low)\n");
                goto errout;
        }
	parport_write_data(0);
	/* release RST (not connected on the Pb1000 anyway */
	parport_write_control(PARPORT_CONTROL_STROBE | PARPORT_CONTROL_INIT);
	/* release TAP reset */
	parport_write_data(LPTDATA_TRST);
	return 0;

  errout:
	jtag_close();
	return -1;
}

/* ---------------------------------------------------------------------- */
