A simple MIPS EJTAG u-boot loader
=================================

##What is it?##

   This program allows you to connect a MIPS EJTAG capable CPU to the PC parallel port. The CPU can then be booted from the PC, i.e. the PC loads the u-boot bootloader into the MIPS target SDRAM, and runs it.
   That way, you can bring up a MIPS board without having a programmed FLASH or similar on board.

   I have tested it on an AMD Alchemy Pb1000 board. It takes roughly 37 seconds to load u-boot into memory.

##Files##

   File Name                             Size Description
   ejtag-0.2.tar.gz                    307687 Source Code
   mips-ejtag-0.2-1.src.rpm            308922 RPM source package
   mips-ejtag-0.2-1.i386.rpm           140002 RPM binary package compiled for i386 Fedora Core 3
   mips-ejtag-debuginfo-0.2-1.i386.rpm  47203 Debuginfo RPM Package

##Authors##

   Thomas Sailer

##Links##

* https://web.archive.org/web/20070112094638/http://www.baycom.org/%7Etom/ejtag/
* https://web.archive.org/web/20070107110736/http://www.baycom.org/%7Etom/ejtag/ejtag-0.2.tar.gz
* https://web.archive.org/web/20070107110736/http://www.baycom.org/%7Etom/ejtag/mips-ejtag-0.2-1.src.rpm
* https://web.archive.org/web/20070107110736/http://www.baycom.org/%7Etom/ejtag/mips-ejtag-0.2-1.i386.rpm
* https://web.archive.org/web/20070107110736/http://www.baycom.org/%7Etom/ejtag/mips-ejtag-debuginfo-0.2-1.i386.rpm
