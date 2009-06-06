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

bool force, upgrade;

static void
show_usage ()
{
	const char *s =
		"usage: pkgadd [options] <file>\n"
		" -u, --upgrade               upgrade package with the same name\n"
		" -f, --force                 force install, overwrite conflicting files\n"
		" -r, --root <path>           specify alternative installation root\n"
		" -v, --version               print version and exit\n"
		" -h, --help                  print help and exit\n"
	;

	printf (s);
}

static int
doit (const char *file)
{
	PkgDatabase *db;
	PkgPackage *pkg;
	PkgRuleList *rule_list;

	int ret = 0;

	pkg = pkg_package_new_from_file (file);
	if (!pkg) {
		fprintf (stderr, "cannot open package '%s'\n", file);
		return 1;
	}

	rule_list = pkg_rule_list_from_file (PKG_RULES);

	if (rule_list)
		pkg_rule_apply (pkg, rule_list);

	ignore_signals ();

	db = open_db (true);

	pkg_database_read_package_list (db, PKG_DATABASE_READ_ALL);

	if ((ret = pkg_database_add (db, pkg, upgrade, force))) {
		fprintf (stderr, "could not install package '%s'\n", file);
	}

	pkg_package_unref (pkg);
	pkg_database_unref (db);

	return ret;
}

int
pkgadd (int argc, char **argv)
{
	int c;
	struct option options[] = {
		{ "upgrade", no_argument, NULL, 'u' },
		{ "force", no_argument, NULL, 'f' },
		{ "root", required_argument, NULL, 'r' },
		{ "version", no_argument, NULL, 'v' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};

	while (true) {
		c = getopt_long (argc, argv, "ufr:vh", options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case '?':
				return 1;
			case 'u':
				upgrade = true;
				break;
			case 'f':
				force = true;
				break;
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

	if ((argc - optind) != 1) {
		show_usage ();
		return 1;
	}

	return doit (argv[optind]);
}
