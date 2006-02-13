# -*- sh -*-

# To create the SGML files, you will need to install the tmexpand
# package.  See http://www.jedsoft.org/ for more information.
#
TMEXPAND = /aluche/d1/web/tm-dist/bin/tmexpand
TM2SGML = $(TMEXPAND)
MACRODIR = /aluche/d1/web/tm-dist/macros
TM2HLP = $(TMEXPAND) -I$(MACRODIR) -Mslhlp

FILES = jedfuns libfuns # quickref
SL_LIBDIR = ../../lib

HLP_FILES = jedfuns.hlp libfuns.hlp

TM_FILES =  $(addsuffix .tm,$(FILES))
SGML_FILES =  $(addsuffix .sgml,$(FILES))
HTML_FILES = $(addsuffix .html,$(FILES))
TXT_FILES = $(addsuffix .txt,$(FILES))
TEX_FILES = $(addsuffix .tex,$(FILES))

RM = /bin/rm
MV = /bin/mv
CP = /bin/cp

SGML2LATEX = sgml2latex
SGML2HTML = sgml2html
SGML2TXT = sgml2txt -f
SL2TM = sl2tm

help: jedfuns.hlp libfuns.hlp
all: $(HTML_FILES) $(TEX_FILES)
jedfuns.hlp :
	cat rtl/*.tm | $(TM2HLP) - jedfuns.hlp
libfuns.hlp :
	$(SL2TM) -sl $(SL_LIBDIR)/*.sl | $(TM2HLP) - libfuns.hlp
#
%.tex : %.sgml
	$(SGML2LATEX) $*.sgml
%.html : %.sgml
	$(SGML2HTML) $*.sgml
%.txt : %.sgml
	$(SGML2TXT) $*.sgml
%.sgml : %.tm
	$(TM2SGML) -I$(MACRODIR) $*.tm $@ < /dev/null
install: $(HLP_FILES)
	$(CP) $(HLP_FILES) ../hlp
clean:
	-rm -f *~ *.dvi *.log *.aux *.toc rtl/*.BAK rtl/*~ *.html *.txt *.hlp
