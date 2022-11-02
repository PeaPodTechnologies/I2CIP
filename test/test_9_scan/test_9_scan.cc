#include <Arduino.h>
#include <unity.h>

#include <utils/bst.h>
#include <utils/hashtable.h>
#include <i2cip/routingtable.h>
#include <I2CIP.h>
#include "../config.h"

RoutingTable routingtable = RoutingTable();

bool helper_bst_in_ht(RoutingTable& rt, i2cip_device_t* root) {
  if(root->left == nullptr && root->right == nullptr) {
    bool t = rt[root->value]->contains(root);
    char msg[25];
    sprintf(msg, "BST entry %01X.%01X.%01X.%02X in HT", I2CIP_FQA_SEG_I2CBUS(root->key), I2CIP_FQA_SEG_MUXNUM(root->key), I2CIP_FQA_SEG_MUXBUS(root->key), I2CIP_FQA_SEG_DEVADR(root->key));
    TEST_ASSERT_TRUE_MESSAGE(t, msg);
    return t;
  }

  bool left = false, right = false;
  if(root->left != nullptr) {
    left = helper_bst_in_ht(rt, root->left);
  }
  if(root->right != nullptr) {
    right = helper_bst_in_ht(rt, root->right);
  }
  return left && right;
}

bool helper_ht_in_bst(RoutingTable& rt, HashTableEntry<DeviceGroup*>* entry) {
  if(entry == nullptr || entry->value == nullptr || entry->value->devices == nullptr || entry->value->numdevices == 0) {
    return true;
  }
  bool result = true;
  for(int i = 0; i < entry->value->numdevices; i++) {
    i2cip_fqa_t fqa = entry->value->devices[i]->key;
    bool t = (strcmp(rt[fqa], entry->key) == 0);
    char msg[25];
    sprintf(msg, "HT entry %01X.%01X.%01X.%02X in BST", I2CIP_FQA_SEG_I2CBUS(fqa), I2CIP_FQA_SEG_MUXNUM(fqa), I2CIP_FQA_SEG_MUXBUS(fqa), I2CIP_FQA_SEG_DEVADR(fqa));
    TEST_ASSERT_TRUE_MESSAGE(t, msg);
    result &= t;
  }
  return helper_ht_in_bst(rt, entry->next) && result;
}

void test_routingtable_scan(void) {
  I2CIP::i2cip_errorlevel_t errlev = I2CIP::scanModule(routingtable, MUXNUM, WIRENUM);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, errlev, "Scan");

  TEST_ASSERT_FALSE_MESSAGE(routingtable.getDevices().root == nullptr, "Scan: Device(s) added to BST");

  bool anyslot = false;
  for(int i = 0; i < HASHTABLE_SLOTS; i++) {
    HashTableEntry<DeviceGroup*>* entry = routingtable.getDeviceGroups().hashtable[i];
    anyslot |= (entry != nullptr && entry->value != nullptr && entry->value->numdevices != 0 && entry->value->devices != nullptr);
  }
  TEST_ASSERT_TRUE_MESSAGE(anyslot, "Scan: Device(s) added to HT");
}

void test_routingtable_bst(void) {
  bool bst = helper_bst_in_ht(routingtable, routingtable.getDevices().root);
  TEST_ASSERT_TRUE_MESSAGE(bst, "All BST in HT");
}

void test_routingtable_ht(void) {
  bool ht = true;
  for(int i = 0; i < HASHTABLE_SLOTS; i++) {
    ht &= helper_ht_in_bst(routingtable, routingtable.getDeviceGroups().hashtable[i]);
  }
  TEST_ASSERT_TRUE_MESSAGE(ht, "All HT in BST");
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_routingtable_scan);
  RUN_TEST(test_routingtable_bst);
  RUN_TEST(test_routingtable_ht);

  UNITY_END();
}

void loop() {

}