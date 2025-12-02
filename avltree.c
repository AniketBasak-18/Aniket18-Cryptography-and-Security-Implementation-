#include <stdio.h>
#include <stdlib.h>

// AVL Tree Node Structure
typedef struct AVLNode {
    int data;
    struct AVLNode *left;
    struct AVLNode *right;
    int height;
} AVLNode;

// Get height of a node
int get_height(AVLNode* node) {
    if (node == NULL)
        return 0;
    return node->height;
}

// Get balance factor of a node
int get_balance(AVLNode* node) {
    if (node == NULL)
        return 0;
    return get_height(node->left) - get_height(node->right);
}

// Update height of a node
void update_height(AVLNode* node) {
    if (node != NULL) {
        int left_height = get_height(node->left);
        int right_height = get_height(node->right);
        node->height = 1 + (left_height > right_height ? left_height : right_height);
    }
}

// Print tree in-order traversal
void print_inorder(AVLNode* root) {
    if (root == NULL) {
        return;
    }
    
    print_inorder(root->left);
    printf("%d ", root->data);
    print_inorder(root->right);
}

// Find the node with minimum value in a subtree
AVLNode* _find_min(AVLNode* node) {
    AVLNode* current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

// Right rotation
AVLNode* right_rotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update heights
    update_height(y);
    update_height(x);

    return x;
}

// Left rotation
AVLNode* left_rotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    // Perform rotation
    y->left = x;
    x->right = T2;

    // Update heights
    update_height(x);
    update_height(y);

    return y;
}

// Find a node with key k
AVLNode* _find(AVLNode* root, int k) {
    // Base cases
    if (root == NULL || root->data == k)
        return root;

    // If key is smaller, look in left subtree
    if (k < root->data)
        return _find(root->left, k);

    // If key is larger, look in right subtree
    return _find(root->right, k);
}

// Create a new AVL tree node
AVLNode* create_node(int data) {
    AVLNode* node = (AVLNode*)malloc(sizeof(AVLNode));
    node->data = data;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    return node;
}

// Insert a new node
AVLNode* _insert(AVLNode* root, int k) {
    // 1. Perform standard BST insertion
    if (root == NULL)
        return create_node(k);

    if (k < root->data)
        root->left = _insert(root->left, k);
    else if (k > root->data)
        root->right = _insert(root->right, k);
    else
        // Duplicate keys are not allowed
        return root;

    // 2. Update height of current node
    update_height(root);

    // 3. Get the balance factor
    int balance = get_balance(root);

    // 4. Balance the tree if needed (4 cases)
    // Left Left Case
    if (balance > 1 && k < root->left->data)
        return right_rotate(root);

    // Right Right Case
    if (balance < -1 && k > root->right->data)
        return left_rotate(root);

    // Left Right Case
    if (balance > 1 && k > root->left->data) {
        root->left = left_rotate(root->left);
        return right_rotate(root);
    }

    // Right Left Case
    if (balance < -1 && k < root->right->data) {
        root->right = right_rotate(root->right);
        return left_rotate(root);
    }

    return root;
}

// Delete a node
AVLNode* _delete(AVLNode* root, int k) {
    // 1. Perform standard BST delete
    if (root == NULL)
        return root;

    // Find the node to be deleted
    if (k < root->data)
        root->left = _delete(root->left, k);
    else if (k > root->data)
        root->right = _delete(root->right, k);
    else {
        // Node with only one child or no child
        if (root->left == NULL) {
            AVLNode* temp = root->right;
            free(root);
            return temp;
        }
        else if (root->right == NULL) {
            AVLNode* temp = root->left;
            free(root);
            return temp;
        }

        // Node with two children: Get the inorder successor
        AVLNode* temp = _find_min(root->right);
        root->data = temp->data;
        root->right = _delete(root->right, temp->data);
    }

    // If the tree had only one node, return
    if (root == NULL)
        return root;

    // 2. Update height
    update_height(root);

    // 3. Get the balance factor
    int balance = get_balance(root);

    // 4. Balance the tree (4 cases)
    // Left Left Case
    if (balance > 1 && get_balance(root->left) >= 0)
        return right_rotate(root);

    // Left Right Case
    if (balance > 1 && get_balance(root->left) < 0) {
        root->left = left_rotate(root->left);
        return right_rotate(root);
    }

    // Right Right Case
    if (balance < -1 && get_balance(root->right) <= 0)
        return left_rotate(root);

    // Right Left Case
    if (balance < -1 && get_balance(root->right) > 0) {
        root->right = right_rotate(root->right);
        return left_rotate(root);
    }

    return root;
}

// Public find method
int find(AVLNode* r, int k) {
    AVLNode* result = _find(r, k);
    return result == NULL ? 1 : 0;
}

// Public insert method
int insert(AVLNode** r, int k) {
    // First check if key already exists
    if (_find(*r, k) != NULL)
        return 0;
    
    // Perform insertion
    *r = _insert(*r, k);
    return 1;
}

// Public delete method
int delete(AVLNode** r, int k) {
    // First check if key exists
    if (_find(*r, k) == NULL)
        return 0;
    
    // Perform deletion
    *r = _delete(*r, k);
    return 1;
}

// Free entire tree to prevent memory leaks
void free_tree(AVLNode* root) {
    if (root == NULL)
        return;
    
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

int main() {
    AVLNode* root = NULL;
    int choice, value;

    while (1) {
        // Print menu
        printf("\n--- AVL Tree Operations ---\n");
        printf("1. Insert\n");
        printf("2. Delete\n");
        printf("3. Find\n");
        printf("4. Print In-order Traversal\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        
        // Get user choice
        scanf("%d", &choice);

        switch(choice) {
            case 1:
                printf("Enter value to insert: ");
                scanf("%d", &value);
                if (insert(&root, value)) {
                    printf("Value %d inserted successfully\n", value);
                } else {
                    printf("Value %d already exists\n", value);
                }
                break;

            case 2:
                printf("Enter value to delete: ");
                scanf("%d", &value);
                if (delete(&root, value)) {
                    printf("Value %d deleted successfully\n", value);
                } else {
                    printf("Value %d not found\n", value);
                }
                break;

            case 3:
                printf("Enter value to find: ");
                scanf("%d", &value);
                if (find(root, value) == 0) {
                    printf("Value %d found in the tree\n", value);
                } else {
                    printf("Value %d not found in the tree\n", value);
                }
                break;

            case 4:
                printf("Current In-order Traversal: ");
                print_inorder(root);
                break;

            case 5:
                // Free memory before exiting
                free_tree(root);
                printf("Exiting...\n");
                return 0;

            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}