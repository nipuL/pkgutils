/*
 * Pkgutils
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
#include <string.h>
#include <regex.h>
#include <limits.h>
#include <errno.h>

#include "pkg_rule.h"
#include "list.h"
#include "utils.h"

static const char *PkgRuleTypeStrings[N_RULE_TYPES] = { "INSTALL", "UPGRADE" };

PkgRuleType
get_pkg_rule_type (char *string)
{
	int i;
	for (i=0; i < N_RULE_TYPES; i++) {
		if (strcmp (string, PkgRuleTypeStrings[i]) == 0)
			return i;
	}

	return -1;
}

PKG_API
PkgRuleList
pkg_rule_list_from_file (const char *file, int *error)
{
	PkgRule *rule;
	PkgRuleList rules_list = NULL;
	FILE *fp;
	char buf[PATH_MAX];
	
	if ((fp = fopen (file, "r")) == NULL) {
		*error = errno;
		return NULL;
	}
	
	while ((fgets (buf, sizeof (buf), fp))) {
		if ((rule = pkg_rule_from_string (buf)) != NULL)
			rules_list = list_prepend (rules_list, (void *) rule);
	}
	
	return rules_list;
}

PKG_API
PkgRule *
pkg_rule_from_string(char *string) {
	PkgRule *rule;
	char token[PKG_RULE_BUF_MAX], *ptr;
	regex_t re;
	
	if ((rule = malloc (sizeof (PkgRule))) == NULL) {
		return NULL;
	}
	
	ptr = lstrip (string);
	
	if (*ptr == '#' || *ptr == '\n') {
		free (rule);
		return NULL;
	}
	
	/* First token is rule type */
	ptr = get_token (token, ptr);
	
	if ((rule->type = get_pkg_rule_type (token)) == -1) {
		free (rule);
		return NULL;
	}
	
	/* Second token is regex */
	ptr = get_token (token, ptr);
	if ((regcomp (&re, token, 0))) {
		free (rule);
		return NULL;
	}
	rule->regex = re;
	
	/* Remainder is unprocessed user data */
	rule->data = (void *) lstrip (ptr);
	
	return rule;
}
