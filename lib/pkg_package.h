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

#ifndef __PKG_PACKAGE_H
#define __PKG_PACKAGE_H

#include <stdbool.h>

#include "pkg_package_entry.h"
#include "list.h"

#define PKG_PACKAGE_MAX_NAME_LEN 63
#define PKG_PACKAGE_MAX_VERSION_LEN 15
#define PKG_PACKAGE_MAX_RELEASE_LEN 15

typedef struct {
	int refcount;

	List *entries;

	char version[PKG_PACKAGE_MAX_VERSION_LEN + 1];
	char release[PKG_PACKAGE_MAX_RELEASE_LEN + 1];
	char name[];
} PkgPackage;

typedef void (*PkgPackageForeachFunc) (PkgPackageEntry *entry,
                                       void *user_data);

PKG_API
PkgPackage *pkg_package_new (const char *name, const char *version,
                             const char *release);

PKG_API
PkgPackage *pkg_package_new_from_file (const char *file);

PKG_API
void pkg_package_ref (PkgPackage *pkg);

PKG_API
void pkg_package_unref (PkgPackage *pkg);

PKG_API
void pkg_package_foreach (PkgPackage *pkg, PkgPackageForeachFunc func,
                          void *user_data);

PKG_API
void pkg_package_foreach_reverse (PkgPackage *pkg,
                                  PkgPackageForeachFunc func,
                                  void *user_data);

bool pkg_package_includes (PkgPackage *pkg,
                           const char *name, size_t name_len);

#endif
