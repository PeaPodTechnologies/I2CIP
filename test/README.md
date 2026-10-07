# I2CIP Unity Test Suite

<!-- Written by GPT-6 Astra, edited by JLefebvre55. Prompt: Write to test/README.md. Analyze and summarize each unit test's point, with expected values and descriptions of how the actual values are being retrieved. Be verbose.-->

This directory contains eight Arduino Unity test sketches. Each `test_N_...`
directory is a separate firmware test program; the tests are not a single
host-side test suite. They are intended to be built and run on an Arduino-
compatible board using PlatformIO and, for the I2C tests, the matching
module hardware attached (MUX + EEPROM).

The project configuration defines the `nano` and `feather` PlatformIO
environments. The test sketches include Arduino
headers and call `Wire`, `Serial`, `millis()`, GPIO, and I2CIP device APIs
directly. Sequential I2C transactions determine the actual behavior and state
of the connected I2C devices, and error levels are derived from acknowledgement
signals and operation results.

This README depends upon knowledge of I2CIP module concepts, including the
structure of Fully Qualified Addresses (FQAs), the role of the I2C multiplexer,
SPRT JSON format, and the organization of device groups within a module.

## Shared test configuration

[`config.h`](./config.h) supplies settings and the `TestModule` used by several
sketches:

- `WIRENUM` is `0` and `MODULE` is `0`. All device tests address I2C bus 0,
  module 0, with the default SPRT EEPROM at address `0x50` (decimal 80).
- The test EEPROM bytes are `'['` (module array start) and `'{'` (bus object
  start) at EEPROM registers 0 and 1. They are used to verify byte and word
  writes.
- `I2CIP_TEST_EEPROM_OVERWRITE` is enabled and
  `EEPROM_JSON_CONTENTS_TEST` supplies the string
  `[{"24LC32":[80],"SHT45":[68]},{"SHT45":[68]}]`.
- `TestModule` can make device groups for a subset of device libraries: 24LC32 EEPROM, SHT45 and K30 environmental sensors, HT16K33 7-segment display, PCA9685 PWM driver,
  MCP23008 for LCD backpack, Adafruit SEESAW rotary encoder with button, MCP23017 GPIO expander, and Wii Nunchuck devices. Discovery tests
  still require the connected module's EEPROM contents and hardware to be
  compatible with the requested operation.

## `test_0_blink` — built-in LED and GPIO readback

This sketch checks the configured built-in LED pin and then exercises its
output state three times. `setup()` begins Unity, runs the pin-number test,
and configures the pin as an output; `loop()` runs the high and low tests
with a 500 ms pause after each state change.

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_led_builtin_pin_number` | `LED_BUILTIN` must equal integer pin number `13`. | Reads the board/core's `LED_BUILTIN` constant at compile/runtime; no GPIO read is involved. |
| `test_led_state_high` | After writing `HIGH`, the pin must read back `HIGH`. | Calls `digitalWrite(LED_BUILTIN, HIGH)`, then obtains the actual level from `digitalRead(LED_BUILTIN)`. |
| `test_led_state_low` | After writing `LOW`, the pin must read back `LOW`. | Calls `digitalWrite(LED_BUILTIN, LOW)`, then obtains the actual level from `digitalRead(LED_BUILTIN)`. |

The two state tests are executed three times each by the loop counter. This
checks the Arduino core's digital GPIO API/readback behavior, as well as proving
that the controller is reachable via UART.

## `test_1_fqa` — FQA packing, extraction, and wire mapping

These tests are software checks and do not need a responding I2C device.

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_fqa_create` | `I2CIP_FQA_CREATE(5, 3, 4, 65)` must equal the bitwise-OR of the expected fields: wire 5 (`0xA000`), MUX 3 (`0x0C00`), bus 4 (`0x0200`), and device address 65 (`0x0041`). The packed expected FQA is `0xAE41` (decimal 44,609). | Calls `I2CIP_FQA_CREATE` with those four inputs. Unity compares the returned 16-bit integer with the independently OR'ed constants defined at the top of the sketch. |
| `test_fqa_segments` | For FQA created from `(wire=0, mux=0, bus=I2CIP_MUX_BUS_DEFAULT, address=0x50)`, segment extraction must return `0`, `0`, the default MUX bus (`0`), and `0x50`, respectively. | `I2CIP::createFQA` first packs and validates the arguments. `I2CIP_FQA_SEG_I2CBUS`, `...MODULE`, `...MUXBUS`, and `...DEVADR` then independently shift and mask that value. |
| `test_fqa_to_wire` | `wires[0]` must be the same pointer as `&Wire`. | Reads the configured global wire-pointer array (defined by the I2C wire configuration) and compares its first element with the address of Arduino's `Wire` object. No transmission takes place. |

