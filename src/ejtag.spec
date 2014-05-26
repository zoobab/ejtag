Summary: Simple MIPS EJTAG Simulator
Name: mips-ejtag
Version: 0.2
Release: 1
License: GPL
Group: Development Tools/MIPS
URL: http://www.baycom.org/~tom/ejtag/
Source0: ejtag-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
This is a simple MIPS EJTAG Parallel Port tool. It allows u-boot to be
loaded into the board SDRAM without a working bootloader. It has been
tested on AMD's Pb1000 development board.

%prep
%setup -q -n ejtag-%{version}
%configure --datadir=/usr/share/ejtag

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR="$RPM_BUILD_ROOT" install

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc AUTHORS ChangeLog COPYING INSTALL NEWS README doc/ejtag.pdf
/usr/bin/*
/usr/share/ejtag

%changelog
* Tue May 24 2005 Abhijit Bhopatkar <bainonline@gmail.com> - 0.2-1
- Support Xilinx DLC5 cable

* Wed Jan 19 2005 Thomas Sailer <t.sailer@alumni.ethz.ch> - 0.1-1
- Initial build.

