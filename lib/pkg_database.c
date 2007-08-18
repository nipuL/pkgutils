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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "pkg_database.h"

#define PKG_DIR "/var/lib/pkg"
#define PKG_DB "/var/lib/pkg/db"
#define PKG_DB_TMP PKG_DB ".incomplete_transaction"
#define PKG_DB_BAK PKG_DB ".backup"

static int
pkg_database_lock_init (PkgDatabaseLock *lock,
                        const char *root, size_t root_len,
                        bool exclusive)
{
	char path[PATH_MAX];
	int s, flags = LOCK_NB;

	/* concat root + PKG_DIR.
	 * the caller guarantees that this won't exceed PATH_MAX bytes.
	 */
	memcpy (mempcpy (path, root, root_len), PKG_DIR, sizeof (PKG_DIR));

	lock->dir = opendir (path);
	if (!lock->dir)
		return errno;

	flags |= exclusive ? LOCK_EX : LOCK_SH;

	s = flock (dirfd (lock->dir), flags);
	if (s == -1) {
		s = errno;
		closedir (lock->dir);
	}

	return s;
}

static void
pkg_database_lock_free (PkgDatabaseLock *lock)
{
	flock (dirfd (lock->dir), LOCK_UN);
	closedir (lock->dir);
}

PKG_API
PkgDatabase *
pkg_database_new (const char *root, bool exclusive, int *error)
{
	PkgDatabase *db;
	char path[PATH_MAX] = "";
	size_t root_len;
	int s;

	if (root) {
		root_len = strlen (root);

		if (root_len > (PATH_MAX - 1 - strlen (PKG_DB_TMP))) {
			*error = ENAMETOOLONG;
			return NULL;
		}
	} else {
		root = "";
		root_len = 0;
	}

	db = malloc (sizeof (PkgDatabase) + root_len + 1);
	if (!db)
		return NULL;

	s = pkg_database_lock_init (&db->lock, root, root_len, exclusive);
	if (s) {
		free (db);
		*error = s;

		return NULL;
	}

	memcpy (db->root, root, root_len);
	db->root_len = root_len;

	memcpy (mempcpy (path, root, root_len), PKG_DB, sizeof (PKG_DB));

	db->fp = fopen (path, "r");
	if (!db->fp) {
		*error = errno;
		pkg_database_lock_free (&db->lock);
		free (db);

		return NULL;
	}

	db->refcount = 1;
	db->packages = NULL;

	*error = 0;

	return db;
}

PKG_API
void
pkg_database_ref (PkgDatabase *db)
{
	db->refcount++;
}

PKG_API
void
pkg_database_unref (PkgDatabase *db)
{
	if (--db->refcount)
		return;

	fclose (db->fp);
	pkg_database_lock_free (&db->lock);

	while (db->packages) {
		pkg_package_unref (db->packages->data);
		db->packages = list_remove_link (db->packages, db->packages);
	}

	free (db);
}

PKG_API
void
pkg_database_read_package_list (PkgDatabase *db,
                                PkgDatabaseReadMode mode)
{
	PkgPackage *pkg = NULL;
	char buf[PATH_MAX];
	char name[PKG_PACKAGE_MAX_NAME_LEN + 1] = "";

	enum {
		STATE_NAME,
		STATE_VERSION,
		STATE_FILES,
	} state = STATE_NAME;

	fseek (db->fp, 0, SEEK_SET);

	while ((fgets (buf, sizeof (buf), db->fp))) {
		char *version, *release;
		size_t buf_len, max_len;

		buf_len = strlen (buf) - 1;
		buf[buf_len] = 0;

		switch (state) {
			case STATE_NAME:
				max_len = PKG_PACKAGE_MAX_NAME_LEN;

				if (buf_len > max_len) {
					/* FIXME: invalid */
				}

				memcpy (name, buf, buf_len + 1);
				state = STATE_VERSION;
				break;
			case STATE_VERSION:
				max_len = PKG_PACKAGE_MAX_VERSION_LEN + 1 +
				          PKG_PACKAGE_MAX_RELEASE_LEN;

				if (buf_len > max_len) {
					/* FIXME: invalid */
				}

				version = buf;

				release = strchr (version, '-');
				if (!release) {
					/* FIXME: invalid */
				}

				*release++ = 0;

				pkg = pkg_package_new (name, version, release);
				db->packages = list_prepend (db->packages, pkg);

				state = STATE_FILES;
				break;
			case STATE_FILES:
				if (*buf && mode == PKG_DATABASE_READ_ALL) {
					PkgPackageEntry *entry;

					entry = pkg_package_entry_new (buf, buf_len);
					pkg->entries = list_prepend (pkg->entries, entry);
				} else if (!*buf) {
					state = STATE_NAME;

					if (pkg)
						pkg->entries = list_reverse (pkg->entries);
				}

				break;
		}
	}

	db->packages = list_reverse (db->packages);
}

