#include "BinaryTree.h"

static BinaryTreeNode createNode(BinaryTree* tree, void* value) {
	BinaryTreeNode node = malloc(sizeof *node);
	node->value = value;
	node->left = NULL;
	node->right = NULL;
	return node;
}

static void freeNodeAndChildren(BinaryTree* tree, BinaryTreeNode node) {
	if(node == NULL) {
		return;
	}
	freeNodeAndChildren(tree, node->left);
	freeNodeAndChildren(tree, node->right);
	free(node);
}

void binaryTreeInit(BinaryTree* tree, int (*compareFunction)(const void*, const void*)) {
	tree->root = NULL;
	tree->compareFunction = compareFunction;
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
		free(node);
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

void* binaryTreeGet(BinaryTree* tree, void* value) {
	BinaryTreeNode node = tree->root;
	int compareResult;
	while(node != NULL) {
		compareResult = tree->compareFunction(value, node->value);
		if(compareResult == 0) {
			return node->value;
		} else if(compareResult > 0) {
			node = node->right;
		} else { // compareResult < 0
			node = node->left;
		}
	}
	return NULL;
}

void* binaryTreePop(BinaryTree* tree, void* value) {
	if(tree->root == NULL) {
		return NULL;
	}

	BinaryTreeNode nodeParent = NULL;
	BinaryTreeNode node = tree->root;
	int compareResult;

	while(node != NULL && (compareResult = tree->compareFunction(value, node->value)) != 0) {
		nodeParent = node;
		if(compareResult > 0) {
			node = node->right;
		} else { // compareResult < 0
			node = node->left;
		}
	}

	if(node == NULL) {
		return NULL;
	}

	if(nodeParent != NULL) {
		BinaryTreeNode remainingChildOfRemovedNodeParent = nodeParent->left;
		if(compareResult > 0) {
			nodeParent->right = node->right;
		} else {
			nodeParent->left = node->right;
		}
		if(remainingChildOfRemovedNodeParent != NULL) {
			while(remainingChildOfRemovedNodeParent->left != NULL) {
				remainingChildOfRemovedNodeParent = remainingChildOfRemovedNodeParent->left;
			}
			remainingChildOfRemovedNodeParent->left = node->left;
		}
	} else {
		tree->root = NULL;
	}
	return node->value;
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

	free(lowestNode);
	return value;
}

static int binaryTreeNodeSize(BinaryTreeNode node) {
	return (node != NULL ?
	        binaryTreeNodeSize(node->left) + binaryTreeNodeSize(node->right) + 1 :
	        0);
}

int binaryTreeSize(BinaryTree* tree) {
	return binaryTreeNodeSize(tree->root);
}
