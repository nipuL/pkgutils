#
#  pkgutils
#
#  Copyright (c) 2000-2005 by Per Liden <per@fukt.bth.se>
#  Copyright (c) 2006-2007 by CRUX team (http://crux.nu)
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
#  USA.
#

DESTDIR =
BINDIR = /usr/bin
MANDIR = /usr/man
ETCDIR = /etc

VERSION = 5.33.0
NAME = pkgutils-$(VERSION)

CXXFLAGS += -DNDEBUG
CXXFLAGS += -O2 -Wall -pedantic -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" \
	    -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

LDFLAGS += -static -larchive -lz -lbz2

OBJECTS = main.o pkgutil.o pkgadd.o pkgrm.o pkginfo.o

MANPAGES = pkgadd.8 pkgrm.8 pkginfo.8 pkgmk.8 rejmerge.8 pkgmk.conf.5

all: pkgadd pkgmk rejmerge man

pkgadd: .depend $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

pkgmk: pkgmk.in

rejmerge: rejmerge.in

man: $(MANPAGES)

mantxt: man $(MANPAGES:=.txt)

%.8.txt: %.8
	nroff -mandoc -c $< | col -bx > $@

%: %.in
	sed -e "s/#VERSION#/$(VERSION)/" $< > $@

.depend:
	$(CXX) $(CXXFLAGS) -MM $(OBJECTS:.o=.cc) > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

.PHONY:	install clean distclean dist

dist: distclean
	rm -rf $(NAME) $(NAME).tar.gz
	git archive --format=tar --prefix=$(NAME)/ HEAD | tar -x
	git log > $(NAME)/ChangeLog
	tar czvf $(NAME).tar.gz $(NAME)
	rm -rf $(NAME)

install: all
	install -D -m0755 pkgadd $(DESTDIR)$(BINDIR)/pkgadd
	install -D -m0644 pkgadd.conf $(DESTDIR)$(ETCDIR)/pkgadd.conf
	install -D -m0755 pkgmk $(DESTDIR)$(BINDIR)/pkgmk
	install -D -m0755 rejmerge $(DESTDIR)$(BINDIR)/rejmerge
	install -D -m0644 pkgmk.conf $(DESTDIR)$(ETCDIR)/pkgmk.conf
	install -D -m0644 rejmerge.conf $(DESTDIR)$(ETCDIR)/rejmerge.conf
	install -D -m0644 pkgadd.8 $(DESTDIR)$(MANDIR)/man8/pkgadd.8
	install -D -m0644 pkgrm.8 $(DESTDIR)$(MANDIR)/man8/pkgrm.8
	install -D -m0644 pkginfo.8 $(DESTDIR)$(MANDIR)/man8/pkginfo.8
	install -D -m0644 pkgmk.8 $(DESTDIR)$(MANDIR)/man8/pkgmk.8
	install -D -m0644 rejmerge.8 $(DESTDIR)$(MANDIR)/man8/rejmerge.8
	install -D -m0644 pkgmk.conf.5 $(DESTDIR)$(MANDIR)/man5/pkgmk.conf.5
	ln -sf pkgadd $(DESTDIR)$(BINDIR)/pkgrm
	ln -sf pkgadd $(DESTDIR)$(BINDIR)/pkginfo

clean:
	rm -f .depend
	rm -f $(OBJECTS)
	rm -f $(MANPAGES)
	rm -f $(MANPAGES:=.txt)

distclean: clean
	rm -f pkgadd pkginfo pkgrm pkgmk rejmerge

# End of file
