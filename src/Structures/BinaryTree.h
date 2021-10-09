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
	int (*compareFunction)(void*, void*);
	void (*valueFreeFunction)(void*);
} BinaryTree;

void binaryTreeInit(BinaryTree* tree, int (*compareFunction)(void*, void*));
void binaryTreeDestroy(BinaryTree* tree);

void binaryTreePut(BinaryTree* tree, void* value);
void* binaryTreePopLowest(BinaryTree* tree);

#endif // ifndef BINARY_TREE_H
