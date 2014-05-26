## Process this file with automake to produce Makefile.in

INCLUDES = -DCONFIGDIR=\"$(sysconfdir)\" -DLOCALSTATEDIR=\"$(localstatedir)\" \
	-I$(top_srcdir)/ -I$(top_srcdir)/ntdrv -I$(top_srcdir)/w9xdrv -I$(top_srcdir)/include -I$(srcdir)

if PPUSER
PPSRC1 = ppuser.c ppuserinit.c ppdev.c ppdevinit.c
else
PPSRC1 = 
endif

if PPKDRV
PPSRC2 = ppkdrv.c ppkdrvinit.c
else
PPSRC2 = 
endif

if WIN32
PPSRC3 = ppwin.c ppwininit.c ppntddkgenport.c ppntddkgenportinit.c ppw9xring0.c ppw9xring0init.c
else
PPSRC3 =
endif

noinst_LIBRARIES = libparport.a

libparport_a_SOURCES = \
	ppdirect.c ppgeneric.c $(PPSRC1) $(PPSRC2) $(PPSRC3) ppdirectinit.c

noinst_HEADERS =        \
	sysdeps.h       \
	util.h          \
	parport.h       \
	ppdev.h         \
	ppuser.h


EXTRA_DIST = \
	ppuser.c ppuserinit.c ppdev.c ppdevinit.c ppkdrv.c ppkdrvinit.c \
	ppntddkgenport.c ppntddkgenportinit.c ppnt.c ppntinit.c ppwin.c ppwininit.c\
	ppw9xring0.c ppw9xring0init.c
