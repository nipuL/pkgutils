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
#include "pkg_database_error.h"

#define PKG_DIR "var/lib/pkg"
#define PKG_DB "db"
#define PKG_DB_TMP PKG_DB ".incomplete_transaction"
#define PKG_DB_BAK PKG_DB ".backup"

typedef bool (*Predicate) (PkgPackage *node_data, void *user_data);

typedef struct {
	PkgDatabase *db;

	char dir_prev[PATH_MAX];
	size_t dir_prev_len;

	bool included;
} RemoveData;

typedef struct {
	Predicate predicate;

	void *user_data;
	bool found;
} HasPackageData;

static int
pkg_database_lock_init (PkgDatabaseLock *lock,
                        int root, bool exclusive)
{
	int s, flags = LOCK_NB;

	lock->dir = openat (root, PKG_DIR, O_RDONLY);
	if (lock->dir == -1)
		return errno;

	flags |= exclusive ? LOCK_EX : LOCK_SH;

	s = flock (lock->dir, flags);
	if (s == -1) {
		s = errno;
		close (lock->dir);
	}

	return s;
}

static void
pkg_database_lock_free (PkgDatabaseLock *lock)
{
	flock (lock->dir, LOCK_UN);
	close (lock->dir);
}

static int
compare_entries_cb (void *a, void *b)
{
	PkgPackage *aa = a;
	PkgPackage *bb = b;

	return strcmp (aa->name, bb->name);
}

PKG_API
PkgDatabase *
pkg_database_new (const char *root, bool exclusive, int *error)
{
	PkgDatabase *db;
	int s, fd;

	fd = open (root ? root : "/", O_RDONLY);
	if (fd == -1) {
		*error = errno;
		return NULL;
	}

	db = malloc (sizeof (PkgDatabase));
	if (!db)
		return NULL;

	db->root = fd;

	s = pkg_database_lock_init (&db->lock, fd, exclusive);
	if (s) {
		close (fd);
		free (db);
		*error = s;

		return NULL;
	}

	fd = openat (db->lock.dir, PKG_DB, O_RDONLY);
	if (fd == -1) {
		*error = errno;
		pkg_database_lock_free (&db->lock);
		close (db->root);
		free (db);

		return NULL;
	}

	db->fp = fdopen (fd, "r");
	db->refcount = 1;
	db->packages = bst_new (compare_entries_cb,
	                        (BstNodeFreeFunc) pkg_package_unref);

	*error = 0;

	return db;
}

PKG_API
PkgDatabase *
pkg_database_ref (PkgDatabase *db)
{
	db->refcount++;

	return db;
}

PKG_API
void
pkg_database_unref (PkgDatabase *db)
{
	if (--db->refcount)
		return;

	fclose (db->fp);
	pkg_database_lock_free (&db->lock);
	close (db->root);

	if (db->packages)
		bst_free (db->packages);

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
#warning FIXME: invalid 
				}

				memcpy (name, buf, buf_len + 1);
				state = STATE_VERSION;
				break;
			case STATE_VERSION:
				max_len = PKG_PACKAGE_MAX_VERSION_LEN + 1 +
				          PKG_PACKAGE_MAX_RELEASE_LEN;

				if (buf_len > max_len) {
#warning FIXME: invalid 
				}

				version = buf;

				release = strchr (version, '-');
				if (!release) {
#warning FIXME: invalid 
				}

				*release++ = 0;

				pkg = pkg_package_new (name, version, release);
				bst_insert (db->packages, pkg);

				state = STATE_FILES;
				break;
			case STATE_FILES:
				if (*buf && mode == PKG_DATABASE_READ_ALL) {
					PkgPackageEntry *entry;

					entry = pkg_package_entry_new (buf, buf_len);
					pkg_package_add_entry (pkg, entry);
				} else if (!*buf)
					state = STATE_NAME;

				break;
		}
	}
}

PKG_API
void
pkg_database_foreach (PkgDatabase *db, PkgDatabaseForeachFunc func,
                      void *user_data)
{
	bst_foreach (db->packages, (BstForeachFunc) func, user_data);
}

static int
find_package_cb (void *data, void *user_data)
{
	PkgPackage *pkg = user_data;
	const char *name = (const char *) data;

	return strcmp (name, pkg->name);
}

PKG_API
PkgPackage *
pkg_database_find (PkgDatabase *db, const char *name)
{
	PkgPackage *pkg;

	pkg = bst_find (db->packages, find_package_cb, (void *) name);

	return pkg ? pkg_package_ref (pkg) : NULL;
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
		pkg_package_add_entry (pkg, entry);
	}
}

static bool
path_is_referenced_cb (PkgPackage *pkg, void *user_data)
{
	const char *path = user_data;

	return pkg_package_includes_path (pkg, path);
}

static bool
entry_is_referenced_cb (PkgPackage *pkg, void *user_data)
{
	PkgPackageEntry *entry = user_data;

	return pkg_package_includes (pkg, entry);
}

static void
my_dirname (char *dir, size_t *dir_len)
{
	while ((*dir_len)--)
		if (dir[*dir_len] == '/') {
			dir[++(*dir_len)] = 0;
			return;
		}
}

static bool
has_package_cb (void *node_data, void *user_data)
{
	HasPackageData *has_package_data = user_data;

	has_package_data->found =
		has_package_data->predicate (node_data,
		                             has_package_data->user_data);

	return !has_package_data->found;
}

