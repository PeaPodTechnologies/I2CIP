# I2CIP

Inter-Integrated Circuit Intranet Protocols: A library of protocols for dynamic routing and interfacing with I2C devices on a modular switched network.

## I2C Intranet

It is advantageous to view a collection of devices on an I2C bus as a network - analogous to the Internet - under certain conditions:

- Dynamic Routing: ***Device reachability is subject to change.*** This enables physical “plug-and-play” functionality.
- Modularity: ***Devices can be added to the network as physical collections.*** This enables lifecycle management for physical collections of devices, and informs dynamic routing.

In order to achieve an *intra-network* of I2C devices, we propose a suite of protocols built on top of I2C which together form the I2C Intranet Protocol, along with a compatible hardware specification.

### Why I2C?

Well, from NXP (the current owners of Philips Semiconductor, the original filer of the I2C standard in 1980) we have this Pro/Con list for I2C among a variety of other serial communications protocols:

```markdown
Pros:
- Simple
- Well known
- Universally accepted
- Plug & Play
- Large portfolio
- Cost effective

Cons:

- Limited speed
```

# Overview

> Since I2C follows a controller-target topology, only the controller device needs to implement these protocols.

## OSI Model for I2C

| OSI Layer         | IP Implementation                 | I2CIP Equivalent        |
| ----------------- | --------------------------------- | ----------------------- |
| Physical          | Ethernet, MAC Addresses           | I2C SDA, SCL; addresses |
| Data Link         | IP packets                        | I2C packets             |
| Network           | IP addresses, routing, switching  | Multiplexers, FQA       |
| Transport         | TCP                               | Bottom-Up QOS 2         |

## Multiplexers

On an I2C bus, only one device may occupy any given 7-bit I2C address at a time. By splitting the I2C bus into many individually-selectable sub-busses/"subnetworks" using a multiplexer ("MUX"), one is able to communicate with devices that share I2C/device-address space independently on multiple subnets.

MUX bus 0 is reserved as the "default bus". SPRT EEPROM must be located on this bus.

MUX bus 7 is reserved as the "inactive bus". In order to forbid unintended communication, multiplexers are switched to this bus when not in use. NO devices can be located on this bus.

## Fully Qualified Addressing (FQA)

> **FQA** defines 3 new prefixed address segments for addressing devices on an I2C Intranet. A necessary expansion on the I2C *device-address* system.

1. The first 3 bits are reserved to indicate which of up to 8 I2C busses this Network is on. `LSB 13, LEN 3`
2. There are 8 reserved multiplexer addresses: 0x70 → 0x77. The following 3 bits are used to encode the “module address”. `LSB 10, LEN 3`
3. There are 8 busses available on each multiplexer. 3 bits are used to encode the bus. `LSB 7, LEN 3 `
4. There are 7 bits of uniquely-addressable *device-address* space on each I2C bus. `LSB 0, LEN 7`

**FQA size: 16 bits.** Together, segments 1 through 3 (bits 7 through 15) define the **Subnet**, while segment 4 (bits 0 through 6 still define the **device-address**.

Segment Extraction: `(address >> LSB) & (0xFFFF >> (16 - LEN))`

Visualized in binary: `0bNETMODBUSADDRESS`

In decimal, like an IP address: `N : M : B : ADR`.

E.g. `0:3:1:043` (`0b000:0 11:00 1:010 1011`, `0x0668`) indicates a device on I2C bus 0, module 3 (MUX address 0x73), MUX bus 1 (MUX instruction `0b00000010`), device address 43 (`0x2B`).

## Protocols

| I2CIP Protocol         | IP Equivalent | Function                                                                                                         | Library Implementation                                                                                    | OSI Layer(s)       |
| ---------------------- | ------------- | ---------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------- | ------------------ |
| Device Lookup Protocol | ARP           | Allows device groups to be looked up by ID. Informs "keep-alive" in a rebuild.																		| Routing Table (Hash Table): Device ID → FQA [ ]                                           								| Data Link, Network |
| Reverse DL Protocol    | RARP          | Allows device IDs to be looked up by FQA.																																				| Routing Table (BST): FQA → Device ID 																																			| Data Link, Network |
| Network Scan Protocol  | RIP           | Builds the Routing Table. Uses SPRT EEPROM and any local SPRT to attempt address recognition (otherwise ignore). | Ping for modules + One-time local all-scan → Read EEPROM & One-time module all-scan → Build Routing Table | Data Link, Network |
| Bus Switching Protocol | SNMP          | MUX bus switching                                                                                                | Write `1 << BUS` to MUX                                                                                   | Network            |
|                        |               |                                                                                                                  |                                                                                                           |                    |

> Note: ARP and RARP are two different traversal methods of the same tree. This is done in order to minimize heap memory allocation (necessary on microcontroller architectures).

# Compatible Hardware Specification

## Static Partial Routing Table (SPRT) EEPROM

> EEPROM on Bus 0 at address `0x50` stores a UTF-8-encoded JSON-formatted partial routing table (with *decimal-encoded* addresses).

A partial routing table (see below) indexes only the devices on a bus.

```json
// Indexed by BUS
[
	// Bus 0
	{
		// Array is used to list all occurrences of flexible-address devices
		"deviceid" : [ 65 ], // Decimal encoding (0x41).
		"..." : [],
	},
	//Bus 1, etc. up to Bus 7 (makes 8 buses)
	{
		"..." : [],
	},
]
```

> Note: when flashed, all comments, whitespaces, and trailing commas MUST be removed. All strings must be double quotes.

# Software Development

## Paradigm

- RoutingTable class *composes* (de/allocates) the actual stored values as *device groups* and uses BST/HT simply to sort them for traversal by FQA or ID (respectively)
- BST/Hash Table utility classes *aggregate* the values they hold; this means they are stored by *reference* (template type T: `T& value`) and do *NOT* manage the lifecycle of the values they store

- Both use entry classes and creates them by *construction* (passing in values *by reference*); this prevents unnecessary allocation by `malloc`
- Utility classes store and return entries by *pointer*; this allows getters to return `nullptr` if no matching entry is found