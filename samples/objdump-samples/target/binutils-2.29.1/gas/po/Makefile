# Makefile for program source directory in GNU NLS utilities package.
# Copyright (C) 1995, 1996, 1997 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
# Copyright (C) 2003-2017 Free Software Foundation, Inc.
#
# This file may be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

PACKAGE = gas
VERSION = 2.29.1

SHELL = /bin/sh


srcdir = .
top_srcdir = ..

top_builddir = ..

prefix = /usr/local
exec_prefix = ${prefix}
datadir = $(prefix)/share
localedir = $(datadir)/locale
gnulocaledir = $(prefix)/share/locale
gettextsrcdir = $(prefix)/share/gettext/po
subdir = po

DESTDIR =

INSTALL = /usr/bin/install -c
INSTALL_DATA = /usr/bin/install -c -m 644
MKINSTALLDIRS = $(top_builddir)/../mkinstalldirs

CC = gcc
GENCAT = gencat
GMSGFMT = PATH=../src:$$PATH /usr/bin/msgfmt
MSGFMT = /usr/bin/msgfmt
XGETTEXT = PATH=../src:$$PATH /usr/bin/xgettext
MSGMERGE = PATH=../src:$$PATH msgmerge

DEFS = -DHAVE_CONFIG_H
CFLAGS = -fsanitize=address -g
CPPFLAGS = 

INCLUDES = -I.. -I$(top_srcdir)/intl

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

SOURCES = cat-id-tbl.c
POFILES =  fr.po tr.po es.po rw.po id.po ru.po fi.po ja.po zh_CN.po sv.po
GMOFILES =  fr.gmo tr.gmo es.gmo rw.gmo id.gmo ru.gmo fi.gmo ja.gmo zh_CN.gmo sv.gmo
DISTFILES = ChangeLog Makefile.in.in POTFILES.in $(PACKAGE).pot \
stamp-cat-id $(POFILES) $(GMOFILES) $(SOURCES)

