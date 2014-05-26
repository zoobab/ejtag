/*****************************************************************************/

/*
 *      ejtag.h  --  EJTAG Tool.
 *
 *      Copyright (C) 2005  Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#ifndef EJTAG_H
#define EJTAG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include "parport.h"
#include "util.h"

/* ---------------------------------------------------------------------- */

struct parport_params {
	unsigned int iobase, ntddkgenport, ntdrv, w9xring0, ppflags;
	const char *ppuser, *ppkdrv, *ppdev;
};

struct jtag_driver {
	void (*close)(void);
	u_int32_t (*shift)(unsigned int num, u_int32_t tdi, u_int32_t tms);
	void (*shiftout)(unsigned int num, u_int32_t tdi, u_int32_t tms);
};

extern struct jtag_driver jtag_driver;

extern int quit;

extern inline int terminate(void)
{
	return quit;
}

extern void jtag_close(void);
extern int jtag_simple_open(const struct parport_params *pp);
extern int dlc5_jtag_simple_open(const struct parport_params *pp);

extern inline void jtag_shiftout(unsigned int num, u_int32_t tdi, u_int32_t tms)
{
	jtag_driver.shiftout(num, tdi, tms);
}

extern inline u_int32_t jtag_shift(unsigned int num, u_int32_t tdi, u_int32_t tms)
{
	return jtag_driver.shift(num, tdi, tms);
}

/* ---------------------------------------------------------------------- */

/*
 * AU1 defines
 */

#define EJTAG_IRLENGTH      5
#define EJTAG_IR_EXTEST     0x00
#define EJTAG_IR_IDCODE     0x01
#define EJTAG_IR_SAMPLE     0x02
#define EJTAG_IR_IMPCODE    0x03
#define EJTAG_IR_ADDRESS    0x08
#define EJTAG_IR_DATA       0x09
#define EJTAG_IR_CONTROL    0x0a
#define EJTAG_IR_ALL        0x0b
#define EJTAG_IR_EJTAGBOOT  0x0c
#define EJTAG_IR_NORMALBOOT 0x0d
#define EJTAG_IR_EJWATCH    0x1c
#define EJTAG_IR_BYPASS     0x1f

extern int detect_cpu(void);
extern void release_cpu(void);
extern void reset_cpu(void);
extern int cpu_debug_server(void *memarea);

/* ---------------------------------------------------------------------- */

extern int read_hex_file(const char *fn, void *mem);

/* ---------------------------------------------------------------------- */
#endif /* EJTAG_H */