static bool
has_package (Bst *packages, Predicate predicate, void *user_data)
{
	HasPackageData has_package_data;

	has_package_data.predicate = predicate;
	has_package_data.user_data = user_data;
	has_package_data.found = false;

	bst_foreach (packages, has_package_cb, &has_package_data);

	return has_package_data.found;
}

/* checks whether the passed entry is referenced by another
 * package in the database.
 *
 * the shortcut we're taking here is that if eg /usr/bin isn't
 * referenced by any other package, then /usr/bin/foo cannot
 * be referenced either and we don't need to call
 * pkg_package_includes() on it.
 */
static bool
entry_is_referenced (PkgPackageEntry *entry, RemoveData *data)
{
	/* first, check whether this entry is a child of the directory
	 * that we examined earlier.
	 */
	if (data->dir_prev_len &&
	    !strncmp (data->dir_prev, entry->name, data->dir_prev_len)) {

		/* if the parent isn't included, the child cannot be included
		 * either.
		 */
		if (!data->included)
			return false;
	} else {
		char dir[PATH_MAX];
		size_t dir_len;

		/* get the directory part of the entry */
		memcpy (dir, entry->name, entry->name_len + 1);
		dir_len = entry->name_len;
		my_dirname (dir, &dir_len);

		memcpy (data->dir_prev, dir, dir_len + 1);
		data->dir_prev_len = dir_len;

		/* if the entry is not a directory, check whether the directory
		 * is referenced by another package, and store that test result.
		 */
		if (entry->name[entry->name_len - 1] != '/') {
			data->included = has_package (data->db->packages,
			                              path_is_referenced_cb,
			                              &dir[1]);
			if (!data->included)
				return false;
		}
	}

	/* as a last resort, we check whether the actual entry is
	 * referenced by another package.
	 */
	return has_package (data->db->packages,
	                    entry_is_referenced_cb, entry);
}

static bool
remove_file_cb (PkgPackageEntry *entry, void *user_data)
{
	RemoveData *data = user_data;
	PkgDatabase *db = data->db;
	struct stat st;
	int s, flags = 0;

	if (entry_is_referenced (entry, data))
		return true; /* keep going */

	s = fstatat (db->root, &entry->name[1], &st, AT_SYMLINK_NOFOLLOW);
	if (s)
		return true; /* keep going */

	if (S_ISDIR (st.st_mode))
		flags = AT_REMOVEDIR;

	s = unlinkat (db->root, &entry->name[1], flags);
	if (s == -1)
		fprintf (stderr, "could not remove '%s': %s (%i)\n",
		         &entry->name[1], strerror (errno), errno);

	return true; /* keep going */
}

static bool
write_package_cb2 (PkgPackageEntry *entry, void *user_data)
{
	FILE *fp = user_data;

	fprintf (fp, "%s\n", &entry->name[1]);

	return true; /* keep going */
}

static bool
write_package_cb (PkgPackage *pkg, void *user_data)
{
	FILE *fp = user_data;

	fprintf (fp, "%s\n%s-%s\n", pkg->name, pkg->version, pkg->release);
	pkg_package_foreach (pkg, write_package_cb2, fp);
	fprintf (fp, "\n");

	return true; /* keep going */
}

static bool
database_commit (PkgDatabase *db)
{
	FILE *fp;
	int fd;

	fd = openat (db->lock.dir, PKG_DB_TMP,
	             O_CREAT | O_TRUNC | O_WRONLY, 0444);
	if (fd == -1)
		return false;

	fp = fdopen (fd, "w");

	pkg_database_foreach (db, write_package_cb, fp);

	fflush (fp);
	fsync (fd);
	fclose (fp);

	unlinkat (db->lock.dir, PKG_DB_BAK, 0);

	linkat (db->lock.dir, PKG_DB, db->lock.dir, PKG_DB_BAK, 0);
	renameat (db->lock.dir, PKG_DB_TMP, db->lock.dir, PKG_DB);

	return true;
}

PKG_API
int
pkg_database_add (PkgDatabase *db, PkgPackage *pkg, bool upgrade, bool force)
{
	PkgPackage *pkg2;

	/* find the PkgPackage object for this package */
	pkg2 = bst_find (db->packages, find_package_cb, pkg->name);
	if (pkg2) {
	  if (upgrade)
	    return PKG_DATABASE_TODO;
	  return PKG_DATABASE_PKG_INSTALLED;
	}

	if (!pkg_package_extract (pkg, db->root))
	  return !(int) false;

	if (pkg2) {
		bst_remove (db->packages, find_package_cb, pkg->name);
		pkg_package_unref (pkg2);
	}

	bst_insert (db->packages, pkg_package_ref (pkg));

	return !(int) database_commit (db);
}

PKG_API
int
pkg_database_remove (PkgDatabase *db, const char *name)
{
	PkgPackage *pkg;
	RemoveData remove_data;

	pkg = bst_remove (db->packages, find_package_cb, (void *) name);
	if (!pkg)
		return PKG_DATABASE_PKG_NOT_FOUND;

	/* remove the files/directories in this package */
	remove_data.db = db;
	remove_data.dir_prev[0] = 0;
	remove_data.dir_prev_len = 0;

	pkg_package_foreach_reverse (pkg, remove_file_cb, &remove_data);
	pkg_package_unref (pkg);

	return !(int) database_commit (db);
}