POTFILES = \
     ../app.c \
     ../as.c \
     ../as.h \
     ../asintl.h \
     ../atof-generic.c \
     ../bignum.h \
     ../bit_fix.h \
     ../cgen.c \
     ../cgen.h \
     ../compress-debug.c \
     ../compress-debug.h \
     ../cond.c \
     ../config/atof-ieee.c \
     ../config/atof-vax.c \
     ../config/e-crisaout.c \
     ../config/e-criself.c \
     ../config/e-i386aout.c \
     ../config/e-i386coff.c \
     ../config/e-i386elf.c \
     ../config/e-mipself.c \
     ../config/obj-aout.c \
     ../config/obj-aout.h \
     ../config/obj-coff.c \
     ../config/obj-coff.h \
     ../config/obj-ecoff.c \
     ../config/obj-ecoff.h \
     ../config/obj-elf.c \
     ../config/obj-elf.h \
     ../config/obj-evax.c \
     ../config/obj-evax.h \
     ../config/obj-fdpicelf.c \
     ../config/obj-fdpicelf.h \
     ../config/obj-macho.c \
     ../config/obj-macho.h \
     ../config/obj-multi.c \
     ../config/obj-multi.h \
     ../config/obj-som.c \
     ../config/obj-som.h \
     ../config/tc-aarch64.c \
     ../config/tc-aarch64.h \
     ../config/tc-alpha.c \
     ../config/tc-alpha.h \
     ../config/tc-arc.c \
     ../config/tc-arc.h \
     ../config/tc-arm.c \
     ../config/tc-arm.h \
     ../config/tc-avr.c \
     ../config/tc-avr.h \
     ../config/tc-bfin.c \
     ../config/tc-bfin.h \
     ../config/tc-cr16.c \
     ../config/tc-cr16.h \
     ../config/tc-cris.c \
     ../config/tc-cris.h \
     ../config/tc-crx.c \
     ../config/tc-crx.h \
     ../config/tc-d10v.c \
     ../config/tc-d10v.h \
     ../config/tc-d30v.c \
     ../config/tc-d30v.h \
     ../config/tc-dlx.c \
     ../config/tc-dlx.h \
     ../config/tc-epiphany.c \
     ../config/tc-epiphany.h \
     ../config/tc-fr30.c \
     ../config/tc-fr30.h \
     ../config/tc-frv.c \
     ../config/tc-frv.h \
     ../config/tc-ft32.c \
     ../config/tc-ft32.h \
     ../config/tc-h8300.c \
     ../config/tc-h8300.h \
     ../config/tc-hppa.c \
     ../config/tc-hppa.h \
     ../config/tc-i370.c \
     ../config/tc-i370.h \
     ../config/tc-i386.c \
     ../config/tc-i386.h \
     ../config/tc-i860.c \
     ../config/tc-i860.h \
     ../config/tc-i960.c \
     ../config/tc-i960.h \
     ../config/tc-ia64.c \
     ../config/tc-ia64.h \
     ../config/tc-ip2k.c \
     ../config/tc-ip2k.h \
     ../config/tc-iq2000.c \
     ../config/tc-iq2000.h \
     ../config/tc-lm32.c \
     ../config/tc-lm32.h \
     ../config/tc-m32c.c \
     ../config/tc-m32c.h \
     ../config/tc-m32r.c \
     ../config/tc-m32r.h \
     ../config/tc-m68hc11.c \
     ../config/tc-m68hc11.h \
     ../config/tc-m68k.c \
     ../config/tc-m68k.h \
     ../config/tc-mcore.c \
     ../config/tc-mcore.h \
     ../config/tc-mep.c \
     ../config/tc-mep.h \
     ../config/tc-metag.c \
     ../config/tc-metag.h \
     ../config/tc-microblaze.c \
     ../config/tc-microblaze.h \
     ../config/tc-mips.c \
     ../config/tc-mips.h \
     ../config/tc-mmix.c \
     ../config/tc-mmix.h \
     ../config/tc-mn10200.c \
     ../config/tc-mn10200.h \
     ../config/tc-mn10300.c \
     ../config/tc-mn10300.h \
     ../config/tc-moxie.c \
     ../config/tc-msp430.c \
     ../config/tc-msp430.h \
     ../config/tc-mt.c \
     ../config/tc-mt.h \
     ../config/tc-nds32.c \
     ../config/tc-nds32.h \
     ../config/tc-nios2.c \
     ../config/tc-nios2.h \
     ../config/tc-ns32k.c \
     ../config/tc-ns32k.h \
     ../config/tc-or1k.c \
     ../config/tc-or1k.h \
     ../config/tc-pdp11.c \
     ../config/tc-pdp11.h \
     ../config/tc-pj.c \
     ../config/tc-pj.h \
     ../config/tc-ppc.c \
     ../config/tc-ppc.h \
     ../config/tc-pru.c \
     ../config/tc-pru.h \
     ../config/tc-riscv.c \
     ../config/tc-riscv.h \
     ../config/tc-rl78.c \
     ../config/tc-rl78.h \
     ../config/tc-rx.c \
     ../config/tc-rx.h \
     ../config/tc-s390.c \
     ../config/tc-s390.h \
     ../config/tc-score.c \
     ../config/tc-score.h \
     ../config/tc-sh.c \
     ../config/tc-sh.h \
     ../config/tc-sh64.c \
     ../config/tc-sh64.h \
     ../config/tc-sparc.c \
     ../config/tc-sparc.h \
     ../config/tc-spu.c \
     ../config/tc-spu.h \
     ../config/tc-tic30.c \
     ../config/tc-tic30.h \
     ../config/tc-tic4x.c \
     ../config/tc-tic4x.h \
     ../config/tc-tic54x.c \
     ../config/tc-tic54x.h \
     ../config/tc-tic6x.c \
     ../config/tc-tic6x.h \
     ../config/tc-tilegx.c \
     ../config/tc-tilegx.h \
     ../config/tc-tilepro.c \
     ../config/tc-tilepro.h \
     ../config/tc-v850.c \
     ../config/tc-v850.h \
     ../config/tc-vax.c \
     ../config/tc-vax.h \
     ../config/tc-visium.c \
     ../config/tc-visium.h \
     ../config/tc-wasm32.c \
     ../config/tc-wasm32.h \
     ../config/tc-xc16x.c \
     ../config/tc-xc16x.h \
     ../config/tc-xgate.c \
     ../config/tc-xgate.h \
     ../config/tc-xstormy16.c \
     ../config/tc-xstormy16.h \
     ../config/tc-xtensa.c \
     ../config/tc-xtensa.h \
     ../config/tc-z80.c \
     ../config/tc-z80.h \
     ../config/tc-z8k.c \
     ../config/tc-z8k.h \
     ../config/te-386bsd.h \
     ../config/te-aix5.h \
     ../config/te-armeabi.h \
     ../config/te-armfbsdeabi.h \
     ../config/te-armfbsdvfp.h \
     ../config/te-armlinuxeabi.h \
     ../config/te-dynix.h \
     ../config/te-epoc-pe.h \
     ../config/te-freebsd.h \
     ../config/te-generic.h \
     ../config/te-gnu.h \
     ../config/te-go32.h \
     ../config/te-hppa.h \
     ../config/te-hppa64.h \
     ../config/te-hppalinux64.h \
     ../config/te-i386aix.h \
     ../config/te-ia64aix.h \
     ../config/te-interix.h \
     ../config/te-lynx.h \
     ../config/te-mach.h \
     ../config/te-macos.h \
     ../config/te-nbsd.h \
     ../config/te-nbsd532.h \
     ../config/te-netware.h \
     ../config/te-pc532mach.h \
     ../config/te-pe.h \
     ../config/te-psos.h \
     ../config/te-riscix.h \
     ../config/te-solaris.h \
     ../config/te-sparcaout.h \
     ../config/te-sun3.h \
     ../config/te-svr4.h \
     ../config/te-symbian.h \
     ../config/te-tmips.h \
     ../config/te-vms.c \
     ../config/te-vxworks.h \
     ../config/te-wince-pe.h \
     ../config/xtensa-relax.c \
     ../config/xtensa-relax.h \
     ../depend.c \
     ../dw2gencfi.c \
     ../dw2gencfi.h \
     ../dwarf2dbg.c \
     ../dwarf2dbg.h \
     ../ecoff.c \
     ../ecoff.h \
     ../ehopt.c \
     ../emul-target.h \
     ../emul.h \
     ../expr.c \
     ../expr.h \
     ../flonum-copy.c \
     ../flonum-konst.c \
     ../flonum-mult.c \
     ../flonum.h \
     ../frags.c \
     ../frags.h \
     ../hash.c \
     ../hash.h \
     ../input-file.c \
     ../input-file.h \
     ../input-scrub.c \
     ../itbl-lex.h \
     ../itbl-ops.c \
     ../itbl-ops.h \
     ../listing.c \
     ../listing.h \
     ../literal.c \
     ../macro.c \
     ../macro.h \
     ../messages.c \
     ../obj.h \
     ../output-file.c \
     ../output-file.h \
     ../read.c \
     ../read.h \
     ../remap.c \
     ../sb.c \
     ../sb.h \
     ../stabs.c \
     ../struc-symbol.h \
     ../subsegs.c \
     ../subsegs.h \
     ../symbols.c \
     ../symbols.h \
     ../tc.h \
     ../write.c \
     ../write.h

