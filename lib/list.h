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

#ifndef __LIST_H
#define __LIST_H

typedef struct __List {
	struct __List *prev;
	struct __List *next;

	void *data;
} List;

typedef int (*ListCompareFunc) (void *a, void *b);

List *list_prepend (List *list, void *data);
List *list_find (List *list, void *data);
List *list_find_custom (List *list, ListCompareFunc func, void *user_data);
List *list_remove (List *list, void *data);
List *list_remove_link (List *list, List *link);
List *list_reverse (List *list);

#endif
