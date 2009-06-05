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
#include <ctype.h>
#include <string.h>

char *lstrip(char *buf) {
  while (*buf && (isspace(*buf) || isblank(*buf)))
    buf++;
  return buf;
  
}

#warning FIXME: Handle escaped spaces 
char *get_token(char *token, char *buf) {
  char *p1 = lstrip(buf);
  char *p2 = p1;
  int n = 0;

  while (*p2 && !(isspace(*p2) || isblank(*p2))) {
    p2++;
    n++;
  }

  memcpy(token, p1, n);

  return p2;
}
