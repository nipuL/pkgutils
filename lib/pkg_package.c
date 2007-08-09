/*
 * pkgutil_ps
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

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <archive.h>
#include <archive_entry.h>

#include "pkg_package.h"

/* FIXME */
#define INIT_ARCHIVE(ar) \
	archive_read_support_compression_all (ar); \
	archive_read_support_format_all (ar);

#define MIN(a, b) ((a) < (b) ? (a) : (b))

PKG_API
PkgPackage *
pkg_package_new (const char *name, const char *version, const char *release)
{
	PkgPackage *pkg;
	size_t name_len, version_len, release_len;

	name_len = strlen(name);

	if (name_len > PKG_PACKAGE_MAX_NAME_LEN)
		return NULL;

	version_len = version ? strlen (version) : 0;
	if (version_len > PKG_PACKAGE_MAX_VERSION_LEN)
		return NULL;

	release_len = release ? strlen (release) : 0;
	if (release_len > PKG_PACKAGE_MAX_RELEASE_LEN)
		return NULL;

	pkg = malloc (sizeof (PkgPackage));
	if (!pkg)
		return NULL;

	memcpy (pkg->name, name, name_len + 1);

	if (version)
		memcpy (pkg->version, version, version_len + 1);
	else
		pkg->version[0] = 0;

	if (release)
		memcpy (pkg->release, release, release_len + 1);
	else
		pkg->release[0] = 0;

	pkg->refcount = 1;
	pkg->entries = NULL;

	return pkg;
}

static bool
parse_filename (PkgPackage *pkg, const char *path)
{
	char buf[PATH_MAX], *hash, *dash, *suffix;
	const char *file;
	size_t file_len, len;

	file = strrchr (path, '/');

	if (!file) {
		file = path;
		file_len = strlen (file);
	} else {
		file_len = strlen (file);
		if (file_len < 6)
			return false;

		file++;
		file_len--;
	}

	if (file_len > sizeof (buf) - 1)
		return false;

	memcpy (buf, file, file_len + 1);

	hash = strchr (buf, '#');
	if (!hash)
		return false;

	*hash++ = 0;

	len = hash - buf;
	if (len > PKG_PACKAGE_MAX_NAME_LEN - 1)
		return false;

	memcpy (pkg->name, buf, len);

	dash = strchr (hash, '-');
	if (!dash)
		return false;

	*dash++ = 0;

	len = dash - hash;
	if (len > PKG_PACKAGE_MAX_VERSION_LEN - 1)
		return false;

	memcpy (pkg->version, hash, len);

	suffix = strstr (dash, ".pkg.tar.");
	if (!suffix)
		return false;

	*suffix = 0;

	len = suffix - dash;
	if (len > PKG_PACKAGE_MAX_RELEASE_LEN - 1)
		return false;

	memcpy (pkg->release, dash, len);

	return true;
}

PKG_API
PkgPackage *
pkg_package_new_from_file (const char *file)
{
	PkgPackage *pkg;
	struct archive *archive;
	struct archive_entry *entry;
	int s;

	pkg = malloc (sizeof (PkgPackage));
	if (!pkg)
		return NULL;

	if (!parse_filename (pkg, file))
		return NULL;

	archive = archive_read_new ();
	INIT_ARCHIVE (archive);

	s = archive_read_open_filename (archive, file,
	                                ARCHIVE_DEFAULT_BYTES_PER_BLOCK);
	if (s != ARCHIVE_OK) {
		archive_read_finish (archive);
		free (pkg);

		return NULL;
	}

	pkg->entries = NULL;

	while (archive_read_next_header (archive, &entry) == ARCHIVE_OK) {
		PkgPackageEntry *pkg_entry;
		const char *s;

		s = archive_entry_pathname (entry);
		pkg_entry = pkg_package_entry_new (s, (size_t) -1);

		pkg_entry->mode = archive_entry_mode (entry);
		pkg_entry->uid = archive_entry_uid (entry);
		pkg_entry->gid = archive_entry_gid (entry);
		pkg_entry->size = archive_entry_size (entry);

		pkg->entries = list_prepend (pkg->entries, pkg_entry);
	}

	archive_read_finish (archive);

	pkg->entries = list_reverse (pkg->entries);

	pkg->refcount = 1;

	return pkg;
}

PKG_API
void
pkg_package_ref (PkgPackage *pkg)
{
	pkg->refcount++;
}

PKG_API
void
pkg_package_unref (PkgPackage *pkg)
{
	if (--pkg->refcount)
		return;

	while (pkg->entries) {
		pkg_package_entry_unref (pkg->entries->data);
		pkg->entries = list_remove_link (pkg->entries, pkg->entries);
	}

	free (pkg);
}

PKG_API
void
pkg_package_foreach (PkgPackage *pkg, PkgPackageForeachFunc func,
                     void *user_data)
{
	for (List *l = pkg->entries; l; l = l->next)
		func (pkg, l->data, user_data);
}

PKG_API
void
pkg_package_foreach_reverse (PkgPackage *pkg,
                             PkgPackageForeachFunc func,
                             void *user_data)
{
	List *l;

	for (l = pkg->entries; l->next; l = l->next)
		;

	for (; l; l = l->prev)
		func (pkg, l->data, user_data);
}

bool
pkg_package_includes (PkgPackage *pkg,
                      const char *name, size_t name_len)
{
	for (List *l = pkg->entries; l; l = l->next) {
		PkgPackageEntry *entry = l->data;

		if (entry->name_len == name_len &&
		    !memcmp (entry->name, name, name_len))
			return true;
	}

	return false;
}
