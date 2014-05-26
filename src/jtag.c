/*****************************************************************************/

/*
 *      jtag.c  --  EJTAG routines.
 *
 *      Copyright (C) 1998-2000, 2005  Thomas Sailer (t.sailer@alumni.ethz.ch)
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
#include "util.h"

/* ---------------------------------------------------------------------- */

extern inline void jtag_reset_tap(void)
{
        jtag_shiftout(5, 0, ~0);
}

/* ---------------------------------------------------------------------- */

/*
 * this routine expects the target TAP to be in SELECTDR
 * state and leaves it there at exit 
 */

static void jtag_set_ir(unsigned int ir)
{
	ir &= ((1<<(EJTAG_IRLENGTH))-1);
	jtag_shiftout(5+EJTAG_IRLENGTH, ir << 3, 1 | (7 << ((EJTAG_IRLENGTH)+2)));
}

static uint32_t jtag_rw_dr(uint32_t d)
{
	jtag_shiftout(2, 0, 0);
	d = jtag_shift(32, d, 1 << 31);
	jtag_shiftout(2, 0, 3);
	return d;
}


/* ---------------------------------------------------------------------- */

/*
 * this routine expects the target TAP to be in RUNTEST/IDLE
 * state and leaves it there at exit 
 */

static void boundary(unsigned int blen, const unsigned char *in, unsigned char *out)
{
	jtag_shiftout(3, 0, 1);
	while (blen > 8) {
		*out++ = jtag_shift(8, *in++, 0);
		blen -= 8;
	}
	*out = jtag_shift(blen, *in, 1 << (blen-1));
	jtag_shiftout(2, 0, 1);
}

/* ---------------------------------------------------------------------- */

int detect_cpu(void)
{
	unsigned int i;
	uint32_t r;

	/* FPGA held in configuration state due to INITBIAS low */
	jtag_reset_tap(); /* force TAP controller into reset state */
	/* check instruction register length */
	jtag_shiftout(5, 0, 6);   /* enter Shift-IR state */
	jtag_shiftout(32, 0, 0);  /* assume max. 32bit IR */
	i = jtag_shift(32, 1, 0); /* shift in a single bit */
	if (hweight32(i) != 1) {
		lprintf(0, "unable to detect JTAG IR (val %#06x)\n", i);
		return -1;
	}
	if (ffs(i) != EJTAG_IRLENGTH+1) {
		lprintf(0, "size of JTAG IR not %d bits (val %#06x)\n", EJTAG_IRLENGTH, i);
		return -1;
	}
	/* reset TAP */
	jtag_reset_tap();
	/* move to SELECTDR state */
	jtag_shiftout(2, 0, 2);
	/* check CPU */
	jtag_set_ir(EJTAG_IR_IDCODE);
	r = jtag_rw_dr(0);
	printf("CPU IDCode: 0x%08x\n"
	       "  Version: 0x%x\n"
	       "  PNum: 0x%x\n"
	       "  ManID: 0x%x\n", r, (r >> 28) & 0xf, (r >> 12) & 0xffff, (r >> 1) & 0x7ff);
	jtag_set_ir(EJTAG_IR_IMPCODE);
	r = jtag_rw_dr(0);
	printf("CPU IMPCode: 0x%08x\n", r);
	return 0;
}

/* ---------------------------------------------------------------------- */

void release_cpu(void)
{
	jtag_reset_tap();
}

/* ---------------------------------------------------------------------- */

void reset_cpu(void)
{
	uint32_t r;

	jtag_set_ir(EJTAG_IR_CONTROL);
	r = jtag_rw_dr(0x00010000);
	printf("ECR: 0x%08x\n", r);
	r = jtag_rw_dr(0x00000000);
	printf("ECR: 0x%08x\n", r);
	r = jtag_rw_dr(0x00000000);
	printf("ECR: 0x%08x\n", r);
}

/* ---------------------------------------------------------------------- */

#define SRVDEBUGJTAG      0
#define SRVDEBUGCPUACCESS 1

#if defined(SRVDEBUGJTAG) && SRVDEBUGJTAG
#define srvdbgjtag(x) x
#else
#define srvdbgjtag(x) do { } while (0)
#endif

#if defined(SRVDEBUGCPUACCESS) && SRVDEBUGCPUACCESS
#define srvdbgcpuacc(x) x
#else
#define srvdbgcpuacc(x) do { } while (0)
#endif


