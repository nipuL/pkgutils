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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <regex.h>
#include <unistd.h>

#include "common.h"

typedef struct {
	regex_t regex;
	int matches;
	PkgPackage *pkg;
} ListOwnerData;

static void
show_usage ()
{
	const char *s =
		"usage: pkginfo [options]\n"
		" -i, --installed             list installed packages\n"
		" -l, --list <package|file>   list files in <package> or <file>\n"
		" -o, --owner <pattern>       list owner(s) of file(s) matching <pattern>\n"
		" -f, --footprint <file>      print footprint for <file>\n"
		" -r, --root <path>           specify alternative installation root\n"
		" -v, --version               print version and exit\n"
		" -h, --help                  print help and exit\n"
	;

	printf (s);
}

static void
list_installed_cb (PkgPackage *pkg, void *user_data)
{
	printf ("%s %s-%s\n", pkg->name, pkg->version, pkg->release);
}

static int
list_installed ()
{
	PkgDatabase *db;

	db = open_db (false);

	pkg_database_read_package_list (db, PKG_DATABASE_READ_NAMES_ONLY);
	pkg_database_foreach (db, list_installed_cb, NULL);
	pkg_database_unref (db);

	return 0;
}

static void
list_files_cb (PkgPackageEntry *entry, void *user_data)
{
	printf ("%s\n", entry->name);
}

static int
list_files (const char *package)
{
	PkgDatabase *db;
	PkgPackage *pkg;
	int ret = 0;

	db = open_db (false);

	pkg_database_read_package_list (db, PKG_DATABASE_READ_NAMES_ONLY);

	pkg = pkg_database_find (db, package);
	if (!pkg) {
		fprintf (stderr, "package '%s' not found\n", package);
		pkg_database_unref (db);
		return 1;
	}

	pkg_database_fill_package_files (db, pkg);

	pkg_package_foreach (pkg, list_files_cb, NULL);

	pkg_package_unref (pkg);
	pkg_database_unref (db);

	return ret;
}

static void
list_owners_cb2 (PkgPackageEntry *entry, void *user_data)
{
	ListOwnerData *data = user_data;

	if (!regexec (&data->regex, entry->name, 0, 0, 0)) {
		data->matches++;
		printf ("%s %s\n", data->pkg->name, entry->name);
	}
}

static void
list_owners_cb (PkgPackage *pkg, ListOwnerData *data)
{
	data->pkg = pkg;

	pkg_package_foreach (pkg, list_owners_cb2, data);
}

static int
list_owners (const char *pattern)
{
	PkgDatabase *db;
	ListOwnerData data = {
		.matches = 0
	};

	if (regcomp (&data.regex, pattern, REG_EXTENDED | REG_NOSUB)) {
		fprintf (stderr, "invalid pattern '%s'\n", pattern);
		return 1;
	}

	db = open_db (false);

	pkg_database_read_package_list (db, PKG_DATABASE_READ_ALL);
	pkg_database_foreach (db, (PkgDatabaseForeachFunc) list_owners_cb,
	                      &data);
	pkg_database_unref (db);

	if (!data.matches)
		fprintf (stderr, "no owner(s) found\n");

	regfree (&data.regex);

	return 0;
}