PKG_API
void
pkg_database_foreach (PkgDatabase *db, PkgDatabaseForeachFunc func,
                      void *user_data)
{
	for (List *l = db->packages; l; l = l->next)
		func (l->data, user_data);
}

static int
find_package_cb (void *data, void *user_data)
{
	PkgPackage *pkg = data;
	const char *name = (const char *) user_data;

	return strcmp (pkg->name, name);
}

PKG_API
PkgPackage *
pkg_database_find (PkgDatabase *db, const char *name)
{
	PkgPackage *pkg;
	List *l;

	l = list_find_custom (db->packages, find_package_cb, (void *) name);
	if (!l)
		return NULL;

	pkg = l->data;

	pkg_package_ref (pkg);

	return pkg;
}

PKG_API
void
pkg_database_fill_package_files (PkgDatabase *db, PkgPackage *pkg)
{
	char buf[PATH_MAX];
	bool found = false, found_version = false;

	fseek (db->fp, 0, SEEK_SET);

	while ((fgets (buf, sizeof (buf), db->fp))) {
		PkgPackageEntry *entry;
		size_t buf_len;

		buf_len = strlen (buf) - 1;
		buf[buf_len] = 0;

		if (!found) {
			found = !strcmp (buf, pkg->name);
			continue;
		}

		if (!found_version) {
			found_version = true;
			continue;
		}

		if (!*buf)
			break;

		entry = pkg_package_entry_new (buf, buf_len);
		pkg->entries = list_prepend (pkg->entries, entry);
	}

	pkg->entries = list_reverse (pkg->entries);
}

static int
entry_is_referenced_cb (void *data, void *user_data)
{
	PkgPackage *pkg = data;
	PkgPackageEntry *entry = user_data;

	return !pkg_package_includes (pkg, entry->name, entry->name_len);
}

static void
remove_file_cb (PkgPackageEntry *entry, void *user_data)
{
	PkgDatabase *db = user_data;
	char path[PATH_MAX];
	struct stat st;
	int s;

	if (list_find_custom (db->packages, entry_is_referenced_cb, entry))
		return;

	memcpy (mempcpy (path, db->root, db->root_len),
	        entry->name, entry->name_len + 1);

	if (lstat (path, &st))
		return;

	s = remove (path);
	if (s == -1)
		fprintf (stderr, "could not remove '%s': %s (%i)\n",
		         path, strerror (errno), errno);
}

static void
write_package_cb2 (PkgPackageEntry *entry, void *user_data)
{
	FILE *fp = user_data;

	fprintf (fp, "%s\n", &entry->name[1]);
}

static void
write_package_cb (PkgPackage *pkg, void *user_data)
{
	FILE *fp = user_data;

	fprintf (fp, "%s\n%s-%s\n", pkg->name, pkg->version, pkg->release);
	pkg_package_foreach (pkg, write_package_cb2, fp);
	fprintf (fp, "\n");
}

static bool
database_commit (PkgDatabase *db)
{
	FILE *fp;
	char path[PATH_MAX], path_tmp[PATH_MAX], path_bak[PATH_MAX];

	/* create the temporary database */
	memcpy (mempcpy (path_tmp, db->root, db->root_len),
	        PKG_DB_TMP, sizeof (PKG_DB_TMP));

	fp = fopen (path_tmp, "w");
	if (!fp)
		return false;

	pkg_database_foreach (db, write_package_cb, fp);

	fflush (fp);
	fsync (fileno (fp));
	fclose (fp);

	memcpy (mempcpy (path, db->root, db->root_len),
	        PKG_DB, sizeof (PKG_DB));

	memcpy (mempcpy (path_bak, db->root, db->root_len),
	        PKG_DB_BAK, sizeof (PKG_DB_BAK));

	unlink (path_bak);
	link (path, path_bak);

	rename (path_tmp, path);

	return true;
}

PKG_API
bool
pkg_database_remove (PkgDatabase *db, const char *name)
{
	PkgPackage *pkg;
	List *link;

	/* find the PkgPackage object for this package */
	link = list_find_custom (db->packages, find_package_cb,
	                         (void *) name);
	if (!link)
		return false;

	pkg = link->data;

	db->packages = list_remove_link (db->packages, link);

	/* remove the files/directories in this package */
	pkg_package_foreach_reverse (pkg, remove_file_cb, db);
	pkg_package_unref (pkg);

	return database_commit (db);
}
