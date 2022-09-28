#include <utils/bst.h>

// BST NODE

template <typename U> BSTNode<U>::BSTNode(const int& key, const U& value) : key(key), value(value) { }

template <typename U> BSTNode<U>::~BSTNode() {
  delete(this->left);
  delete(this->right);
}

// BINARY SEARCH TREE

template <typename T> BST<T>::~BST() {
    delete(this->root);
}

template <typename T> BSTNode<T>* BST<T>::insert(const int& key, const T& value, bool overwrite, BSTNode<T>* root) {
    // Node empty? Allocate new. Otherwise, insert recursively
    if(root == nullptr) {
        root = new BSTNode<T>(key, value);
    } else if(key < root->key) {
        root->left = insert(key, value, root->left);
    } else if(key > root->key) {
        root->right = insert(key, value, root->right);
    } else if(overwrite) {
        // Keys match, overwrite
        root->value = value;
    }
    return root;
}

template <typename T> BSTNode<T>* BST<T>::remove(const int& key, BSTNode<T>* root) {
    BSTNode<T>* match = find(key, root);
    if(match == nullptr) {
        // Match not found
        return nullptr;
    } else {
        // Match found, remove root properly
        if(root->left && root->right) {
            // Both branches exist, replace root with LEAST key on RIGHT branch
            BSTNode<T>* rightLeast = findMin(root->right);
            root->key = rightLeast->key;
            root->value = rightLeast->value;

            // Remove old node
            root->right = remove(rightLeast->key, root->right);
        } else {
            // Branch(es) missing, replace with a branch node
            if(root->left == nullptr) {
                root = root->right;
            } else if(root->right == nullptr) {
                root = root->left;
            } else {
                // No branches on this match; delete
                delete(root);
            }
        }
    }
    return root;
}

template <typename T> BSTNode<T>* BST<T>::find(const int& key, BSTNode<T>* root) {
    if(root == nullptr || root->key == key) {
        return root;
    } else if(key < root->key) {
        return find(key, root->left);
    } else if(key > root->key) {
        return find(key, root->right);
    }
}

template <typename T> T* BST<T>::operator[](const int& key) {
    return &(find(key)->value);
}

template <typename T> BSTNode<T>* BST<T>::findMin(BSTNode<T>* root) {
    // If this is a leaf node, or there is no left node, return
    if(root == nullptr || root->left == nullptr) {
        return root;
    } else {
        return findMin(root->left);
    }
}

template <typename T> BSTNode<T>* BST<T>::findMax(BSTNode<T>* root) {
    // If this is a leaf node, or there is no right node, return
    if(root == nullptr || root->right == nullptr) {
        return root;
    } else {
        return findMin(root->right);
    }
}