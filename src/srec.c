/*****************************************************************************/

/*
 *      srec.c  --  Motorola S Record Reader.
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

#include "ejtag.h"

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

static int store(uint32_t addr, uint8_t data, void *mem)
{
	uint32_t *memptr, mask, d;

	if (addr < 0xff200000 || addr > 0xff3fffff)
		return -1;
	if (!mem)
		return 0;
	addr ^= 3;  /* big endian */
	memptr = mem;
	addr -= 0xff200000;
	memptr += addr >> 2;
	d = data * 0x01010101;
	mask = 0xff << (8*(addr & 3));
	*memptr = (*memptr & ~mask) | (d & mask);
	return 0;
}

static int process_line(const char *line, void *mem)
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
	case '7': /* ignoring ?? */
	case '8': /* ignoring ?? */
	case '9': /* ignoring ?? */
		return 0;

	case '1':
		addr = (buf[1] << 8) | buf[2];
		for (i = 3; i < buf[0]; i++, addr++)
			if (store(addr, buf[i], mem))
				goto storeerr;
		return 0;

	case '2':
		addr = (buf[1] << 16) | (buf[2] << 8) | buf[3];
		for (i = 4; i < buf[0]; i++, addr++)
			if (store(addr, buf[i], mem))
				goto storeerr;
		return 0;

	case '3':
		addr = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
		for (i = 5; i < buf[0]; i++, addr++)
			if (store(addr, buf[i], mem))
				goto storeerr;
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

int read_hex_file(const char *fn, void *mem)
{
	FILE *f;
	char buf[600];

	if (!fn)
		return -1;
	f = fopen(fn, "r");
	if (!f) {
		fprintf(stderr, "cannot open file \"%s\"\n", fn);
		return -1;
	}
	while (fgets(buf, sizeof(buf), f))
		if (process_line(buf, mem)) {
			fclose(f);
			return -1;
		}
	fclose(f);
	return 0;
}
