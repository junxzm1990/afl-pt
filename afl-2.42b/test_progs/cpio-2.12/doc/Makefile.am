# This file is part of GNU cpio
# Copyright (C) 2004, 2007, 2010, 2014-2015 Free Software Foundation,
# Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA.

info_TEXINFOS = cpio.texi
dist_man_MANS = cpio.1 $(MT_1) $(RMT_8)

if CPIO_MT_COND
  MT_1=mt.1
endif
if PU_RMT_COND
  RMT_8=rmt.8
endif

EXTRA_DIST = gendocs.sh gendocs_template mt.1 rmt.8

GENDOCS=$(srcdir)/gendocs.sh

# Make sure you set TEXINPUT 
manual:
	rm -rf manual
	TEXINPUTS=$(srcdir):$(top_srcdir)/scripts:$$TEXINPUTS \
	  MAKEINFO="$(MAKEINFO) $(AM_MAKEINFOFLAGS) $(MAKEINFOFLAGS)" \
	  TEXI2DVI="texi2dvi -t @finalout" \
	  $(GENDOCS) --no-copy-images $(PACKAGE) '$(PACKAGE_NAME) manual'