For the FQA tests, an FQA (Fully Qualified Address) is a 16-bit value with
four fields: 3 bits for the I2C wire, 3 for the MUX/module number, 3 for the
MUX bus, and 7 for the device address. The low-level creation macro packs
these fields; the `createFQA` function also rejects values outside
the supported ranges. Segment helpers shift and mask the packed value to
recover the individual fields.

## `test_2_mux` — multiplexer helpers and I2C transactions

The hardware tests use `WIRENUM` 0 and `MODULE` 0 (MUX address 0x70) from `config.h`. The standalone
address-mapping assertion uses module number 2 and therefore expects `0x72`.
Successful communication tests require the MUX to be connected to the selected
I2C wire.

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_mux_bus_to_instr` | MUX bus 4 maps to instruction `0b00010000` (`0x10`, decimal 16). | Evaluates `I2CIP_MUX_BUS_TO_INSTR(4)`, which shifts bit 1 left by four places. This is a software-only conversion. |
| `test_mux_num_to_addr` | MUX number 2 maps to I2C address `0x72`. | Evaluates `I2CIP_MODULE_TO_MUXADDR(2)`, adding module number 2 to the base address `0x70`. |
| `test_mux_ping` | `I2CIP::MUX::pingMUX(0, 0)` must return `true` (an ACK was received). | Begins the selected `Wire`, sends an I2C transmission to `I2CIP_MODULE_TO_MUXADDR(0)` (`0x70`), and returns whether `endTransmission(true)` returned zero. It also measures elapsed milliseconds for a pass message; elapsed time is informational, not compared to a limit. |
| `test_mux_bus_set` | Setting the default MUX bus (bus 0) must return `I2CIP_ERR_NONE` (`0`). | Sends the bus-selection instruction `1 << 0` (`0x01`) to MUX address `0x70`. The result is `I2CIP_ERR_HARD` if the transmission is not acknowledged, `I2CIP_ERR_SOFT` if the instruction write fails, or `I2CIP_ERR_NONE` on success. The elapsed time is printed only. |
| `test_mux_bus_reset` | Resetting the MUX must return `I2CIP_ERR_NONE` (`0`). | Sends the reset instruction `0x00` to MUX address `0x70`; the returned status reflects whether the byte was written and the I2C transmission was acknowledged. Elapsed time is printed only. |

## `test_3_eeprom` — EEPROM device, register I/O, and string interface

The sketch creates an EEPROM at FQA 0:0:0:0x50, the default SPRT FQA for Module 0. Its byte, word, and I/O
tests write to the physical EEPROM before reading back from the same
registers. Run this test only with a compatible EEPROM on the selected bus;
the sketch overwrites its contents during the I/O test.

`test_device_io` 

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_device_oop` | EEPROM factory creation must return a non-null object. Its input getter and output setter must each point back to the EEPROM's own interface subobject. | Calls `EEPROM::factory(eeprom_fqa)`. `getInput()` and `getOutput()` expose the interfaces registered by the EEPROM's `IOInterface` base; pointer comparisons check the expected self-links after interface-to-base casts. |
| `test_eeprom_ping` | `eeprom->ping()` must return `I2CIP_ERR_NONE` (`0`). | The error level comes from the device ping against the EEPROM FQA. The diagnostic string is constructed from the FQA's extracted wire, module, MUX-bus, and device-address fields (displayed in hexadecimal); the test expects a reachable device at the configured address. |
| `test_eeprom_write_byte` | Writing `'['` (`0x5B`) to register 0 must return `I2CIP_ERR_NONE`. | Calls the EEPROM object's 16-bit-register `writeRegister(0, value)` API. The device code sends the register address and value over I2C through the EEPROM's FQA-selected wire/MUX path. This assertion checks the transaction status, not the stored byte's contents. |
| `test_eeprom_read_byte` | Reading register 0 must return `I2CIP_ERR_NONE`, and the byte read must equal `'['` (ASCII `0x5B`). | Calls `readRegisterByte(0, c)`. The API selects the device/MUX path, requests one byte from register 0, and writes the received byte into `c` by reference. Unity checks both the status and the resulting value. |
| `test_eeprom_write_word` | Writing two bytes at register 0 must return `I2CIP_ERR_NONE`. The payload bytes are `'['` (ASCII `0x5B`) followed by `'{'` (ASCII `0x7B`). | Calls the 16-bit-register buffer overload `writeRegister(0, buff, 2)`. It sends both payload bytes after the register address. This test checks only the write status; the next test checks the data. |
| `test_eeprom_read_word` | Reading register 0 as a word must return `I2CIP_ERR_NONE`; the low byte must be `'['` (ASCII `0x5B`) and the high byte `'{'` (ASCII `0x7B`), so the assembled value is `0x7B5B`. | `readRegisterWord(0, c)` receives two bytes into a temporary buffer and assembles the word as `(second_byte << 8) | first_byte`. The assertions extract `c & 0xFF` and `c >> 8` separately. This demonstrates 16-bit register read/write consistency. |
| `test_device_io` | The configured JSON string must be accepted by the setter, retrieved successfully by the getter, and match the EEPROM cache byte-for-byte. | Because overwrite is enabled and `EEPROM_JSON_CONTENTS_TEST` is defined, the test calls `eeprom->getOutput()->set(&msg, &len)` with `[{"24LC32":[80],"SHT45":[68]},{"SHT45":[68]}]`. The setter writes it to EEPROM's data registers as ASCII JSON. The subsequent `((Device*)eeprom)->get(nullptr)` uses the input interface getter's default arguments to read from the EEPROM; the retrieved characters are copied into the EEPROM's read buffer and referenced from the interface cache. The final string comparison checks that cache against the original input. Both I2C status values must be `I2CIP_ERR_NONE`. |
| `test_device_delete` | The test's nominal expectation is that deleting the EEPROM completes. | Executes `delete eeprom`, then asserts the constant `true`. That assertion does **not** verify that a pointer was cleared, that destruction happened exactly once, or that the deleted object can no longer be accessed; instead a crash would indicate a problem. |

