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

#include <regex.h>
#include "list.h"
#include "pkg_package.h"

#ifndef __PKG_RULE_H
#define __PKG_RULE_H

#define PKG_RULES "etc/pkgadd.conf"

#define PKG_RULE_BUF_MAX 256

typedef List PkgRuleList;

typedef enum { UPGRADE, INSTALL, N_RULE_TYPES } PkgRuleType;

typedef struct {
	PkgRuleType type;
	regex_t regex;
	void *data;
} PkgRule;

PkgRuleList *pkg_rule_list_from_file (const char *file);
PkgRule *pkg_rule_from_string (char *string);

void pkg_rule_apply(PkgPackage *pkg, PkgRuleList *rule_list);
#endif
