SUBDIRS = libmisc ntdrv w9xdrv parport target doc
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/libmisc -I$(top_srcdir)/parport

EXTRA_DIST = cfgwindows cfgwindowsnodebug ejtag.spec

bin_PROGRAMS = ejtag

ejtag_SOURCES = jtdriver.c jtdriver_dlc5.c jtag.c srec.c main.c
ejtag_LDADD = libmisc/libmisc.a parport/libparport.a

noinst_HEADERS = include/ejtag.h include/util.h
