EXTRA_DIST = ldscript ledblink.S loaduboot.S u-boot.srec u-boot-le.srec

data_DATA = ledblink.srec loaduboot.srec loadubootle.srec

noinst_PROGRAMS = srec2s

srec2s_SOURCES = srec2s.c

uboot.S:	u-boot.srec srec2s
		./srec2s > $@ < $<

ubootle.S:	u-boot-le.srec srec2s
		./srec2s --little > $@ < $<

%.o:		%.S
		@TARGETCC@ -mips32 -O2 -Wall -c -o $@  $<

%.srec:		%.o
		CROSS_COMPILE=mips-linux- @TARGETLD@ -T $(srcdir)/ldscript -Map $@.map -o $@ $<

loaduboot.srec:	loaduboot.o uboot.o
		CROSS_COMPILE=mips-linux- @TARGETLD@ -T $(srcdir)/ldscript -Map $@.map -o $@ $^

loadubootle.srec:	loaduboot.o ubootle.o
		CROSS_COMPILE=mips-linux- @TARGETLD@ -T $(srcdir)/ldscript -Map $@.map -o $@ $^

#mips-linux-objdump -d ledblink.o |less
#mips-linux-objdump -m mips:isa32 -EB -D ledblink.srec|less
