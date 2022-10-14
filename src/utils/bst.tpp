#include <utils/bst.h>

// BST NODE

template <typename K, typename T> BSTNode<K,T>::BSTNode(K key, T value, BSTNode<K,T>* left, BSTNode<K,T>* right) : key(key), value(value), left(left), right(right) { }

template <typename K, typename T> BSTNode<K,T>::~BSTNode() {
  delete(this->left);
  delete(this->right);
}

// BINARY SEARCH TREE

template <typename K, typename T> BST<K,T>::BST() { }

template <typename K, typename T> BST<K,T>::~BST() {
    delete(this->root);
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::insert(K key, T value, BSTNode<K,T>*& root, bool overwrite) {
    // Node empty? Allocate new. Otherwise, insert recursively
    if(root == nullptr) {
        root = new BSTNode<K,T>(key, value);
    } else if(key < root->key) {
        return insert(key, value, root->left);
    } else if(key > root->key) {
        return insert(key, value, root->right);
    } else if(overwrite) {
        // Keys match, overwrite
        root->value = value;
    }
    return root;
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::insert(K key, T value, bool overwrite) {
    return insert(key, value, this->root, overwrite);
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::remove(K key, BSTNode<K,T>*& root) {
    BSTNode<K,T>* match = find(key, root);
    if(match == nullptr) {
        // Match not found
        return nullptr;
    } else {
        // Match found, remove root properly
        if(root->left && root->right) {
            // Both branches exist, replace root with LEAST key on RIGHT branch
            BSTNode<K,T>* rightLeast = findMin(root->right);
            
            // Detach branches
            BSTNode<K,T>* newroot = new BSTNode<K,T>(rightLeast->key, rightLeast->value, remove(rightLeast->key, root->right), root->left);

            // Detach and free old root
            root->right = root->left = nullptr;
            delete(root);
            root = newroot;
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

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::remove(K key) {
    return remove(key, this->root);
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::find(K key, BSTNode<K,T>* root) {
    if(root != nullptr) {
        if(key < root->key) {
            return find(key, root->left);
        } else if(key > root->key) {
            return find(key, root->right);
        }
    }
    return root;
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::find(K key) {
    return find(key, this->root);
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::operator[](K key) {
    return find(key, this->root);
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::findMin(BSTNode<K,T>* root) {
    // If this is a leaf node, or there is no left node, return
    if(root == nullptr || root->left == nullptr) {
        return root;
    } else {
        return findMin(root->left);
    }
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::findMin(void) {
    return findMin(this->root);
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::findMax(BSTNode<K,T>* root) {
    // If this is a leaf node, or there is no right node, return
    if(root == nullptr || root->right == nullptr) {
        return root;
    } else {
        return findMin(root->right);
    }
}

template <typename K, typename T> BSTNode<K,T>* BST<K,T>::findMax(void) {
    return findMax(this->root);
}