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

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <archive.h>
#include <archive_entry.h>

#include "pkg_package.h"

static int
compare_entries_cb (void *a, void *b)
{
	PkgPackageEntry *aa = a;
	PkgPackageEntry *bb = b;

	return strcmp (aa->name, bb->name);
}

static int
find_entry_by_name_cb (void *a, void *b)
{
	PkgPackageEntry *bb = b;

	return strcmp (a, &bb->name[1]);
}

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

	pkg = malloc (sizeof (PkgPackage) + name_len + 1);
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
	pkg->entries = bst_new (compare_entries_cb,
	                        (BstNodeFreeFunc) pkg_package_entry_unref);

	return pkg;
}

static PkgPackage *
create_from_filename (const char *path)
{
	PkgPackage *pkg;
	char buf[PATH_MAX], *hash, *dash, *suffix;
	const char *file;
	size_t len, name_len, version_len, release_len;

	file = basename (path);
	len = strlen (file);

	if (len < strlen (".pkg.tar.") || len >= sizeof (buf))
		return NULL;

	memcpy (buf, file, len + 1);

	hash = strchr (buf, '#');
	if (!hash)
		return NULL;

	*hash++ = 0;

	name_len = hash - buf;
	if (name_len > PKG_PACKAGE_MAX_NAME_LEN + 1)
		return NULL;

	dash = strchr (hash, '-');
	if (!dash)
		return NULL;

	*dash++ = 0;

	version_len = dash - hash;
	if (version_len > PKG_PACKAGE_MAX_VERSION_LEN + 1)
		return NULL;

	suffix = strstr (dash, ".pkg.tar.");
	if (!suffix)
		return NULL;

	*suffix++ = 0;

	release_len = suffix - dash;
	if (release_len > PKG_PACKAGE_MAX_RELEASE_LEN + 1)
		return NULL;

	pkg = malloc (sizeof (PkgPackage) + name_len);
	if (!pkg)
		return NULL;

	memcpy (pkg->name, buf, name_len);
	memcpy (pkg->version, hash, version_len);
	memcpy (pkg->release, dash, release_len);

	return pkg;
}

static struct archive *
open_archive (const char *file)
{
	struct archive *archive;
	int s;

	archive = archive_read_new ();

	archive_read_support_compression_all (archive);
	archive_read_support_format_all (archive);

	s = archive_read_open_filename (archive, file,
	                                ARCHIVE_DEFAULT_BYTES_PER_BLOCK);
	if (s != ARCHIVE_OK) {
		archive_read_finish (archive);
		return NULL;
	}

	return archive;
}

static bool
read_archive (PkgPackage *pkg, const char *file)
{
	struct archive *archive;
	struct archive_entry *entry;
	bool contains_hardlinks = false;

	pkg->entries = bst_new (compare_entries_cb,
	                        (BstNodeFreeFunc) pkg_package_entry_unref);

	archive = open_archive (file);
	if (!archive)
		return false;

	while (archive_read_next_header (archive, &entry) == ARCHIVE_OK) {
		PkgPackageEntry *pkg_entry;
		const char *s;

		if (archive_entry_hardlink (entry)) {
			contains_hardlinks = true;
			continue;
		}

		s = archive_entry_pathname (entry);
		pkg_entry = pkg_package_entry_new (s, (size_t) -1);
		pkg_entry->archive_entry = archive_entry_clone (entry);

		pkg_package_add_entry (pkg, pkg_entry);
	}

	archive_read_finish (archive);

	if (!contains_hardlinks)
		return true;

	archive = open_archive (file);
	if (!archive)
		return false;

	while (archive_read_next_header (archive, &entry) == ARCHIVE_OK) {
		PkgPackageEntry *pkg_entry, *pkg_entry2;
		const char *s, *hardlink;

		hardlink = archive_entry_hardlink (entry);
		if (!hardlink)
			continue;

		s = archive_entry_pathname (entry);
		pkg_entry = pkg_package_entry_new (s, (size_t) -1);
		pkg_entry->archive_entry = archive_entry_clone (entry);

		pkg_entry2 = bst_find (pkg->entries, find_entry_by_name_cb,
		                       (void *) hardlink);

		archive_entry_set_mode (pkg_entry->archive_entry,
		                        pkg_package_entry_get_mode (pkg_entry2));
		archive_entry_set_size (pkg_entry->archive_entry,
		                        pkg_package_entry_get_size (pkg_entry2));

		pkg_package_add_entry (pkg, pkg_entry);
	}

	archive_read_finish (archive);

	return true;
}

PKG_API
PkgPackage *
pkg_package_new_from_file (const char *file)
{
	PkgPackage *pkg;

	pkg = create_from_filename (file);
	if (!pkg)
		return NULL;

	if (!read_archive (pkg, file)) {
		bst_free (pkg->entries);
		free (pkg);
		return NULL;
	}

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

	bst_free (pkg->entries);

	free (pkg);
}

PKG_API
void
pkg_package_foreach (PkgPackage *pkg, PkgPackageForeachFunc func,
                     void *user_data)
{
	bst_foreach (pkg->entries, (BstForeachFunc) func, user_data);
}

PKG_API
void
pkg_package_foreach_reverse (PkgPackage *pkg,
                             PkgPackageForeachFunc func,
                             void *user_data)
{
	bst_foreach_reverse (pkg->entries, (BstForeachFunc) func, user_data);
}

bool
pkg_package_includes (PkgPackage *pkg, PkgPackageEntry *entry)
{
	return !!bst_find (pkg->entries, compare_entries_cb, entry);
}

void
pkg_package_add_entry (PkgPackage *pkg, PkgPackageEntry *entry)
{
	bst_insert (pkg->entries, entry);
}
