noinst_DATA = ejtag.pdf

EXTRA_DIST = ejtag.tex

# Makefile for generating our latex report

all:		ejtag.pdf

SUFFIXES= .tex .dvi .ps .fig

DVI=ejtag.dvi
XDVI=xdvi
LATEX=latex
PDFLATEX=pdflatex
BIBTEX=bibtex
DVIPS=dvips
CPP=cpp
CPPFLAGS=-C -P
FIG2DEV=fig2dev
PS2PDF=ps2pdf
EPSTOPDF=epstopdf
TIFF2PS=tiff2ps


TEXSRC = ejtag.tex
FIGSRC = 
EPSSRC = 
PSSRC = 
JPGSRC = 
TIFFSRC = 

SRC = $(TEXSRC) $(FIGSRC:.fig=.eps) $(EPSSRC) $(PSSRC) $(JPGSRC:.jpg=.eps) $(TIFFSRC:.tiff=.eps)

SRCPDF = $(TEXSRC) $(FIGSRC:.fig=.pdf) $(EPSSRC:.eps=.pdf) $(PSSRC:.ps=.pdf) $(JPGSRC) $(TIFFSRC:.tiff=.png)

# implicit rules

%.dvi:		%.tex
		$(LATEX) $<
		$(BIBTEX) $* || echo "No BiBTeX"
		$(LATEX) $<

%.pdf:		%.tex
		$(PDFLATEX) $<
		$(BIBTEX) $* || echo "No BiBTeX"
		$(PDFLATEX) $<

%.ps:		%.dvi
		$(DVIPS) -f $< > $@

%2.pdf:		%.ps
		$(PS2PDF) -dDEVICEWIDTH=5953 -dDEVICEHEIGHT=8419 $< $@

%.eps:		%.fig
		$(FIG2DEV) -L eps -m 1 $< $@

%.eps:		%.tiff
		$(TIFF2PS) -e1 $< > $@

%.png:		%.tiff
		convert -verbose $< $@

%.pdf:		%.eps
		$(EPSTOPDF) $<

%.latex:	%.fig
		$(FIG2DEV) -L latex -m 0.5 $< $@

%.pstex:	%.fig
		$(FIG2DEV) -L pstex -m 1 $< $@

%.pstex_t:	%.fig %.pstex
		$(FIG2DEV) -L pstex_t -m 1 -p $*.pstex $< $@

# explicit rules

clean:
		rm -f *~ *.bak ejtag.aux ejtag.bbl \
		ejtag.blg ejtag.log ejtag.out \
		ejtag.pdf ejtag.toc ejtag.lof ejtag.lot \
		$(FIGSRC:.fig=.eps) $(FIGSRC:.fig=.pdf) $(EPSSRC:.eps=.pdf) $(PSSRC:.ps=.pdf)  \
		$(JPGSRC:.jpg=.eps) $(TIFFSRC:.tiff=.eps) $(TIFFSRC:.tiff=.png) $(TEXSRC:.tex=.aux) \
		$(DVI)

backup:
#		gnutar czf ../bericht.tgz Makefile *.tex *.fig

x:
		rm $(DVI)

ps:		ejtag.ps

pdf:		ejtag2.pdf

pdft:		ejtag.pdf

$(DVI):		$(SRC)

ejtag.pdf:		$(SRCPDF)

