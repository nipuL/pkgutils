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

	void *archive_entry;

	size_t name_len;
	char name[];
} PkgPackageEntry;

PkgPackageEntry *pkg_package_entry_new (const char *name, size_t name_len);

PkgPackageEntry *pkg_package_entry_ref (PkgPackageEntry *entry);
void pkg_package_entry_unref (PkgPackageEntry *entry);

PKG_API
mode_t pkg_package_entry_get_mode (PkgPackageEntry *entry);

PKG_API
uid_t pkg_package_entry_get_uid (PkgPackageEntry *entry);

PKG_API
gid_t pkg_package_entry_get_gid (PkgPackageEntry *entry);

PKG_API
int64_t pkg_package_entry_get_size (PkgPackageEntry *entry);

PKG_API
dev_t pkg_package_entry_get_dev (PkgPackageEntry *entry);

PKG_API
const char *pkg_package_entry_get_symlink_target (PkgPackageEntry *entry);

#endif