The call order matters: the byte write/read occurs first, followed by a two-byte
write/read at the same address. The final string-interface test then overwrites
the EEPROM with its configured JSON before reading and checking the cache.

## `test_4_bst` — binary search tree operations

All values in this sketch live in one global
`BST<unsigned, const char*> bst`. The sequence intentionally builds on the
prior test's state, so the tests are ordered.

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_bst_empty` | On the initially empty tree, both minimum and maximum nodes must be `nullptr`. | Calls `bst.findMin()` and `bst.findMax()`, which walk to the leftmost/rightmost node from the root; since the root is null, each returns null. |
| `test_bst_insert` | Inserting key `1` with value `"a"` must produce a node whose key is `1` and value is `"a"`. | Uses the node pointer returned by `bst.insert(1, "a")`; Unity reads the node's public `key` and `value` fields and compares the integer and string. |
| `test_bst_overwrite` | Inserting key `1` with `"b"` and `overwrite=false` must preserve `"a"`. Inserting the same key with the default overwrite option must change the value to `"b"`. | Both calls return the matching tree node. Its `value` field is checked after each call; the first checks duplicate-key preservation and the second checks replacement. |
| `test_bst_find` | Looking up key `1` must find key `1` with value `"b"`. | `bst.find(1)` traverses the tree by comparing the key to each node. The returned node's `key` and `value` are read and asserted. |
| `test_bst_remove` | Removing key `1` must leave no matching node; a subsequent `find(1)` must return `nullptr`. | Calls `bst.remove(1)`, then performs a separate lookup and asserts the returned node pointer is null. The remove method's returned pointer is not itself checked. |

## `test_5_hashtable` — hash table set/get/overwrite/remove

One global, initially empty `HashTable<int> hashtable` is shared across the
ordered tests. The hash table maps string keys to heap-allocated `int` values.
Optional serial output calls `toString()` to display table entries, but those
printed representations are not part of the assertions.

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_hashtable_empty` | Looking up `"null"` in the empty table must return a null value pointer. | The `operator[]` shortcut calls `get("null")`, which hashes the key, scans its bucket chain using string comparison, and returns null when no entry exists. |
| `test_hashtable_set` | Setting key `"test"` to a pointer to integer `1` must return an entry with key `"test"` and value `1`. | Calls `hashtable.set("test", new int(1))`; the returned `HashTableEntry<int>*` supplies the key and value pointer. The test dereferences `entry->value` to compare the stored integer. |
| `test_hashtable_overwrite` | With overwrite disabled, setting `"test"` to integer `2` must leave the stored value at `1`. With overwrite enabled, it must become `2`. | Allocates `y = new int(2)` and calls `set("test", y, false)`, then `set("test", y, true)`. Both return the existing entry; the test dereferences that entry's value after each call. The second call replaces the stored pointer with `y`. |
| `test_hashtable_get` | `get("test")` must find an entry with key `"test"` and value `2`; `hashtable["test"]` must be non-null and be the exact same pointer as `entry->value`. | Retrieves the entry with `get`, retrieves its value pointer separately with `operator[]`, and compares the key string, non-nullness, pointer identity, and dereferenced integer. |
| `test_hashtable_remove` | Removing `"test"` must return `true`; looking it up afterward must return `nullptr`. | `remove` locates the key in its hash bucket, unlinks and deletes the entry (the entry destructor also deletes its stored `int`). The test then checks the removal flag and performs a fresh `operator[]` lookup. |

