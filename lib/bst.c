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
	} while (0)

/* double rotation */
#define ROT2(left, right) \
	do { \
		new_root = old_root->left->right; \
		old_root->left->right = new_root->left; \
		new_root->left = old_root->left; \
		old_root->left = new_root->right; \
		new_root->right = old_root; \
\
		new_root->left->balance = new_root->right->balance = 0; \
\
		switch (new_root->balance) { \
			case -1: \
				new_root->left->balance = 1; \
				break; \
			case 1: \
				new_root->right->balance = -1; \
				break; \
		} \
\
		new_root->balance = 0; \
	} while (0)

#define TRAVERSE(left, right) \
	BstNode *path[MAX_HEIGHT * 2], *node = tree->root; \
	bool keep_going; \
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
			keep_going = func (node->data, user_data); \
			if (!keep_going) \
				return; \
\
			node = path[--i]; \
		} \
\
		keep_going = func (node->data, user_data); \
		if (!keep_going) \
			return; \
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

/* performs an LL rotation at the given subtree and returns
 * the new root of that subtree.
 */
static BstNode *
rotate_ll (BstNode *old_root)
{
	BstNode *new_root;

	ROT1 (left, right);

	return new_root;
}

/* performs an RR rotation at the given subtree and returns
 * the new root of that subtree.
 */
static BstNode *
rotate_rr (BstNode *old_root)
{
	BstNode *new_root;

	ROT1 (right, left);

	return new_root;
}

/* performs an LR rotation at the given subtree and returns
 * the new root of that subtree.
 */
static BstNode *
rotate_lr (BstNode *old_root)
{
	BstNode *new_root;

	ROT2 (left, right);

	return new_root;
}

/* performs an RL rotation at the given subtree and returns
 * the new root of that subtree.
 */
static BstNode *
rotate_rl (BstNode *old_root)
{
	BstNode *new_root;

	ROT2 (right, left);

	return new_root;
}

/* rebalance the given subtree.
 * return the new root of that subtree.
 */
static BstNode *
tree_rebalance (BstNode *old_root)
{
	BstNode *new_root = old_root;

	if (old_root->balance > 1) {
		if (old_root->left->balance > 0) {
			new_root = rotate_ll (old_root);
			old_root->balance = new_root->balance = 0;
		} else
			new_root = rotate_lr (old_root);
	} else if (old_root->balance < -1) {
		if (old_root->right->balance < 0) {
			new_root = rotate_rr (old_root);
			old_root->balance = new_root->balance = 0;
		} else
			new_root = rotate_rl (old_root);
	}

	return new_root;
}

int
bst_insert (Bst *tree, void *data)
{
	BstNode *node, *node2, *top, *node_parent, *top_parent;
	int i = 0, c_top[MAX_HEIGHT];

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

void *
bst_find (Bst *tree, BstCompareFunc compare_func, void *data)
{
	BstNode *node = tree->root;

	while (node) {
		int c = compare_func (data, node->data);

		if (!c)
			return node->data;
		else if (c < 0)
			node = node->left;
		else
			node = node->right;
	}

	return NULL;
}

static BstNode **
get_attach_point (BstNode *node, BstNode *parent)
{
	return (parent->left == node) ? &parent->left : &parent->right;
}

void *
bst_remove (Bst *tree, BstCompareFunc compare_func, void *data)
{
	BstNode **dest, *path[MAX_HEIGHT], *node;
	void *removed_data;
	int c_top[MAX_HEIGHT], i = 0;

	/* find the node to remove, and store the path to that node */
	for (node = tree->root; node;) {
		int c = compare_func (data, node->data);

		if (!c)
			break;

		c_top[i] = c;
		path[i++] = node;

		if (c < 0)
			node = node->left;
		else
			node = node->right;
	}

	if (!node)
		return NULL;

	/* find out where 'node' is attached */
	dest = i ? get_attach_point (node, path[i - 1]) : &tree->root;

	/* the three cases of BST node removal follow */

	if (!node->right) {
		/* easiest case: 'node' doesn't have a right child.
		 * we can just replace 'node' by its left child.
		 */
		*dest = node->left;
	} else if (!node->right->left) {
		/* 'node's right child doesn't have a left child, so
		 * node->right is 'node's successor, and we can swap it in.
		 */
		BstNode *r = node->right;

		*dest = r;

		/* 'node's left child remains at the same position in the tree,
		 * but we need to attach it to its new parent node.
		 */
		r->left = node->left;
		r->balance = node->balance;

		/* we might have to rebalance the subtree starting at 'r',
		 * so record it. also note that the right subtree got shorter
		 * there.
		 */
		path[i] = r;
		c_top[i++] = 1;
	} else {
		/* find the successor of 'node' (the left-most right child) */
		BstNode *s = node->right->left, *s_parent = node->right;
		BstNode **found;

		found = &path[i];
		c_top[i++] = 1;

		path[i] = s_parent;
		c_top[i++] = -1;

		while (s->left) {
			path[i] = s;
			c_top[i++] = -1;

			s_parent = s;
			s = s->left;
		}

		*found = *dest = s;

		s_parent->left = s->right;
		s->left = node->left;
		s->right = node->right;

		s->balance = node->balance;
	}

	/* remember what to return and free the node itself */
	removed_data = node->data;
	free (node);

	/* now walk the path from the node that we removed back to the root.
	 * at each step we update the current node's balance value and
	 * rebalance the subtree if necessary.
	 */
	while (--i >= 0) {
		node = path[i];

		/* if we went left at this point, the deleted node
		 * was also in the left subtree, so the left subtree
		 * got shorter and the balance is shifting to the right
		 * hand side.
		 */
		if (c_top[i] < 0) {
			if (--node->balance == -1)
				break;

			if (node->balance == -2) {
				BstNode *r = node->right;

				if (r->balance == 1)
					node->right = rotate_rl (r);
				else {
					BstNode *new_root, *old_root = node;

					/* find out where 'node' is attached */
					dest = i ? get_attach_point (node, path[i - 1])
					         : &tree->root;

					*dest = new_root = rotate_rr (node);

					if (r->balance)
						new_root->balance = old_root->balance = 0;
					else {
						new_root->balance = 1;
						old_root->balance = -1;
						break;
					}
				}
			}
		} else {
			if (++node->balance == 1)
				break;

			if (node->balance == 2) {
				BstNode *r = node->left;

				if (r->balance == -1)
					node->left = rotate_lr (r);
				else {
					BstNode *new_root, *old_root = node;

					/* find out where 'node' is attached */
					dest = i ? get_attach_point (node, path[i - 1])
					         : &tree->root;

					*dest = new_root = rotate_ll (node);

					if (r->balance)
						new_root->balance = old_root->balance = 0;
					else {
						new_root->balance = -1;
						old_root->balance = 1;
						break;
					}
				}
			}
		}
	}

	return removed_data;
}
