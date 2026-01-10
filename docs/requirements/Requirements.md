# I2CIP: Inter-Integrated Circuit Intra-networking Protocols

Requirements for a Hardware Design Specification for a Bus-Switched Intra-Network of Hot-Swap Modules of I2C Targets and a Software Library of Intra-Network Communications Protocols for Rapid Implementation of Plug-and-Play Embedded Systems

## Scope and Justification

### I2C Specification

From the I2C Specification Version 7 (2021, *NXP Semiconductors*): an 8-bit-oriented one-ended (“controller”-driven) bidirectional (read & write) serial communication over a 2-wire bus (data “SDA” & clock “SCL”) for integrated circuit devices (“targets”), including (but not limited to):

- **Remote Multi-Channel Ports**: GPIO banks, internal-clock PWM
  drivers, Analog-to-Digital and Digital-to-Analog converters, etc.

- **System Devices**: real-time clocks, LCD screens, microcontrollers,
  etc.

- **Data Storage Devices**: EEPROM, SRAM, FRAM, etc.

- **Digital Sensors**: temperature, humidity, light, acceleration,
  pressure, etc.

### OSI Model Analogue for I2C

The I2C Specification can be imagined as an incomplete analogue to the Internet’s OSI Model, with the following layers defined:

1. **Physical Layer**

    1. **VDD & GND**: e.g. +5 VDC

    2. **SDA & SCL**: Pull-Up  Bias Resistors (e.g. 10 k*Ω*)

2. **Data Link Layer**

    1. **Controller**: Bus Speed Control, Start & Stop Conditions, Multi-Controller Arbitration

    2. **Targets**: 7-bit Device Addressing, Acknowledgement (“ACK”)

    3. **Packet Structure**: “Read” & “Write” Flags, 8-/16-bit Register Addressing, Byte-Stream Data

The **Network** (data routing), **Transport** (data delivery), and **Session** (transmission context) layers of the OSI model analogy are not defined by the I2C Specification. The following proposed extensions to the I2C Specification, the focus of the **I2CIP** design, are intended to fill this gap, enabling **Presentation** and **Application** layer functionality to be rapidly implemented by developers for embedded systems (e.g. control systems).

### Switched-Bus Intra-Networking

Suppose an I2C target device *D* with *N* possible unique addresses. A standard I2C bus controller *C* can communicate with *N* uniquely-addressed instantiations of *D* on each I2C bus without modification to connections or encountering conflict.

Suppose a type of I2C target device *X* with *M* possible unique addresses that acts as a multiplexer and repeater ("switch") for *B* bitwise-enabled output busses ("subnetworks"). Using this switch device *X*, the I2C bus controller *C* can communicate with *M* \* *B* \* *N* independently-addressable instantiations of the target device *D* across *M* \* *B* subnetworks by setting ONE active output bus on each of the *M* switches (and disabling ALL on the remaining *M* − 1).

### Extended OSI Model for I2CIP

For the purposes of effective intra-network communication across switched subnetworks, it is proposed that a “fully-qualified address” be implemented at the controller level, comprising routing information that encodes the I2C bus, switch address, and subnetwork number, alongside the target device address, for each target device.

Suppose a dedicated target device *E* consisting of EEPROM memory containing routing information for all devices on all subnetworks of one switch. If this EEPROM device *E* is granted a fixed address on a consistent subnetwork on each switch, the controller can reliably retrieve routing information for all devices on all subnetworks of any switch by querying each switch’s dedicated EEPROM device.

Together, the EEPROM device *E*, the switch device *X*, and all devices on all subnetworks of the switch *X* comprise a **Module**.

1. **Network Layer**: Fully-Qualified Addressing (“FQA”)

2. **Transport Layer**: Switch & Target Ping Prior to Target Control with Quality-of-Service 2 (“only-once” delivery) via ACK

3. **Session Layer**: Target Discovery & Module Configuration via Dedicated EEPROM Target

## Definitions

A number of useful definitions have emerged from the above scoping:

1.  **Switch**: An I2C target device that acts as a repeater for bitwise-multiplexed output busses.

2.  **Subnetwork**: A specific output bus of a specific switch.

3.  **Intra-Network**: A general term referring to all routable targets (not including switches) on all subnetworks across all of a controller’s I2C busses.

4.  **Fully-Qualified Address (FQA)**: A unique intra-network routing location identifier, encoding: a specific I2C bus, and; a specific subnetwork (i.e. switch and bus), and; a target’s I2C address.

5.  **Module**: A switch, and; a physical collection of targets located on the switch’s subnetworks, and; a data storage target at a predetermined location with all routing information for all targets on this switch’s subnetworks.

# Framing

## Problem Statement

Formulate a hardware design specification for a bus-switched intra-network of hot-swap modules of I2C targets, and a software library of intra-network communications protocols for fully-qualified addressing and Quality-of-Service 2 packet delivery for all targets, including dedicated routing EEPROM targets for modular target lifecycle management and configuration.

## Solution Requirements

The following are the overall requirements implied from scoping and definitions of the I2CIP design:

1. **Must** include a hardware design specification of an I2C intra-network that:

    1. **Must** implement switching of output bus subnetworks while preventing cross-talk;

    2. **Must** implement hot-swap capability;

    3. **Must** implement dedicated routing EEPROM targets;

2. **Must** include a software library for operating the I2C intra-network that:

    1. **Must** implement fully-qualified addressing encoding routing information for any target on any subnetworks across all I2C busses;

    2. **Must** implement Quality-of-Service 2 packet delivery for all communications across the intra-network;

    3. **Must** implement plug-and-play modular target lifecycle management via dedicated routing EEPROM targets;

3. **Should** support multiple I2C bus controllers.

4. **Should** implement electrical isolation for all targets on all subnetworks.

## Stakeholders and Values

1. Embedded Systems Designers - Modularity, Scalability

2. System End-Users - Reliability, Maintainability

3. Hobbyists, Makers, Educators - Accessibility, Inspectability