#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct BTNode {
	void* value;
	struct BTNode* left;
	struct BTNode* right;
} *BinaryTreeNode;

typedef struct {
	BinaryTreeNode root;
	int (*compare_function)(const void*, const void*);
} BinaryTree;

void binary_tree_init(BinaryTree* tree, int (*compare_function)(const void*, const void*));
void binary_tree_destroy(BinaryTree* tree);

void binary_tree_put(BinaryTree* tree, void* value);
void* binary_tree_get(BinaryTree* tree, const void* value);
void* binary_tree_remove(BinaryTree* tree, void* value);
void* binary_tree_pop_lowest(BinaryTree* tree);
int binary_tree_size(BinaryTree* tree);

#endif /* ifndef BINARY_TREE_H */
