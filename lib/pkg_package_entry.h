/*
 * pkgutils
 *
 * Copyright (c) 2000-2005 Per Liden
 * Copyright (c) 2007 by CRUX team (http://crux.nu)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PKG_PACKAGE_ENTRY_H
#define __PKG_PACKAGE_ENTRY_H

typedef struct {
	int refcount;

	mode_t mode;
	uid_t uid;
	gid_t gid;

	int64_t size;

	size_t name_len;
	char name[];
} PkgPackageEntry;

PkgPackageEntry *pkg_package_entry_new (const char *name, size_t name_len);

void pkg_package_entry_ref (PkgPackageEntry *entry);
void pkg_package_entry_unref (PkgPackageEntry *entry);

#endif
