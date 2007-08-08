/*
 * pkgutils
 *
 * Copyright (c) 2000-2005 Per Liden
 * Copyright (c) 2005-2007 by CRUX team (http://crux.nu)
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
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define __COMMON_C
#include "common.h"

const char *progname, *root = "";

PkgDatabase *
open_db (bool exclusive)
{
	PkgDatabase *db;
	int error = 0;

	db = pkg_database_new (root, exclusive, &error);
	if (db)
		return db;

	fprintf (stderr, "cannot open database: %s\n", strerror (error));
	exit (1);
}

void
ignore_signals ()
{
	struct sigaction sa;

	memset (&sa, 0, sizeof (sa));

	sa.sa_handler = SIG_IGN;

	sigaction (SIGHUP, &sa, NULL);
	sigaction (SIGINT, &sa, NULL);
	sigaction (SIGQUIT, &sa, NULL);
	sigaction (SIGTERM, &sa, NULL);
}
