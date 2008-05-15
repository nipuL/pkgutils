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

#ifndef __PKG_DATABASE_H
#define __PKG_DATABASE_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>

#include "pkg_package.h"
#include "list.h"

typedef struct {
	int dir;
} PkgDatabaseLock;

typedef struct {
	int refcount;

	PkgDatabaseLock lock;
	FILE *fp;
	List *packages;

	int root;
} PkgDatabase;

typedef enum {
	PKG_DATABASE_READ_ALL,
	PKG_DATABASE_READ_NAMES_ONLY,
} PkgDatabaseReadMode;

typedef void (*PkgDatabaseForeachFunc) (PkgPackage *pkg, void *user_data);

PKG_API
PkgDatabase *pkg_database_new (const char *root, bool exclusive, int *error);

PKG_API
void pkg_database_ref (PkgDatabase *db);

PKG_API
void pkg_database_unref (PkgDatabase *db);

PKG_API
void pkg_database_read_package_list (PkgDatabase *db,
                                     PkgDatabaseReadMode mode);

PKG_API
void pkg_database_foreach (PkgDatabase *db, PkgDatabaseForeachFunc func,
                           void *user_data);

PKG_API
PkgPackage *pkg_database_find (PkgDatabase *db, const char *name);

PKG_API
void pkg_database_fill_package_files (PkgDatabase *db, PkgPackage *pkg);

PKG_API
bool pkg_database_remove (PkgDatabase *db, const char *name);

#endif
