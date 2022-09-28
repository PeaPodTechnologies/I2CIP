#ifndef BST_H_
#define BST_H_

template <typename U> class BSTNode {
  public:
    BSTNode(const int& key, const U& value);
    ~BSTNode();

    const int key;            // IMMUTABLE (copied) key - passed in
    U value;                  // Value
    BSTNode* left = nullptr;  // Pointer to left (lesser) node
    BSTNode* right = nullptr; // Pointer to right (greater) node
};

template <typename T> class BST {
  private:
    // Pointer to root node - all nodes de/allocated on insert()/remove()
    BSTNode<T>* root = nullptr;
  public:
    BST();

    ~BST();

    /**
     * Recursively finds a spot to insert a new node.
     * @param key
     * @param value
     * @param root
     * @return Pointer to the inserted node
     */
    BSTNode<T>* insert(const int& key, const T& value, bool overwrite = true, BSTNode<T>* root = this->root);

    /**
     * Recursively finds a node by key and removes it.
     * @param key
     * @param root
     */
    BSTNode<T>* remove(const int& key, BSTNode<T>* root = this->root);

    /**
     * Recursively finds a node by key. Breadth-first.
     * @param key
     * @param root
     */
    BSTNode<T>* find(const int& key, BSTNode<T>* root = this->root);

    T* operator[](const int& key);

    /**
     * Recursively finds the least-key node. Depth-first.
     * @param root
     */
    BSTNode<T>* findMin(BSTNode<T>* root = this->root);

    /**
     * Recursively finds the greatest-key node. Depth-first.
     * @param root
     */
    BSTNode<T>* findMax(BSTNode<T>* root = this->root);

};

#endif