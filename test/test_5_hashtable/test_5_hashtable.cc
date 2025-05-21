#include <Arduino.h>
#include <unity.h>

#define I2CIP_DEBUG_SERIAL Serial
#define DEBUG_DELAY() { delayMicroseconds(10); }

#include <hashtable.h>

HashTable<int> hashtable = HashTable<int>();

void test_hashtable_empty(void) {
  int* entry = hashtable["null"];
  TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, entry, "Empty Hashtable: \"null\" -> nullptr");

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(hashtable.toString());
    DEBUG_DELAY();
  #endif
}

void test_hashtable_set(void) {
  HashTableEntry<int>* entry = hashtable.set("test", new int(1));
  TEST_ASSERT_EQUAL_STRING_MESSAGE("test", entry->key, "Hashtable Set: Key match");
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, *entry->value, "Hashtable Set: Value match");

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(hashtable.toString());
    DEBUG_DELAY();
  #endif
}

void test_hashtable_overwrite(void) {
  int* y = new int(2);
  HashTableEntry<int>* entry = hashtable.set("test", y, false);
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, *entry->value, "Hashtable NOT Overwrite: Overwritten");
  entry = hashtable.set("test", y, true);
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, *entry->value, "Hashtable Overwrite: Not overwritten");

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(hashtable.toString());
    DEBUG_DELAY();
  #endif
}

void test_hashtable_get(void) {
  HashTableEntry<int>* entry = hashtable.get("test");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("test", entry->key, "Hashtable Get: Key match");
  int* value = hashtable["test"];
  TEST_ASSERT_FALSE_MESSAGE(value == nullptr, "Hashtable Get");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(entry->value, value, "Hashtable Shortcut: Ptr match");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, *value, "Hashtable Get: Value match");
}

// NOTE: This operation DELETES y
void test_hashtable_remove(void) {
  bool found = hashtable.remove("test");
  TEST_ASSERT_TRUE_MESSAGE(found, "Hashtable Remove");
  int* value = hashtable["test"];
  TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, value, "Hashtable Remove");

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(hashtable.toString());
    DEBUG_DELAY();
  #endif
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_hashtable_empty);

  delay(1000);

  RUN_TEST(test_hashtable_set);

  delay(1000);
  
  RUN_TEST(test_hashtable_overwrite);

  delay(1000);
  
  RUN_TEST(test_hashtable_get);

  delay(1000);
  
  RUN_TEST(test_hashtable_remove);

  delay(1000);
  

  UNITY_END();
}

void loop() {

}