int cpu_debug_server(void *memarea)
{
	static const char *accwidth[4] = { "BYTE", "HALFWORD", "WORD", "TRIPLE" };
	/* indexes: first: 0=BE, 1=LE second: PSZ third: Address low bits */
	static const uint32_t writemask[2][4][4] = {
		{
			/* Big Endian */
			{ 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
			{ 0xffff0000, 0, 0x0000ffff, 0 },
			{ 0xffffffff, 0, 0, 0 },
			{ 0xffffff00, 0x00ffffff, 0, 0 }
		}, {
			/* Little Endian */
			{ 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
			{ 0x0000ffff, 0, 0xffff0000, 0 },
			{ 0xffffffff, 0, 0, 0 },
			{ 0x00ffffff, 0xffffff00, 0, 0 }
		}
	};
	uint32_t ecr = 0x0000C000 | 0x00010000 /* reset */, data = 0, addr = 0, addrhi = 0;
	uint32_t *memptr, mask;
	unsigned int psz, pollcount = 256;

	jtag_set_ir(EJTAG_IR_EJTAGBOOT);
	jtag_set_ir(EJTAG_IR_ALL);
	while (!terminate()) {
                ecr |=  0x00040000;
		if (pollcount > 0)
			pollcount--;
		else
			usleep(10000);
		srvdbgjtag(printf("W ECR 0x%08x D 0x%08x A 0x%01x%08x\n", ecr, data, addrhi, addr));
		jtag_shiftout(2, 0, 0);
		ecr = jtag_shift(32, ecr, 0);
		data = jtag_shift(32, data, 0);
		addr = jtag_shift(32, addr, 0);
		addrhi = jtag_shift(4, addrhi, 1 << 3) & 0x0f;
		jtag_shiftout(2, 0, 3);
		srvdbgjtag(printf("R ECR 0x%08x D 0x%08x A 0x%01x%08x\n", ecr, data, addrhi, addr));
		ecr &= ~0x9FB33FF7;
                ecr |=  0x0000C000;
		switch (ecr & 0x000C0000) {
		case 0x00040000:  /* read */
			psz = (ecr >> 29) & 3;
			if (addr >= 0xff200000 && addr <= 0xff3fffff && memarea) {
				memptr = memarea;
				memptr += (addr - 0xff200000) >> 2;
				data = *memptr;
			} else
				/* out of debug address range or mem area not populated */
				data = 0;
			srvdbgcpuacc(printf("CPU Read Access: A 0x%01x%08x D 0x%08x %s\n", addrhi, addr, data, accwidth[psz]));
			/* reset pending bit */
			goto reset_pending;

		case 0x000C0000:  /* write */
			psz = (ecr >> 29) & 3;
			srvdbgcpuacc(printf("CPU Write Access: A 0x%01x%08x D 0x%08x %s\n", addrhi, addr, data, accwidth[psz]));
			if (addr >= 0xff200000 && addr <= 0xff3fffff && memarea) {
				memptr = memarea;
				memptr += (addr - 0xff200000) >> 2;
				mask = writemask[0][psz][addr & 3];
				*memptr = (*memptr & ~mask) | (data & mask);
				if (!mask) {
					srvdbgcpuacc(printf("CPU Write WARNING: invalid byte select\n"));
				}
			}
			/* reset pending bit */
		  reset_pending:
			ecr &= ~0x9FB73FF7;
			ecr |=  0x0000C000;
			srvdbgjtag(printf("W ECR 0x%08x D 0x%08x A 0x%01x%08x\n", ecr, data, addrhi, addr));
			jtag_shiftout(2, 0, 0);
			ecr = jtag_shift(32, ecr, 0);
			data = jtag_shift(32, data, 0);
			addr = jtag_shift(32, addr, 0);
			addrhi = jtag_shift(4, addrhi, 1 << 3) & 0x0f;
			jtag_shiftout(2, 0, 3);
			srvdbgjtag(printf("R ECR 0x%08x D 0x%08x A 0x%01x%08x\n", ecr, data, addrhi, addr));
			pollcount = 256;
			break;
		}
		ecr &= ~0x9FB33FF7;
                ecr |=  0x0000C000;
	}
	jtag_set_ir(EJTAG_IR_NORMALBOOT);
	return 0;
}