## `test_6_module` — module setup, discovery, and EEPROM operations

This sketch creates `TestModule(0, 0)` and operates on its module EEPROM.
It needs a responding MUX at I2C bus 0, module 0 and a compatible EEPROM at `0x50` on MUX bus 0 whose contents can be read and parsed as module configuration. The module tests
are stateful: initialization and discovery happen in `setup()`, and repeated
checks happen in `loop()`.

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_module_init` | Allocating the module must produce a non-null pointer. | Executes `new TestModule(0, 0)` and asserts the assigned `Module* m` is non-null. If allocation returns null, the test enters an LED-blink loop instead of continuing. |
| `test_module_discovery` | `discoverEEPROM()` must return `I2CIP_ERR_NONE`; an EEPROM device group, its EEPROM object, the expected FQA, and the global device-tree entry must all be present and refer to the same device. | `discoverEEPROM()` registers the module EEPROM, checks the MUX, reads EEPROM contents through the EEPROM input interface, and parses the contents into module devices. The test obtains the group using `m->operator[](EEPROM::getID())`, obtains the EEPROM from that group using the module EEPROM's FQA, derives the expected FQA with `createFQA(m->getWireNum(), m->getModuleNum(), 0, I2CIP_EEPROM_ADDR)`, and looks up the same FQA in `I2CIP::devicetree`. It compares the FQA and the stored `Device*`. |
| `test_module_self_check` | The module's self-check must return `I2CIP_ERR_NONE`. | Calls `m->operator()()`. After discovery, the module checks the MUX and pings its EEPROM; an error also sets the sketch's `end` flag so subsequent loop checks are skipped. |
| `test_module_eeprom_check` | The typed check of the module EEPROM must return `I2CIP_ERR_NONE`. | Calls `m->operator()<EEPROM>(m->operator I2CIP::EEPROM &())`, passing the module's EEPROM reference to the module call operator. This exercises the module's EEPROM/device handling path and checks its returned error level. |
| `test_module_eeprom_update` | The typed EEPROM update must return `I2CIP_ERR_NONE`. | Calls the same typed module call operator with `update=true`, causing the module/device path to perform its update work against EEPROM. Only the returned error level is asserted; the commented-out cache/value comparisons are not run, and the test explicitly does not verify overwrite contents. This is left for `test_3_eeprom`. |
| `test_module_delete` | The test expects control to reach module deletion. | Executes `delete(m)` and then calls `TEST_PASS()`. The test does not set `m` to null or verify destruction beyond reaching the pass call. |

The self-check, EEPROM check, and EEPROM update run in the loop while `end` is
false. Starting from `count = 2`, the loop permits three passes through these
checks before deleting the module and calling `UNITY_END()`, unless a check
reports an error first. On an error, later checks are skipped and cleanup/end
is reached by the loop's error branch.

## `test_7_debug` — live module detection and DebugJson integration

This sketch is an interactive hardware/serial smoke-test loop rather than a
conventional assertion suite. It waits for `Serial` to become available,
starts Unity, then repeatedly unloads failed modules, probes each possible MUX
number, and calls `DebugJson::update(Serial, I2CIP::commandRouter)`.

| Test | Point and expected value | How the actual value is obtained |
| --- | --- | --- |
| `test_load_modules` | Operationally, each MUX number from `0` through `I2CIP_MUX_COUNT - 1` (0–7) is probed. A responding MUX gets a `DebugModule` if one is not already allocated, then the module's call operator performs discovery/self-check. A successful module sends its revision over Serial; a missing MUX gets `I2CIP_ERR_HARD`. | For each `m`, `MUX::pingMUX(TEST_7_DEBUG_WIRENUM, m)` checks the I2C ACK on wire 0. The code updates `I2CIP::modules[m]` and `I2CIP::errlev[m]` based on the ping and module call. However, the function unconditionally calls `TEST_IGNORE_MESSAGE(msg.c_str())` for every MUX slot. It contains **no assertion** that distinguishes success from failure, and the Unity outcome is reported as ignored rather than as a pass/fail validation of the module. |
| `test_unload_modules` | Modules marked with `I2CIP_ERR_HARD` should be deleted and their slots set to null. | Iterates over the global module and error-level arrays; if the pointer is non-null and its recorded error is hard, it deletes the module and assigns `nullptr`. There is no Unity assertion checking the resulting pointer. |

`setup()` sets the LED pin as output, initializes Serial at 115200 baud, and
waits in a blink loop until a Serial connection is present. The main loop runs
`test_unload_modules`, then `test_load_modules`, pauses, and lets DebugJson
process serial commands. The `DebugModule` factory recognizes SHT45, JHD1313,
rotary encoder, and EEPROM device groups. Since test results are explicitly
ignored in `test_load_modules`, use its serial output and the DebugJson
interaction to observe behavior; do not treat the Unity ignored status as
proof that all modules passed.

## Interpreting the suite

- **Pure software checks:** FQA packing/decoding, the FQA-to-wire pointer
  mapping, MUX helper conversions, BST behavior, and hash-table behavior do
  not require I2C peripherals.
- **Physical I/O checks:** LED readback uses the board GPIO implementation.
  MUX and EEPROM tests use actual I2C transactions and need correctly connected
  hardware and compatible wiring/addresses.
- **Order and side effects:** EEPROM and data-structure tests share state
  within each sketch. The EEPROM sketch writes test bytes and then replaces
  EEPROM contents with the configured JSON string. The hash-table remove test
  deletes the allocated integer associated with `"test"`.
- **Assertions vs. smoke tests:** Some checks assert only transaction status,
  not resulting content (`test_eeprom_write_byte`, `test_eeprom_write_word`).
  Some pass markers are not substantive validation (`test_device_delete`,
  `test_module_delete`), and the module-debug sketch reports each slot as
  ignored rather than asserting success or failure.