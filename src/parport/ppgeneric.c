/*****************************************************************************/

/*
 *      ppgeneric.c  --  Parport generic routines.
 *
 *      Copyright (C) 2002
 *        Thomas Sailer (t.sailer@alumni.ethz.ch)
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

#include "parport.h"

/* ---------------------------------------------------------------------- */

unsigned int parport_generic_compat_write(const void *buf, unsigned int sz)
{
        unsigned int count = 0;
        const unsigned char *addr = buf;
        u_int8_t ctl = (PARPORT_CONTROL_SELECT | PARPORT_CONTROL_INIT);

        parport_write_control(ctl);
        while (count < sz) {
		unsigned int tmo = 0;
		u_int8_t status;

                /* Wait until the peripheral's ready */
                for (;;) {
			status = parport_read_status();

                        /* Is the peripheral ready yet? */
                        if ((status & (PARPORT_STATUS_ERROR | PARPORT_STATUS_BUSY)) ==
			    (PARPORT_STATUS_ERROR | PARPORT_STATUS_BUSY))
                                /* Skip the loop */
                                goto ready;

                        /* Is the peripheral upset? */
                        if ((status &
                             (PARPORT_STATUS_PAPEROUT |
                              PARPORT_STATUS_SELECT |
                              PARPORT_STATUS_ERROR))
                            != (PARPORT_STATUS_SELECT |
                                PARPORT_STATUS_ERROR))
                                /* If nFault is asserted (i.e. no
                                 * error) and PAPEROUT and SELECT are
                                 * just red herrings, give the driver
                                 * a chance to check it's happy with
                                 * that before continuing. */
                                goto stop;

                        /* Have we run out of time? */
			if (tmo > 1000)
				break;
			tmo++;
                }
                break;

        ready:
                /* Write the character to the data lines. */
                parport_write_data(*addr++);
                //udelay (1);
                /* Pulse strobe. */
                parport_write_control(ctl | PARPORT_CONTROL_STROBE);
                //udelay (1); /* strobe */

                parport_write_control(ctl);
                //udelay (1); /* hold */

                /* Assume the peripheral received it. */
                count++;
        }
 stop:
        return count;
}
