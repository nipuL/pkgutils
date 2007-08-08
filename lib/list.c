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

#include "list.h"

List *
list_prepend (List *list, void *data)
{
	List *item;

	item = malloc (sizeof (List));
	if (!item)
		return list;

	item->data = data;
	item->next = list;
	item->prev = NULL;

	if (list)
		list->prev = item;

	return item;
}

List *
list_find (List *list, void *data)
{
	for (List *l = list; l; l = l->next)
		if (l->data == data)
			return l;

	return NULL;
}

List *
list_find_custom (List *list, ListCompareFunc func, void *user_data)
{
	for (List *l = list; l; l = l->next)
		if (!func (l->data, user_data))
			return l;

	return NULL;
}

List *
list_remove (List *list, void *data)
{
	List *l;

	l = list_find (list, data);

	return list_remove_link (list, l);
}

List *
list_remove_link (List *list, List *link)
{
	List *p, *n;

	if (!link)
		return list;

	p = link->prev;
	n = link->next;

	if (p)
		p->next = n;
	else
		list = n;

	if (n)
		n->prev = p;

	free (link);

	return list;
}

List *
list_reverse (List *list)
{
	List *ret, *next = NULL;

	for (ret = list; list; list = list->prev) {
		list->prev = list->next;
		list->next = next;

		ret = next = list;
	}

	return ret;
}
