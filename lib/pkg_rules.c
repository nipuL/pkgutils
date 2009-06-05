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
#include <string.h>
#include <regex.h>
#include <limits.h>
#include <errno.h>

#include "pkg_rules.h"
#include "list.h"

char *lstrip(char *buf) {
  char *p = buf;
  while (p[0] == ' ' && p[0] != '\0')
    *p++;
  return p;
}

/* FIXME: Handle escaped spaces */
char *get_token(char *buf, char *token) {
  int n = 0;
  char *ptr = lstrip(buf);
  char *p = ptr;

  while (p[0] != ' ' && p[0] != '\0') {
    n++;
    *p++;
  }
  
  memcpy(token, ptr, n);
  token[n] = '\0';

  return p;
}    

PkgRuleType get_pkg_rule_type(char *string) {
  int i;
  for (i=0; i < N_RULE_TYPES; i++) {
    if (strcmp(string, PkgRuleTypeStrings[i]) == 0)
      return i;
  }
  return -1;
}

PKG_API
List *
pkg_rules_from_file(char *file, int *error) {
  PkgRule *rule;
  List *rules_list;
  FILE *fp;
  char buf[PATH_MAX];

  if ((rules_list = malloc (sizeof (List))) == NULL) {
    *error = errno;
    return NULL;
  }

  if ((fp = fopen(file, "r")) == NULL) {
    *error = errno;
    free (rules_list);
    return NULL;
  }

  while ((fgets (buf, sizeof (buf), fp))) {
    if ((rule = pkg_rule_from_string(buf)) != NULL)
      rules_list = list_prepend(rules_list, (void *)rule);
  }
  return rules_list;
}

PKG_API
PkgRule *
pkg_rule_from_string(char *string) {
  PkgRule *rule;
  char token[PKG_RULES_BUF_MAX], *ptr;
  regex_t re;
  
  if ((rule = malloc (sizeof (PkgRule))) == NULL)
    return NULL;

  /* First token is rule type */
  ptr = get_token(token, string);
  if ((rule->type = get_pkg_rule_type(token)) == -1)
    return NULL;

  /* Second token is regex */
  ptr = get_token(token, ptr);
  if ((regcomp(&re, token, 0)))
    return NULL;
  rule->regex = re;

  /* Remainder is unprocessed user data */
  rule->user_data = (void *) lstrip(ptr);

  return rule;
}
