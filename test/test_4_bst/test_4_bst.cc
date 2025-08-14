#include <Arduino.h>
#include <unity.h>

#include <bst.h>

BST<int, const char*> bst = BST<int, const char*>();

void test_bst_empty(void) {
  BSTNode<int, const char*>* node = bst.findMin();
  TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, node, "Empty BST: Find Min -> nullptr");
  node = bst.findMax();
  TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, node, "Empty BST: Find Max -> nullptr");
}

void test_bst_insert(void) {
  BSTNode<int, const char*>* node = bst.insert(1, "a");
  TEST_ASSERT_EQUAL_UINT_MESSAGE(1, node->key, "BST Insert: Key match");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("a", node->value, "BST Insert: Value match");
}

void test_bst_overwrite(void) {
  BSTNode<int, const char*>* node = bst.insert(1, "b", false);
  TEST_ASSERT_EQUAL_STRING_MESSAGE("a", node->value, "BST NOT Overwrite: Overwritten");
  node = bst.insert(1, "b");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("b", node->value, "BST Overwrite: Not overwritten");
}

void test_bst_find(void) {
  BSTNode<int, const char*>* node = bst.find(1);
  TEST_ASSERT_EQUAL_UINT_MESSAGE(1, node->key, "BST Find: Key match");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("b", node->value, "BST Find: Value match");
}

void test_bst_remove(void) {
  bst.remove(1);
  BSTNode<int, const char*>* node = bst.find(1);
  TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, node, "BST Remove: Find -> nullptr");
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_bst_empty);

  delay(1000);

  RUN_TEST(test_bst_insert);

  delay(1000);

  RUN_TEST(test_bst_overwrite);

  delay(1000);

  RUN_TEST(test_bst_find);

  delay(1000);

  RUN_TEST(test_bst_remove);

  delay(1000);

  UNITY_END();
}

void loop() {

}