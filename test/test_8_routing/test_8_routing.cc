#include <Arduino.h>
#include <unity.h>

#include <utils/bst.h>
#include <utils/hashtable.h>
#include <i2cip/routingtable.h>
#include <I2CIP.h>
#include "../config.h"

// RoutingTable routingtable = RoutingTable();
RoutingTable* rt = nullptr;

// void test_routingtable_empty(void) {
//   i2cip_fqa_t fqa = 0;
//   DeviceGroup* group = routingtable["null"];
//   const char* id = routingtable[fqa];``
//   TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, group, "Hashtable[\"null\"] -> nullptr");
//   TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, id, "BST[0] -> nullptr");
// }

// void test_routingtable_add(void) {
//   const i2cip_device_t& device = routingtable.add("test", 1);
//   TEST_ASSERT_EQUAL_UINT_MESSAGE(1, device.key, "New Entry: FQA match");
//   TEST_ASSERT_EQUAL_STRING_MESSAGE("test", device.value, "New Entry: ID match");
//   const char* key = routingtable[1];
//   TEST_ASSERT_EQUAL_STRING_MESSAGE(device.value, key, "BST: FQA lookup");
//   DeviceGroup* group = routingtable["test"];
//   TEST_ASSERT_FALSE_MESSAGE(group == nullptr, "HT: Device Group lookup by ID");
//   TEST_ASSERT_EQUAL_UINT_MESSAGE(1, group->numdevices, "Device Group: Device Count");
//   TEST_ASSERT_EQUAL_STRING_MESSAGE("test", group->key, "Device Group: ID match");
//   TEST_ASSERT_EQUAL_UINT_MESSAGE(1, group->devices[0]->key, "Device Group: devices[0] FQA match");
// }

// void test_routingtable_overwrite(void) {
//   const i2cip_device_t& device = routingtable.add("test1", 1, false);
//   TEST_ASSERT_EQUAL_STRING_MESSAGE("test", device.value, "NO Overwrite: Overwritten");
//   routingtable.add("test1", 1);
//   TEST_ASSERT_EQUAL_STRING_MESSAGE("test1", device.value, "Overwrite: Not overwritten");
// }

// void test_routingtable_addgroup(void) {
//   i2cip_fqa_t fqas[3] = {1, 2, 3};
//   // Add without overwriting {1:"test1"} from the previous test
//   const DeviceGroup& devicegroup = routingtable.addGroup("test2", fqas, 3, false);
//   TEST_ASSERT_EQUAL_UINT_MESSAGE(2, devicegroup.numdevices, "Add Group: Device count match");
//   for (int i = 0; i < devicegroup.numdevices; i++) {
//     if(devicegroup.devices[i]->key == 1) {
//       TEST_ASSERT_EQUAL_STRING_MESSAGE("test1", devicegroup.devices[i]->value, "Add Group: Overwrote");
//     } else {
//       TEST_ASSERT_EQUAL_STRING_MESSAGE("test2", devicegroup.devices[i]->value, "Added entry ID match");
//     }
//   }
// }

// void test_routingtable_remove(void) {
//   TEST_ASSERT_FALSE_MESSAGE(routingtable.remove(0), "Remove: Nonexistent entry");
//   TEST_ASSERT_TRUE_MESSAGE(routingtable.remove(1), "Found entry and attempted to remove!");
//   TEST_ASSERT_EQUAL_PTR_MESSAGE(nullptr, routingtable[1], "Entry removed successfully!");
//   uint8_t n = routingtable["test1"]->numdevices;
//   TEST_ASSERT_EQUAL_UINT_MESSAGE(0, n, "Entry removed successfully!");
//   TEST_ASSERT_TRUE_MESSAGE(routingtable.remove(2), "Removed remaining group entries! (1/2)");
//   TEST_ASSERT_TRUE_MESSAGE(routingtable.remove(3), "Removed remaining group entries! (2/2)");
//   n = routingtable["test2"]->numdevices;
//   TEST_ASSERT_EQUAL_UINT_MESSAGE(0, n, "Device group removed successfully!");
// }

bool helper_bst_in_ht(RoutingTable& rt, i2cip_device_t* root) {
  if(root->left == nullptr && root->right == nullptr) {
    bool t = rt[root->value]->contains(root);
    TEST_ASSERT_TRUE(t);
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
    bool t = (strcmp(rt[entry->value->devices[i]->key], entry->key) == 0);
    TEST_ASSERT_TRUE(t);
    result &= t;
  }
  return helper_ht_in_bst(rt, entry->next) && result;
}

void test_routingtable_scan(void) {
  // delete(&routingtable);
  rt = I2CIP::Routing::createRoutingTable();

  TEST_ASSERT_FALSE_MESSAGE(rt->getDevices().root == nullptr, "Scan: Device(s) added to BST");

  bool anyslot = false;
  for(int i = 0; i < HASHTABLE_SLOTS; i++) {
    HashTableEntry<DeviceGroup*>* entry = rt->getDeviceGroups().hashtable[i];
    anyslot |= (entry != nullptr && entry->value != nullptr && entry->value->numdevices != 0 && entry->value->devices != nullptr);
  }
  TEST_ASSERT_TRUE_MESSAGE(anyslot, "Scan: Device(s) added to HT");
}

void test_routingtable_bst(void) {
  bool bst = helper_bst_in_ht(*rt, rt->getDevices().root);
  TEST_ASSERT_TRUE_MESSAGE(bst, "All BST in HT");
}

void test_routingtable_ht(void) {
  bool ht = true;
  for(int i = 0; i < HASHTABLE_SLOTS; i++) {
    ht &= helper_ht_in_bst(*rt, rt->getDeviceGroups().hashtable[i]);
  }
  TEST_ASSERT_TRUE_MESSAGE(ht, "All HT in BST");
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  // routingtable.add("eeprom", eeprom_fqa);

  // RUN_TEST(test_routingtable_empty);
  // RUN_TEST(test_routingtable_add);
  // RUN_TEST(test_routingtable_overwrite);
  // RUN_TEST(test_routingtable_addgroup);
  // RUN_TEST(test_routingtable_remove);
  RUN_TEST(test_routingtable_scan);
  RUN_TEST(test_routingtable_bst);
  RUN_TEST(test_routingtable_ht);

  UNITY_END();
}

void loop() {

}