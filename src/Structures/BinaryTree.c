#include "BinaryTree.h"

static BinaryTreeNode binary_tree_create_node(BinaryTree* tree, void* value) {
	BinaryTreeNode node = malloc(sizeof *node);
	node->value = value;
	node->left = NULL;
	node->right = NULL;
	return node;
}

static void binary_tree_free_node_and_children(BinaryTree* tree, BinaryTreeNode node) {
	if(node == NULL) {
		return;
	}
	binary_tree_free_node_and_children(tree, node->left);
	binary_tree_free_node_and_children(tree, node->right);
	free(node);
}

void binary_tree_init(BinaryTree* tree, int (*compare_function)(const void*, const void*)) {
	tree->root = NULL;
	tree->compare_function = compare_function;
}

void binary_tree_destroy(BinaryTree* tree) {
	binary_tree_free_node_and_children(tree, tree->root);
}

BinaryTreeNode binary_tree_put_in_node(BinaryTree* tree, BinaryTreeNode node, void* value) {
	int compare_result;
	if(node == NULL) {
		compare_result = 0;
	} else {
		compare_result = tree->compare_function(value, node->value);
	}

	if(compare_result == 0) {
		free(node);
		return binary_tree_create_node(tree, value);
	} else if(compare_result < 0) {
		node->left = binary_tree_put_in_node(tree, node->left, value);
	} else {
		node->right = binary_tree_put_in_node(tree, node->right, value);
	}

	return node;
}

void binary_tree_put(BinaryTree* tree, void* value) {
	BinaryTreeNode node = binary_tree_create_node(tree, value);
	if(tree->root == NULL) {
		tree->root = node;
	} else {
		tree->root = binary_tree_put_in_node(tree, tree->root, value);
	}
}

void* binary_tree_get(BinaryTree* tree, const void* value) {
	BinaryTreeNode node = tree->root;
	int compare_result;
	while(node != NULL) {
		compare_result = tree->compare_function(value, node->value);
		if(compare_result == 0) {
			return node->value;
		} else if(compare_result > 0) {
			node = node->right;
		} else { /* compare_result < 0 */
			node = node->left;
		}
	}
	return NULL;
}

void* binary_tree_remove(BinaryTree* tree, void* value) {
	if(tree->root == NULL) {
		return NULL;
	}

	BinaryTreeNode node_parent = NULL;
	BinaryTreeNode node = tree->root;
	int compare_result;

	while(node != NULL && (compare_result = tree->compare_function(value, node->value)) != 0) {
		node_parent = node;
		if(compare_result > 0) {
			node = node->right;
		} else { /* compare_result < 0 */
			node = node->left;
		}
	}

	if(node == NULL) {
		return NULL;
	}

	if(node_parent != NULL) {
		BinaryTreeNode remaining_child_of_removed_node_parent = node_parent->left;
		if(compare_result > 0) {
			node_parent->right = node->right;
		} else {
			node_parent->left = node->right;
		}
		if(remaining_child_of_removed_node_parent != NULL) {
			while(remaining_child_of_removed_node_parent->left != NULL) {
				remaining_child_of_removed_node_parent = remaining_child_of_removed_node_parent->left;
			}
			remaining_child_of_removed_node_parent->left = node->left;
		}
	} else {
		tree->root = NULL;
	}

	void* node_value = node->value;
	free(node);
	return node_value;
}

void* binary_tree_pop_lowest(BinaryTree* tree) {
	if(tree->root == NULL) {
		return NULL;
	}

	BinaryTreeNode lowest_node_parent = NULL;
	BinaryTreeNode lowest_node = tree->root;
	while(lowest_node->left != NULL) {
		lowest_node_parent = lowest_node;
		lowest_node = lowest_node->left;
	}

	void* value = lowest_node->value;
	if(lowest_node_parent != NULL) {
		lowest_node_parent->left = lowest_node->right;
	} else {
		tree->root = lowest_node->right;
	}

	free(lowest_node);
	return value;
}

static int binary_tree_node_size(BinaryTreeNode node) {
	return (node != NULL ?
	        binary_tree_node_size(node->left) + binary_tree_node_size(node->right) + 1 :
	        0);
}

int binary_tree_size(BinaryTree* tree) {
	return binary_tree_node_size(tree->root);
}
