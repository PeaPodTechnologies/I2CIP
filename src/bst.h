#ifndef I2CIP_BST_H_
#define I2CIP_BST_H_

#include <type_traits>

// Basic Binary Search Tree (BST) implementation

template <typename K, typename T> class BSTNode {
  public:
    BSTNode(K key, T value, BSTNode<K,T>* left = nullptr, BSTNode<K,T>* right = nullptr);
    ~BSTNode();

    K key;
    T value;
    BSTNode<K,T>* left;
    BSTNode<K,T>* right;

    String toString(void) const {
      String str;
      if(this->left != nullptr) { 
        str += this->left->toString();
      }
      str += this->key; str += (' ');
      if(this->right != nullptr) {
        str += this->right->toString();
      }
      return str;
    }
};

template <typename K, typename T> class BST {
  static_assert(std::is_unsigned<K>::value, "BST key <typename K> must be an unsigned integer type.");
  public:
    BST();

    ~BST();

    // Pointer to root node - all nodes de/allocated on insert()/remove()
    BSTNode<K,T>* root = nullptr;

    /**
     * Recursively finds a spot to insert a new node.
     * @param key
     * @param value
     * @param root - Pass-by-ref, to be modified on insertion
     * @return Pointer to the inserted node
     */
    static BSTNode<K,T>* insert(K key, T value, BSTNode<K,T>*& root, bool overwrite = true);
    BSTNode<K,T>* insert(K key, T value, bool overwrite = true);

    /**
     * Recursively finds a node by key and removes it.
     * @param key
     * @param root
     */
    static BSTNode<K,T>* remove(K key, BSTNode<K,T>*& root);
    BSTNode<K,T>* remove(K key);

    /**
     * Recursively finds a node by key. Breadth-first.
     * @param key
     * @param root
     */
    static BSTNode<K,T>* find(K key, BSTNode<K,T>* const root);
    BSTNode<K,T>* find(K key) const;

    T* operator[](K key) const;

    /**
     * Recursively finds the least-key node. Depth-first.
     * @param root
     */
    static BSTNode<K,T>* findMin(BSTNode<K,T>* root);
    BSTNode<K,T>* findMin(void);

    /**
     * Recursively finds the greatest-key node. Depth-first.
     * @param root
     */
    static BSTNode<K,T>* findMax(BSTNode<K,T>* root);
    BSTNode<K,T>* findMax(void);

    String toString(void) const {
      return this->root == nullptr ? String("BST Empty") : String("BST [") + this->root->toString() + "]";
    }
};

#include "bst.tpp"

#endif