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
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "bst.h"

#define MAX_HEIGHT 16

/* single rotation */
#define ROT1(left, right) \
	do { \
		new_root = old_root->left; \
		old_root->left = new_root->right; \
		new_root->right = old_root; \
		old_root->balance = new_root->balance = 0; \
	} while (0)

/* double rotation */
#define ROT2(left, right) \
	do { \
		new_root = old_root->left->right; \
		new_root->left = old_root->left; \
\
		old_root->left->right = new_root->left; \
		old_root->left = new_root->right; \
\
		new_root->right = old_root; \
\
		new_root->left->balance = new_root->right->balance = 0; \
\
		switch (new_root->balance) { \
			case -1: \
				new_root->left->balance = -1; \
				break; \
			case 1: \
				new_root->right->balance = 1; \
				break; \
		} \
	} while (0)

#define TRAVERSE(left, right) \
	BstNode *path[MAX_HEIGHT * 2], *node = tree->root; \
	int i = 0; \
\
	while (node) { \
		while (node) { \
			if (node->right) \
				path[i++] = node->right; \
\
			path[i++] = node; \
			node = node->left; \
		} \
\
		node = path[--i]; \
\
		while (i && !node->right) { \
			func (node->data, user_data); \
			node = path[--i]; \
		} \
\
		func (node->data, user_data); \
\
		node = i ? path[--i] : NULL; \
	}

typedef struct __BstNode {
	struct __BstNode *left;
	struct __BstNode *right;

	int8_t balance;

	void *data;
} BstNode;

struct __Bst {
	BstNode *root;
	BstCompareFunc compare_func;
	BstNodeFreeFunc free_func;
};

Bst *
bst_new (BstCompareFunc cmp_func, BstNodeFreeFunc free_func)
{
	Bst *tree;

	tree = malloc (sizeof (Bst));

	tree->root = NULL;
	tree->compare_func = cmp_func;
	tree->free_func = free_func;

	return tree;
}

void
bst_free (Bst *tree)
{
	BstNode *save, *node;

	for (node = tree->root; node; node = save) {
		if (node->left) {
			save = node->left;
			node->left = save->right;
			save->right = node;
		} else {
			save = node->right;

			tree->free_func (node->data);
			free (node);
		}
	}

	free (tree);
}

static BstNode *
node_new (void *data)
{
	BstNode *node;

	node = malloc (sizeof (BstNode));
	if (!node)
		return NULL;

	node->left = node->right = NULL;
	node->balance = 0;
	node->data = data;

	return node;
}

/* rebalance the given subtree.
 * return the new root of that subtree.
 */
static BstNode *
tree_rebalance (BstNode *old_root)
{
	BstNode *new_root = old_root;

	if (old_root->balance > 1) {
		if (old_root->left->balance > 0)
			ROT1 (left, right); /* ll */
		else
			ROT2 (left, right); /* lr */
	} else if (old_root->balance < -1) {
		if (old_root->right->balance < 0)
			ROT1 (right, left); /* rr */
		else
			ROT2 (right, left); /* rl */
	}

	return new_root;
}

int
bst_insert (Bst *tree, void *data)
{
	BstNode *node, *node2, *top, *node_parent, *top_parent;
	int8_t c_top[MAX_HEIGHT];
	int i = 0;

	if (!tree->root) {
		tree->root = node_new (data);

		return 0;
	}

	node = top = tree->root;
	node2 = node_parent = top_parent = NULL;

	while (!node2) {
		int c = tree->compare_func (data, node->data);

		if (!c)
			return EEXIST;

		if (node->balance) {
			top = node;
			top_parent = node_parent;
			i = 0;
		}

		c_top[i++] = c;

		if (c < 0) {
			if (node->left) {
				node_parent = node;
				node = node->left;
			} else
				node->left = node2 = node_new (data);
		} else {
			if (node->right) {
				node_parent = node;
				node = node->right;
			} else
				node->right = node2 = node_new (data);
		}
	};

	/* fix up balance values */
	for (i = 0, node = top; node != node2; i++)
		if (c_top[i] < 0) {
			node->balance++;
			node = node->left;
		} else {
			node->balance--;
			node = node->right;
		}

	/* rebalance the tree */
	if (!top_parent)
		tree->root = tree_rebalance (top);
	else if (top_parent->left == top)
		top_parent->left = tree_rebalance (top);
	else
		top_parent->right = tree_rebalance (top);

	return 0;
}

void
bst_foreach (Bst *tree, BstForeachFunc func, void *user_data)
{
	TRAVERSE (left, right);
}

void
bst_foreach_reverse (Bst *tree, BstForeachFunc func, void *user_data)
{
	TRAVERSE (right, left);
}

bool
bst_includes (Bst *tree, void *data)
{
	BstNode *node = tree->root;

	while (node) {
		int c = tree->compare_func (data, node->data);

		if (!c)
			return true;
		else if (c < 0)
			node = node->left;
		else
			node = node->right;
	}

	return false;
}
