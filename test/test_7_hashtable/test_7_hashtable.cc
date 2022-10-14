#include <Arduino.h>
#include <unity.h>

#include <utils/hashtable.h>

HashTable<int> hashtable = HashTable<int>();

void test_hashtable_empty(void) {
  HashTableEntry<int>* entry = hashtable["null"];
  TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, entry, "Empty Hashtable: \"null\" -> nullptr");
}

void test_hashtable_set(void) {
  HashTableEntry<int>* entry = hashtable.set("test", 1);
  TEST_ASSERT_EQUAL_STRING_MESSAGE("test", entry->key, "Hashtable Set: Key match");
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, entry->value, "Hashtable Set: Value match");
}

void test_hashtable_overwrite(void) {
  HashTableEntry<int>* entry = hashtable.set("test", 2, false);
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, entry->value, "Hashtable NOT Overwrite: Overwritten");
  entry = hashtable.set("test", 2);
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, entry->value, "Hashtable Overwrite: Not overwritten");
}

void test_hashtable_get(void) {
  HashTableEntry<int>* entry = hashtable["test"];
  TEST_ASSERT_FALSE_MESSAGE(entry == nullptr, "Hashtable Get");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("test", entry->key, "Hashtable Get: Key match");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, entry->value, "Hashtable Get: Value match");
}

void test_hashtable_remove(void) {
  bool found = hashtable.remove("test");
  TEST_ASSERT_TRUE_MESSAGE(found, "Hashtable Remove");
  HashTableEntry<int>* entry = hashtable["test"];
  TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, entry, "Hashtable Remove");
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_hashtable_empty);
  RUN_TEST(test_hashtable_set);
  RUN_TEST(test_hashtable_overwrite);
  RUN_TEST(test_hashtable_get);
  RUN_TEST(test_hashtable_remove);

  UNITY_END();
}

void loop() {

}