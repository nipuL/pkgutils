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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include "pkg_rules.h"
#include "list.h"

PKG_API
List *
pkg_rules_from_file(char *file, int *error) {
  PkgRule *rule;
  List *rules = malloc (sizeof (List));
  FILE *fp;
  char buf[PATH_MAX];

  if ((fp = fopen(file, "r")) == NULL) {
    *error = errno;
    free (rules);
    return NULL;
  }

  while ((fgets (buf, sizeof (buf), fp))) {
    if ((rule = pkg_rule_from_string(buf)) != NULL)
      rules = list_prepend(rules, rule);
  }
  return rules;
}

PKG_API
PkgRule *
pkg_rule_from_string(char *string) {
  return NULL;
}
