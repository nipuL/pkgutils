#
# pkgutils
#
# Copyright (c) 2000-2005 Per Liden
# Copyright (c) 2007 by CRUX team (http://crux.nu)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

VERSION := 6.0.0

CFLAGS ?= -O2
CFLAGS += -std=c99 -fPIC -Wall -Wwrite-strings -Wnonnull

CPPFLAGS += \
	-D_GNU_SOURCE \
	-DVERSION=\"$(VERSION)\" \
	-DPKG_API="__attribute__ ((visibility(\"default\")))"

LIB_LDFLAGS := -larchive

ifdef STATIC
	PROGRAM_LDFLAGS := -static -Llib -lpkgutils -larchive -lz -lbz2
else
	PROGRAM_LDFLAGS := -Llib -lpkgutils -larchive
endif

ifndef VERBOSE
	QUIET_CC = @echo '   ' CC $@;
	QUIET_AR = @echo '   ' AR $@;
	QUIET_LINK = @echo '   ' LINK $@;
endif

PROGRAMS := src/pkginfo src/pkgrm
LIBS := lib/libpkgutils.so lib/libpkgutils.a

PROGRAM_OBJS := \
	src/main.o \
	src/common.o \
	src/pkginfo.o \
	src/pkgrm.o \

LIB_OBJS := \
	lib/bst.o \
	lib/list.o \
	lib/pkg_package.o \
	lib/pkg_package_entry.o \
	lib/pkg_database.o \

all: $(LIBS) $(PROGRAMS)

lib/%.so: $(LIB_OBJS)
	$(QUIET_LINK)$(CC) -shared $(LIB_LDFLAGS) $(LIB_OBJS) -o $@

lib/%.a: $(LIB_OBJS)
	$(QUIET_AR)$(AR) crs $@ $(LIB_OBJS)

src/pkginfo: $(LIBS) $(PROGRAM_OBJS)
	$(QUIET_LINK)$(CC) $(PROGRAM_OBJS) $(PROGRAM_LDFLAGS) -o $@

src/pkgrm: src/pkginfo
	ln -s pkginfo $@

lib/%.o: lib/%.c
	$(QUIET_CC)$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden -o $@ -c $<

src/%.o: src/%.c
	$(QUIET_CC)$(CC) $(CPPFLAGS) $(CFLAGS) -I. -o $@ -c $<

clean:
	rm -f $(LIBS) $(PROGRAMS) $(LIB_OBJS) $(PROGRAM_OBJS)

# don't delete lib/*.o after the static library is built
.PRECIOUS: lib/%.o
