/*****************************************************************************/

/*
 *      parport.h  -- FPGA test library exports (and imports).
 *
 *      Copyright (C) 1998, 2000, 2001  Thomas Sailer (sailer@ife.ee.ethz.ch)
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

#ifndef _PARPORT_H
#define _PARPORT_H

/* ---------------------------------------------------------------------- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdeps.h"

/* ---------------------------------------------------------------------- */

/* LPT control register */
#define LPTCTRL_PROGRAM       0x04   /* 0 to reprogram */
#define LPTCTRL_WRITE         0x01
#define LPTCTRL_ADDRSTB       0x08
#define LPTCTRL_DATASTB       0x02
#define LPTCTRL_INTEN         0x10
#define LPTCTRL_DIRECTION     0x20

/* LPT status register */
#define LPTSTAT_SHIFT_NINTR   6
#define LPTSTAT_WAIT          0x80
#define LPTSTAT_NINTR         (1<<LPTSTAT_SHIFT_NINTR)
#define LPTSTAT_PE            0x20
#define LPTSTAT_DONE          0x10
#define LPTSTAT_NERROR        0x08
#define LPTSTAT_EPPTIMEOUT    0x01

/* ECP device status register */
#define LPTDSR_PERIPHACK      0x80  /* inv */
#define LPTDSR_NACK           0x40  /* noninv */
#define LPTDSR_NACKREVERSE    0x20  /* noninv */
#define LPTDSR_SELECT         0x10  /* noninv */
#define LPTDSR_NPERIPHREQ     0x08  /* noninv */

/* ECP device control register */
#define LPTDCR_DIRECTION      0x20  /* reverse dir when set */
#define LPTDCR_ACKINTEN       0x10  /* enable int on rising edge of nAck */
#define LPTDCR_1284MODE       0x08  /* inv */
#define LPTDCR_NREVERSEREQ    0x04  /* noninv */
#define LPTDCR_HOSTACK        0x02  /* inv */
#define LPTDCR_STROBE         0x01  /* inv */

#define PARPORT_CONTROL_STROBE    0x1
#define PARPORT_CONTROL_AUTOFD    0x2
#define PARPORT_CONTROL_INIT      0x4
#define PARPORT_CONTROL_SELECT    0x8
#define PARPORT_CONTROL_INTEN     0x10
#define PARPORT_CONTROL_DIRECTION 0x20

#define PARPORT_STATUS_ERROR      0x8
#define PARPORT_STATUS_SELECT     0x10
#define PARPORT_STATUS_PAPEROUT   0x20
#define PARPORT_STATUS_ACK        0x40
#define PARPORT_STATUS_BUSY       0x80

/* ---------------------------------------------------------------------- */

/* change mode constants */

#define PARPORT_MODE_PCSPP              (1<<0)
#define PARPORT_MODE_PCPS2              (1<<1)
#define PARPORT_MODE_PCEPP              (1<<2)
#define PARPORT_MODE_PCECR              (1<<3)  /* ECR Register Exists */
#define PARPORT_MODE_PCECP              (1<<4)
#define PARPORT_MODE_PCECPEPP           (1<<5)
#define PARPORT_MODE_PCECPPS2           (1<<6)

/* ---------------------------------------------------------------------- */

struct parport_ops {
	unsigned char (*parport_read_data)(void);
	void (*parport_write_data)(unsigned char d);
	unsigned char (*parport_read_status)(void);
	unsigned char (*parport_read_control)(void);
	void (*parport_write_control)(unsigned char d);
	void (*parport_frob_control)(unsigned char mask, unsigned char val);
	unsigned int (*parport_epp_write_data)(const void *buf, unsigned int sz);
	unsigned int (*parport_epp_read_data)(void *buf, unsigned int sz);
	unsigned int (*parport_epp_write_addr)(const void *buf, unsigned int sz);
	unsigned int (*parport_epp_read_addr)(void *buf, unsigned int sz);
	unsigned int (*parport_ecp_write_data)(const void *buf, unsigned int sz);
	unsigned int (*parport_ecp_read_data)(void *buf, unsigned int sz);
	unsigned int (*parport_ecp_write_addr)(const void *buf, unsigned int sz);
	unsigned int (*parport_fpgaconfig_write)(const void *buf, unsigned int sz);
	unsigned int (*parport_compat_write)(const void *buf, unsigned int sz);
};