static void
print_mode (mode_t mode, char *ptr)
{
	/* File type */
	switch (mode & S_IFMT) {
		case S_IFREG:
			*ptr++ = '-';
			break; // Regular
		case S_IFDIR:
			*ptr++ = 'd';
			break; // Directory
		case S_IFLNK:
			*ptr++ = 'l';
			break; // Symbolic link
		case S_IFCHR:
			*ptr++ = 'c';
			break; // Character special
		case S_IFBLK:
			*ptr++ = 'b';
			break; // Block special
		case S_IFSOCK:
			*ptr++= 's';
			break; // Socket
		case S_IFIFO:
			*ptr++ = 'p';
			break; // Fifo
		default:
			*ptr++ = '?';
			break; // Unknown
	}

	// User permissions
	*ptr++ = (mode & S_IRUSR) ? 'r' : '-';
	*ptr++ = (mode & S_IWUSR) ? 'w' : '-';

	switch (mode & (S_IXUSR | S_ISUID)) {
		case S_IXUSR:
			*ptr++ = 'x';
			break;
		case S_ISUID:
			*ptr++ = 'S';
			break;
		case S_IXUSR | S_ISUID:
			*ptr++ = 's';
			break;
		default:
			*ptr++ = '-';
			break;
	}

	// Group permissions
	*ptr++ = (mode & S_IRGRP) ? 'r' : '-';
	*ptr++ = (mode & S_IWGRP) ? 'w' : '-';

	switch (mode & (S_IXGRP | S_ISGID)) {
		case S_IXGRP:
			*ptr++ = 'x';
			break;
		case S_ISGID:
			*ptr++ = 'S';
			break;
		case S_IXGRP | S_ISGID:
			*ptr++ = 's';
			break;
		default:
			*ptr++ = '-';
			break;
	}

	// Other permissions
	*ptr++ = (mode & S_IROTH) ? 'r' : '-';
	*ptr++ = (mode & S_IWOTH) ? 'w' : '-';

	switch (mode & (S_IXOTH | S_ISVTX)) {
		case S_IXOTH:
			*ptr++ = 'x';
			break;
		case S_ISVTX:
			*ptr++ = 'T';
			break;
		case S_IXOTH | S_ISVTX:
			*ptr++ = 't';
			break;
		default:
			*ptr++ = '-';
			break;
	}

	*ptr = 0;
}

static void
list_footprint_cb (PkgPackageEntry *entry, void *user_data)
{
	uid_t uid;
	gid_t gid;
	struct passwd *pw;
	struct group *gr;
	char mode_buf[16], user[32], group[32];
	mode_t mode;

	mode = pkg_package_entry_get_mode (entry);
	print_mode (mode, mode_buf);

	uid = pkg_package_entry_get_uid (entry);
	pw = getpwuid (uid);
	if (pw) {
		strncpy (user, pw->pw_name, sizeof (user));
		user[sizeof (user) - 1] = 0;
	} else
		sprintf (user, "%i", uid);

	gid = pkg_package_entry_get_gid (entry);
	gr = getgrgid (gid);
	if (gr) {
		strncpy (group, gr->gr_name, sizeof (group));
		group[sizeof (group) - 1] = 0;
	} else
		sprintf (group, "%i", gid);

	printf ("%s\t%s/%s\t%s", mode_buf, user, group,
	        &entry->name[1]);

	if (S_ISLNK (mode))
		printf (" -> %s\n", pkg_package_entry_get_symlink_target (entry));
	else if (S_ISCHR (mode) || S_ISBLK (mode)) {
		dev_t d;

		d = pkg_package_entry_get_dev (entry);
		printf (" (%i, %i)\n", major (d), minor (d));
	} else if (S_ISREG (mode) && !pkg_package_entry_get_size (entry))
		printf (" (EMPTY)\n");
	else
		printf ("\n");
}

static int
list_footprint (const char *file)
{
	PkgPackage *pkg;

	pkg = pkg_package_new_from_file (file);
	if (!pkg) {
		fprintf (stderr, "cannot open package '%s'\n", file);
		return 1;
	}

	pkg_package_foreach (pkg, list_footprint_cb, NULL);
	pkg_package_unref (pkg);

	return 0;
}

int
pkginfo (int argc, char **argv)
{
	int c;
	struct option options[] = {
		{ "installed", no_argument, NULL, 'i' },
		{ "list", required_argument, NULL, 'l' },
		{ "owner", required_argument, NULL, 'o' },
		{ "footprint", required_argument, NULL, 'f' },
		{ "root", required_argument, NULL, 'r' },
		{ "version", no_argument, NULL, 'v' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};

	while (true) {
		c = getopt_long (argc, argv, "il:o:f:r:vh", options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case '?':
				return 1;
			case 'i':
				return list_installed ();
			case 'l':
				return list_files (optarg);
			case 'o':
				return list_owners (optarg);
			case 'f':
				return list_footprint (optarg);
			case 'r':
				root = optarg;
				break;
			case 'h':
				show_usage ();
				return 0;
			case 'v':
				printf ("%s (pkgutils) " VERSION "\n", progname);
				return 0;
		}
	}

	show_usage ();

	return 1;
}
