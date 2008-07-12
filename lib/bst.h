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

#ifndef __BST_H
#define __BST_H

#include <stdbool.h>

struct __Bst;
typedef struct __Bst Bst;

typedef int (*BstCompareFunc) (void *a, void *b);
typedef void (*BstNodeFreeFunc) (void *data);
typedef bool (*BstForeachFunc) (void *data, void *user_data);

Bst *bst_new (BstCompareFunc cmp_func, BstNodeFreeFunc free_func);
void bst_free (Bst *tree);
int bst_insert (Bst *tree, void *data);
void bst_foreach (Bst *tree, BstForeachFunc func, void *user_data);
void bst_foreach_reverse (Bst *tree, BstForeachFunc func, void *user_data);
void *bst_find (Bst *tree, BstCompareFunc func, void *data);
void *bst_remove (Bst *tree, BstCompareFunc compare_func, void *data);

#endif
