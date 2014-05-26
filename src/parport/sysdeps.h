/*****************************************************************************/

/*
 *      sysdeps.h  --  System dependencies.
 *
 *      Copyright (C) 1998  Thomas Sailer (sailer@ife.ee.ethz.ch)
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

#ifndef _SYSDEPS_H
#define _SYSDEPS_H

/* ---------------------------------------------------------------------- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_SYS_IO_H)
#include <sys/io.h>
#elif defined(HAVE_ASM_IO_H)
#include <asm/io.h>
#endif

#ifdef GETOPT_H
#include <getopt.h>
#endif

#ifdef UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <stdarg.h>

/* ---------------------------------------------------------------------- */

/*
 * Bittypes
 */

#ifndef HAVE_SIGNED_BITTYPES

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
typedef int int8_t __attribute__((__mode__(__QI__)));
typedef int int16_t __attribute__((__mode__(__HI__)));
typedef int int32_t __attribute__((__mode__(__SI__)));
typedef int int64_t __attribute__((__mode__(__DI__)));
#else
typedef char /* deduced */ int8_t __attribute__((__mode__(__QI__)));
typedef short /* deduced */ int16_t __attribute__((__mode__(__HI__)));
typedef long /* deduced */ int32_t __attribute__((__mode__(__SI__)));
typedef long long /* deduced */ int64_t __attribute__((__mode__(__DI__)));
#endif

#endif /* !HAVE_SIGNED_BITTYPES */

#ifndef HAVE_UNSIGNED_BITTYPES

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
typedef unsigned int u_int8_t __attribute__((__mode__(__QI__)));
typedef unsigned int u_int16_t __attribute__((__mode__(__HI__)));
typedef unsigned int u_int32_t __attribute__((__mode__(__SI__)));
typedef unsigned int u_int64_t __attribute__((__mode__(__DI__)));
#else
typedef unsigned char /* deduced */ u_int8_t __attribute__((__mode__(__QI__)));
typedef unsigned short /* deduced */ u_int16_t __attribute__((__mode__(__HI__)));
typedef unsigned long /* deduced */ u_int32_t __attribute__((__mode__(__SI__)));
typedef unsigned long long /* deduced */ u_int64_t __attribute__((__mode__(__DI__)));
#endif

#endif /* !HAVE_UNSIGNED_BITTYPES */

/* ---------------------------------------------------------------------- */
/*
 * IO routines
 */

#ifndef HAVE_IOFUNCS

extern inline unsigned char inb(unsigned short port)
{
        unsigned char ret;
        __asm__ __volatile__("inb %w1,%b0" : "=a" (ret) : "d" (port));
        return ret;
}

extern inline void outb(unsigned char val, unsigned short port)
{
        __asm__ __volatile__("outb %b0,%w1" : : "a" (val), "d" (port));
}

extern inline unsigned short inw(unsigned short port)
{
        unsigned short ret;
        __asm__ __volatile__("inw %w1,%w0" : "=a" (ret) : "d" (port));
        return ret;
}

extern inline void outw(unsigned short val, unsigned short port)
{
        __asm__ __volatile__("outw %w0,%w1" : : "a" (val), "d" (port));
}

extern inline unsigned int inl(unsigned short port)
{
        unsigned int ret;
        __asm__ __volatile__("inl %w1,%0" : "=a" (ret) : "d" (port));
        return ret;
}

extern inline void outl(unsigned int val, unsigned short port)
{
        __asm__ __volatile__("outl %0,%w1" : : "a" (val), "d" (port));
}

extern inline void insb(unsigned short port, unsigned char *addr, unsigned long count)
{
        __asm__ __volatile__("cld ; rep ; insb" : "=D" (addr), "=c" (count) : 
                             "d" (port),"0" (addr),"1" (count));
}

extern inline void outsb(unsigned short port, const unsigned char *addr, unsigned long count)
{
        __asm__ __volatile__("cld ; rep ; outsb" : "=S" (addr), "=c" (count) : 
                             "d" (port),"0" (addr),"1" (count));
}

#endif /* !HAVE_IOFUNCS */

/* ---------------------------------------------------------------------- */
/*
 * syslog routines
 */

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else

#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

/* facility codes */
#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
#define LOG_CRON        (9<<3)  /* clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* ftp daemon */

extern inline void closelog(void) {}
extern inline void openlog(__const char *__ident, int __option, int __facility) {}
extern inline void vsyslog(int __pri, __const char *__fmt, va_list __ap) {}

#endif

/* ---------------------------------------------------------------------- */

#ifdef __MINGW32__
#include <windows.h>
extern inline void usleep(unsigned long x)
{
	Sleep(x / 1000);
}
#endif

/* ---------------------------------------------------------------------- */

#ifdef HAVE_GETTIMEOFDAY

#include <sys/time.h>
#include <unistd.h>

extern inline int gettime(struct timeval *tv)
{
	return gettimeofday(tv, NULL);
}

#elif HAVE_GETSYSTEMTIME

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if 0
struct timeval {
        long    tv_sec;
        long    tv_usec;
};
#endif

extern inline int gettime(struct timeval *tv)
{
	SYSTEMTIME tm;

	GetSystemTime(&tm);
	tv->tv_usec = 1000UL * tm.wMilliseconds;
	tv->tv_sec = tm.wSecond + 60 * tm.wMinute + 3600 * tm.wHour + 86400 * tm.wDay;
	return 0;
}

#else

#error "Don't know how to get a high resolution time"

#endif

/* ---------------------------------------------------------------------- */

#ifndef HAVE_RANDOM

extern inline long int random(void)
{
	return rand();
}

#endif

/* ---------------------------------------------------------------------- */
#endif /* _SYSDEPS_H */
