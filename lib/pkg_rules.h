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

#ifndef __PKG_RULES_H
#define __PKG_RULES_H

#define PKG_RULES "etc/pkgadd.conf"

#define PKG_RULES_BUF_MAX 256

typedef enum { UPGRADE, INSTALL, N_RULE_TYPES } PkgRuleType;

const char *PkgRuleTypeStrings[N_RULE_TYPES] = { "INSTALL", "UPGRADE" };

typedef struct {
  PkgRuleType type;
  regex_t regex;
  void *user_data;
} PkgRule;

List *pkg_rule_list_from_file(char *file);
PkgRule *pkg_rule_from_string(char *string);

#endif // __PKG_RULES_H
