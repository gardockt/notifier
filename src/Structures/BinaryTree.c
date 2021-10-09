#include "BinaryTree.h"

static BinaryTreeNode createNode(BinaryTree* tree, void* value) {
	BinaryTreeNode node = malloc(sizeof *node);
	node->value = value;
	node->left = NULL;
	node->right = NULL;
	return node;
}

static void freeNode(BinaryTree* tree, BinaryTreeNode node) {
	if(node == NULL) {
		return;
	}
	if(tree->valueFreeFunction != NULL) {
		tree->valueFreeFunction(node->value);
	}
	free(node);
}

static void freeNodeAndChildren(BinaryTree* tree, BinaryTreeNode node) {
	if(node == NULL) {
		return;
	}
	freeNodeAndChildren(tree, node->left);
	freeNodeAndChildren(tree, node->right);
	freeNode(tree, node);
}

void binaryTreeInit(BinaryTree* tree, int (*compareFunction)(void*, void*)) {
	tree->root = NULL;
	tree->compareFunction = compareFunction;
	tree->valueFreeFunction = NULL;
}

void binaryTreeDestroy(BinaryTree* tree) {
	freeNodeAndChildren(tree, tree->root);
}

BinaryTreeNode binaryTreePutInNode(BinaryTree* tree, BinaryTreeNode node, void* value) {
	int compareResult;
	if(node == NULL) {
		compareResult = 0;
	} else {
		compareResult = tree->compareFunction(value, node->value);
	}

	if(compareResult == 0) {
		freeNode(tree, node);
		return createNode(tree, value);
	} else if(compareResult < 0) {
		node->left = binaryTreePutInNode(tree, node->left, value);
	} else {
		node->right = binaryTreePutInNode(tree, node->right, value);
	}

	return node;
}

void binaryTreePut(BinaryTree* tree, void* value) {
	BinaryTreeNode node = createNode(tree, value);
	if(tree->root == NULL) {
		tree->root = node;
	} else {
		tree->root = binaryTreePutInNode(tree, tree->root, value);
	}
}

void* binaryTreePopLowest(BinaryTree* tree) {
	if(tree->root == NULL) {
		return NULL;
	}

	BinaryTreeNode lowestNodeParent = NULL;
	BinaryTreeNode lowestNode = tree->root;
	while(lowestNode->left != NULL) {
		lowestNodeParent = lowestNode;
		lowestNode = lowestNode->left;
	}

	void* value = lowestNode->value;
	if(lowestNodeParent != NULL) {
		lowestNodeParent->left = lowestNode->right;
	} else {
		tree->root = lowestNode->right;
	}

	freeNode(tree, lowestNode);
	return value;
}