extern struct parport_ops parport_ops;

extern unsigned int parport_generic_compat_write(const void *buf, unsigned int sz);

/* ---------------------------------------------------------------------- */

extern inline unsigned char parport_read_data(void) {
	return parport_ops.parport_read_data();
}

extern inline void parport_write_data(unsigned char d) {
	parport_ops.parport_write_data(d);
}

extern inline unsigned char parport_read_status(void) {
	return parport_ops.parport_read_status();
}

extern inline unsigned char parport_read_control(void) {
	return parport_ops.parport_read_control();
}

extern inline void parport_write_control(unsigned char d) {
	parport_ops.parport_write_control(d);
}

extern inline void parport_frob_control(unsigned char mask, unsigned char val) {
	parport_ops.parport_frob_control(mask, val);
}

extern inline unsigned parport_epp_write_data(const void *buf, unsigned int sz) {
	return parport_ops.parport_epp_write_data(buf, sz);
}

extern inline unsigned parport_epp_read_data(void *buf, unsigned int sz) {
	return parport_ops.parport_epp_read_data(buf, sz);
}

extern inline unsigned parport_epp_write_addr(const void *buf, unsigned int sz) {
	return parport_ops.parport_epp_write_addr(buf, sz);
}

extern inline unsigned parport_epp_read_addr(void *buf, unsigned int sz) {
	return parport_ops.parport_epp_read_addr(buf, sz);
}

extern inline unsigned parport_ecp_write_data(const void *buf, unsigned int sz) {
	return parport_ops.parport_ecp_write_data(buf, sz);
}

extern inline unsigned parport_ecp_read_data(void *buf, unsigned int sz) {
	return parport_ops.parport_ecp_read_data(buf, sz);
}

extern inline unsigned parport_ecp_write_addr(const void *buf, unsigned int sz) {
	return parport_ops.parport_ecp_write_addr(buf, sz);
}

extern inline unsigned parport_fpgaconfig_write(const void *buf, unsigned int sz) {
	return parport_ops.parport_fpgaconfig_write(buf, sz);
}

extern inline unsigned parport_compat_write(const void *buf, unsigned int sz) {
	if (parport_ops.parport_compat_write)
		return parport_ops.parport_compat_write(buf, sz);
	return parport_generic_compat_write(buf, sz);
}

/* ---------------------------------------------------------------------- */

#define PPFLAG_SWEMULEPP   1
#define PPFLAG_SWEMULECP   2
#define PPFLAG_FORCEHWEPP  4

extern int parport_init_direct(unsigned int io);
extern int parport_init_direct_flags(unsigned int io, unsigned int flags);
extern int parport_init_ppuser(const char *path);
extern int parport_init_ppdev(const char *path);
extern int parport_init_ppkdrv(const char *ifname);
extern int parport_init_ntddkgenport(void);
extern int parport_init_win(unsigned int portnr);        
extern int parport_init_win_flags(unsigned int portnr, unsigned int flags);
extern void parport_stop_win(void);
extern int parport_init_w9xring0(unsigned int portnr);        
extern int parport_init_w9xring0_flags(unsigned int portnr, unsigned int flags);

/* ---------------------------------------------------------------------- */

/*
 * provided externally!
 */

extern int lprintf(unsigned vl, const char *format, ...)
__attribute__ ((format (printf, 2, 3)));

/* ---------------------------------------------------------------------- */
#endif /* _PARPORT_H */
