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
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "common.h"

int pkginfo (int argc, char **argv);
int pkgrm (int argc, char **argv);

int
main (int argc, char **argv)
{
	progname = basename (argv[0]);

	if (!strcmp (progname, "pkginfo"))
		return pkginfo (argc, argv);
	else if (!strcmp (progname, "pkgrm"))
		return pkgrm (argc, argv);
	else {
		fprintf (stderr, "unknown program '%s'\n", progname);
		return EXIT_FAILURE;
	}
}