CATALOGS =  es.gmo fi.gmo fr.gmo id.gmo ja.gmo ru.gmo rw.gmo sv.gmo tr.gmo uk.gmo zh_CN.gmo es.gmo fi.gmo fr.gmo id.gmo ja.gmo ru.gmo rw.gmo sv.gmo tr.gmo uk.gmo zh_CN.gmo
CATOBJEXT = .gmo
INSTOBJEXT = .mo

.SUFFIXES:
.SUFFIXES: .c .o .po .pox .gmo .mo .msg .cat

.c.o:
	$(COMPILE) $<

.po.pox:
	$(MAKE) $(PACKAGE).pot
	$(MSGMERGE) $< $(srcdir)/$(PACKAGE).pot -o $*.pox

.po.mo:
	$(MSGFMT) -o $@ $<

.po.gmo:
	file=`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) -o $$file $<

.po.cat:
	sed -f ../intl/po2msg.sed < $< > $*.msg \
	  && rm -f $@ && $(GENCAT) $@ $*.msg


all: all-yes

all-yes: $(CATALOGS) # $(PACKAGE).pot
all-no:

$(srcdir)/$(PACKAGE).pot: $(POTFILES)
	$(XGETTEXT) --default-domain=$(PACKAGE) --directory=$(top_srcdir) \
	  --add-comments --keyword=_ --keyword=N_ \
	  --msgid-bugs-address=bug-binutils@gnu.org \
	  --files-from=$(srcdir)/POTFILES.in
	rm -f $(srcdir)/$(PACKAGE).pot
	mv $(PACKAGE).po $(srcdir)/$(PACKAGE).pot

$(srcdir)/cat-id-tbl.c: stamp-cat-id; @:
$(srcdir)/stamp-cat-id: $(PACKAGE).pot
	rm -f cat-id-tbl.tmp
	sed -f ../intl/po2tbl.sed $(srcdir)/$(PACKAGE).pot \
		| sed -e "s/@PACKAGE NAME@/$(PACKAGE)/" > cat-id-tbl.tmp
	if cmp -s cat-id-tbl.tmp $(srcdir)/cat-id-tbl.c; then \
	  rm cat-id-tbl.tmp; \
	else \
	  echo cat-id-tbl.c changed; \
	  rm -f $(srcdir)/cat-id-tbl.c; \
	  mv cat-id-tbl.tmp $(srcdir)/cat-id-tbl.c; \
	fi
	cd $(srcdir) && rm -f stamp-cat-id && echo timestamp > stamp-cat-id


install: install-exec install-data
install-exec:
install-info:
install-html:
install-pdf:
install-data: install-data-yes
install-data-no: all
install-data-yes: all
	if test -r $(MKINSTALLDIRS); then \
	  $(MKINSTALLDIRS) $(DESTDIR)$(datadir); \
	else \
	  $(top_srcdir)/mkinstalldirs $(DESTDIR)$(datadir); \
	fi
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  case "$$cat" in \
	    *.gmo) destdir=$(gnulocaledir);; \
	    *)     destdir=$(localedir);; \
	  esac; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  dir=$(DESTDIR)$$destdir/$$lang/LC_MESSAGES; \
	  if test -r $(MKINSTALLDIRS); then \
	    $(MKINSTALLDIRS) $$dir; \
	  else \
	    $(top_srcdir)/mkinstalldirs $$dir; \
	  fi; \
	  if test -r $$cat; then \
	    $(INSTALL_DATA) $$cat $$dir/$(PACKAGE)$(INSTOBJEXT); \
	    echo "installing $$cat as $$dir/$(PACKAGE)$(INSTOBJEXT)"; \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$cat $$dir/$(PACKAGE)$(INSTOBJEXT); \
	    echo "installing $(srcdir)/$$cat as" \
		 "$$dir/$(PACKAGE)$(INSTOBJEXT)"; \
	  fi; \
	  if test -r $$cat.m; then \
	    $(INSTALL_DATA) $$cat.m $$dir/$(PACKAGE)$(INSTOBJEXT).m; \
	    echo "installing $$cat.m as $$dir/$(PACKAGE)$(INSTOBJEXT).m"; \
	  else \
	    if test -r $(srcdir)/$$cat.m ; then \
	      $(INSTALL_DATA) $(srcdir)/$$cat.m \
		$$dir/$(PACKAGE)$(INSTOBJEXT).m; \
	      echo "installing $(srcdir)/$$cat as" \
		   "$$dir/$(PACKAGE)$(INSTOBJEXT).m"; \
	    else \
	      true; \
	    fi; \
	  fi; \
	done
	if test "$(PACKAGE)" = "gettext"; then \
	  if test -r $(MKINSTALLDIRS); then \
	    $(MKINSTALLDIRS) $(DESTDIR)$(gettextsrcdir); \
	  else \
	    $(top_srcdir)/mkinstalldirs $(DESTDIR)$(gettextsrcdir); \
	  fi; \
	  $(INSTALL_DATA) $(srcdir)/Makefile.in.in \
			  $(DESTDIR)$(gettextsrcdir)/Makefile.in.in; \
	else \
	  : ; \
	fi

# Define this as empty until I found a useful application.
installcheck:

uninstall:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT); \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT).m; \
	  rm -f $(DESTDIR)$(gnulocaledir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT); \
	  rm -f $(DESTDIR)$(gnulocaledir)/$$lang/LC_MESSAGES/$(PACKAGE)$(INSTOBJEXT).m; \
	done
	rm -f $(DESTDIR)$(gettextsrcdir)/po-Makefile.in.in

check: all

cat-id-tbl.o: ../intl/libgettext.h

html dvi pdf ps info tags TAGS ID:

mostlyclean:
	rm -f core core.* *.pox $(PACKAGE).po *.old.po cat-id-tbl.tmp
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo *.msg *.cat *.cat.m

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f $(GMOFILES)

distdir = ../$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir: update-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	for file in $$dists; do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

update-po: Makefile
	$(MAKE) $(PACKAGE).pot
	PATH=`pwd`/../src:$$PATH; \
	cd $(srcdir); \
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  mv $$lang.po $$lang.old.po; \
	  echo "$$lang:"; \
	  if $(MSGMERGE) $$lang.old.po $(PACKAGE).pot -o $$lang.po; then \
	    rm -f $$lang.old.po; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.po; \
	    mv $$lang.old.po $$lang.po; \
	  fi; \
	done

POTFILES: POTFILES.in
	( if test 'x$(srcdir)' != 'x.'; then \
	    posrcprefix='$(top_srcdir)/'; \
	  else \
	    posrcprefix="../"; \
	  fi; \
	  rm -f $@-t $@ \
	    && (sed -e '/^#/d' -e '/^[ 	]*$$/d' \
		    -e "s@.*@	$$posrcprefix& \\\\@" < $(srcdir)/$@.in \
		| sed -e '$$s/\\$$//') > $@-t \
	    && chmod a-w $@-t \
	    && mv $@-t $@ )

POTFILES.in: # ../Makefile
	cd .. && $(MAKE) po/POTFILES.in

Makefile: Make-in ../config.status POTFILES
	cd .. \
	  && CONFIG_FILES=$(subdir)/Makefile.in:$(subdir)/Make-in \
	     CONFIG_HEADERS= $(SHELL) ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
