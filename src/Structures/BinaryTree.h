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
	int (*compareFunction)(const void*, const void*);
} BinaryTree;

void binaryTreeInit(BinaryTree* tree, int (*compareFunction)(const void*, const void*));
void binaryTreeDestroy(BinaryTree* tree);

void binaryTreePut(BinaryTree* tree, void* value);
void* binaryTreeGet(BinaryTree* tree, void* value);
void* binaryTreeRemove(BinaryTree* tree, void* value);
void* binaryTreePopLowest(BinaryTree* tree);
int binaryTreeSize(BinaryTree* tree);

#endif // ifndef BINARY_TREE_H
