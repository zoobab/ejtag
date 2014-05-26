/*****************************************************************************/

/*
 *      srec2s.c  --  Motorola S Record Reader.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "getopt.h"
#endif

/* ---------------------------------------------------------------------- */

static uint32_t entry_address = -1;
static uint32_t uboot_start_address = -1;
static uint32_t uboot_length = 0;
static uint32_t uboot[0x10000];
static unsigned int little_endian = 0;

/* ---------------------------------------------------------------------- */

static int hex2byte(const char *str)
{
	char buf[3], *bp;
	unsigned int r;

	if (!str)
		return -1;
	memcpy(buf, str, 2);
	buf[2] = 0;
	r = strtoul(buf, &bp, 16);
	if (bp != buf + 2)
		return -1;
	return r;
}

static int store(uint32_t addr, uint8_t data)
{
	uint32_t *memptr, mask, d;

	if (uboot_start_address == (uint32_t)-1)
		uboot_start_address = addr;
	if (uboot_start_address & 3) {
		fprintf(stderr, "start address not word aligned\n");
		return -1;
	}
	addr -= uboot_start_address;
	if (addr >= sizeof(uboot)) {
		fprintf(stderr, "array overflow\n");
		return -1;
	}
	if (addr > uboot_length)
		uboot_length = addr;
	if (!little_endian)
		addr ^= 3;  /* big endian */
	memptr = uboot;
	memptr += addr >> 2;
	d = data * 0x01010101;
	mask = 0xff << (8*(addr & 3));
	*memptr = (*memptr & ~mask) | (d & mask);
	return 0;
}

static int process_line(const char *line)
{
	uint8_t buf[257], sum = 0;
	unsigned int len, i;
	uint32_t addr;
	int r;

	if (!line || *line != 'S')
		goto invalidline;
	len = strlen(line);
	if (len < 6)
		goto invalidline;
	for (i = 0; i <= buf[0]; i++) {
		r = hex2byte(line + 2 + 2 * i);
		if (r < 0)
			goto invalidline;
		buf[i] = r;
	}
	for (i = 0; i <= buf[0]; i++)
		sum += buf[i];
	if (sum != 0xff) {
		fprintf(stderr, "invalid checksum 0x%02x in line: %s\n", sum, line);
		return -1;
	}
	switch (line[1]) {
	case '0': /* ignoring header */
	case '5': /* ignoring ?? */
	case '6': /* ignoring ?? */
		return 0;

	case '1':
		addr = (buf[1] << 8) | buf[2];
		for (i = 3; i < buf[0]; i++, addr++)
			if (store(addr, buf[i]))
				goto storeerr;
		return 0;

	case '2':
		addr = (buf[1] << 16) | (buf[2] << 8) | buf[3];
		for (i = 4; i < buf[0]; i++, addr++)
			if (store(addr, buf[i]))
				goto storeerr;
		return 0;

	case '3':
		addr = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
		for (i = 5; i < buf[0]; i++, addr++)
			if (store(addr, buf[i]))
				goto storeerr;
		return 0;

	case '7':
		entry_address = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
		return 0;

	case '8':
		entry_address = (buf[1] << 16) | (buf[2] << 8) | buf[3];
		return 0;

	case '9':
		entry_address = (buf[1] << 8) | buf[2];
		return 0;

	default:
		goto invalidline;
	}
  invalidline:
	fprintf(stderr, "invalid motorola hex line: %s\n", line);
	return -1;

  storeerr:
	fprintf(stderr, "error storing value 0x%02x at address 0x%08x\n", buf[i], addr);
	return -1;
}

/* ---------------------------------------------------------------------- */

#undef READBACK

int main(int argc, char *argv[])
{
        static const struct option long_options[] = {
		{ "little-endian", 0, 0, 'L' },
		{ "big-endian", 0, 0, 'B' },
		{ 0, 0, 0, 0 }
        };
	int c, err = 0;
	char buf[600];
	unsigned int i, j;
	uint32_t addroffset = 0;

	while ((c = getopt_long(argc, argv, "LBA:", long_options, NULL)) != EOF) {
		switch (c) {
		case 'B':
			little_endian = 0;
			break;

		case 'L':
			little_endian = 1;
			break;

		case 'A':
			addroffset = strtoul(optarg, NULL, 0) & ~3;
			break;

		default:
			err++;
			break;
		}
	}
	if (err) {
		fprintf(stderr, "usage: %s [--little-endian] [--big-endian]\n", argv[0]);
		exit(1);
	}
	memset(uboot, 0xff, sizeof(uboot));
	while (fgets(buf, sizeof(buf), stdin))
		if (process_line(buf)) {
			return 1;
		}
	if (entry_address & 3) {
		fprintf(stderr, "entry address not word aligned 0x%08x\n", entry_address);
		return 1;
	}
	uboot_start_address += addroffset;
	entry_address += addroffset;
	fprintf(stderr, "uboot: %cE, start 0x%08x, len %u, entry 0x%08x\n", 
		little_endian ? 'L' : 'B', uboot_start_address, uboot_length+1, entry_address);
	j = (uboot_length >> 2) + 1;
	printf("\t.section\tUBOOT,\"a\",@progbits\n"
	       "\t.align\t2\n"
	       "\t.globl\tubootcode\n"
	       "ubootcode:\n");
	for (i = 0; i < j; i++)
		printf("\t.word\t0x%08x\n", uboot[i]);
	printf("\t.section\tUCOPY,\"ax\",@progbits\n"
	       "\t.align\t2\n"
	       "\t.set\tnoreorder\n"
	       "\t.globl\tucopy\n"
	       "\t.ent\tucopy\n"
	       "\t.type\tucopy, @function\n"
	       "ucopy:\n"
	       "\tbal\t1f\n"
	       "\tnop\n"
	       "\t.word\tubootcode\n"
	       "\t.word\t0x%08x\n"
	       "\t.word\t0x%08x\n"
	       "1:\n"
	       "\tli\t$8, 0x%08x\n"
	       "\tmtc0\t$8, $24\n"
	       "\tlw\t$8, 0($31)\n"
	       "\tlw\t$9, 4($31)\n"
	       "\tlw\t$10, 8($31)\n"
	       "2:\n"
	       "\tlw\t$11, 0($8)\n"
	       "\taddiu\t$10, $10, -4\n"
	       "\tsw\t$11, 0($9)\n"
	       "\taddiu\t$8, $8, 4\n"
	       "\tbgtz\t$10, 2b\n"
	       "\taddiu\t$9, $9, 4\n"
#ifdef READBACK
	       "\tli\t$9, 0xff280000\n"
	       "\tlw\t$8, 4($31)\n"
	       "\tlw\t$10, 8($31)\n"
	       "3:\n"
	       "\tlw\t$11, 0($8)\n"
	       "\taddiu\t$10, $10, -4\n"
	       "\tsw\t$11, 0($9)\n"
	       "\taddiu\t$8, $8, 4\n"
	       "\tbgtz\t$10, 3b\n"
	       "\taddiu\t$9, $9, 4\n"
#endif
	       "\tli\t$31, 0xbfc00000\n"
	       "\tderet\n"
	       "\tnop\n"
	       "\t.end\tucopy\n", uboot_start_address, j << 2, entry_address);
	return 0;
}

