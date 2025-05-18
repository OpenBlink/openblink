# OpenBlink Bluetooth Communication Specification

## Overview

This document describes the Bluetooth Low Energy (BLE) communication specification for the OpenBlink system. It includes service and characteristic UUIDs, protocol details, data structures, and communication flow.

## Service and Characteristics

### OpenBlink Service

- **Service UUID**: `227da52c-e13a-412b-befb-ba2256bb7fbe`
- **Description**: Primary service for OpenBlink device communication

### Characteristics

| Characteristic | UUID                                   | Properties                    | Description                              |
| -------------- | -------------------------------------- | ----------------------------- | ---------------------------------------- |
| Program        | `ad9fdd56-1135-4a84-923c-ce5a244385e7` | Write, Write Without Response | Used for bytecode transfer and execution |
| Console        | `a015b3de-185a-4252-aa04-7a87d38ce148` | Notify                        | Used for debug output and notifications  |
| Status         | `ca141151-3113-448b-b21a-6a6203d253ff` | Read                          | Provides device status information       |

## Protocol

### Blink Protocol

- **Version**: 0x01
- **Description**: Protocol for transferring and executing bytecode on the OpenBlink device

### Command Types

| Command | Code | Description                       |
| ------- | ---- | --------------------------------- |
| Data    | 'D'  | Transfers a chunk of bytecode     |
| Program | 'P'  | Executes the transferred bytecode |
| Reset   | 'R'  | Resets the device                 |
| Reload  | 'L'  | Reloads the bytecode              |

## Data Structures

### BLINK_CHUNK_HEADER

- **Size**: 2 bytes
- **Description**: Common header for all Blink protocol commands

| Field   | Type    | Size   | Description                          |
| ------- | ------- | ------ | ------------------------------------ |
| version | uint8_t | 1 byte | Blink protocol version (0x01)        |
| command | uint8_t | 1 byte | Command type ('D', 'P', 'R', or 'L') |

### BLINK_CHUNK_DATA

- **Size**: 6 bytes + data
- **Description**: Structure for data chunk transfers

| Field  | Type               | Size     | Description               |
| ------ | ------------------ | -------- | ------------------------- |
| header | BLINK_CHUNK_HEADER | 2 bytes  | Common header             |
| offset | uint16_t           | 2 bytes  | Offset in bytecode buffer |
| size   | uint16_t           | 2 bytes  | Size of data chunk        |
| data   | uint8_t[]          | Variable | Actual bytecode data      |

### BLINK_CHUNK_PROGRAM

- **Size**: 8 bytes
- **Description**: Structure for program execution command

| Field    | Type               | Size    | Description              |
| -------- | ------------------ | ------- | ------------------------ |
| header   | BLINK_CHUNK_HEADER | 2 bytes | Common header            |
| length   | uint16_t           | 2 bytes | Total bytecode length    |
| crc      | uint16_t           | 2 bytes | CRC16 checksum           |
| slot     | uint8_t            | 1 byte  | Target slot for bytecode |
| reserved | uint8_t            | 1 byte  | Reserved for future use  |

## Communication Flow

### Bytecode Transfer and Execution

```
Client                                      OpenBlink Device
  |                                               |
  |--- Discover OpenBlink Service --------------->|
  |<-- Service and Characteristics Found ---------|
  |                                               |
  |--- Write Data Chunk 1 to Program Char ------->|
  |--- Write Data Chunk 2 to Program Char ------->|
  |--- Write Data Chunk n to Program Char ------->|
  |                                               |
  |--- Write Program Command to Program Char ---->|
  |                   (CRC check)                 |
  |                                               |
  |--- Write Reset/Reload Command --------------->|
  |                                               |
```

### BLE Events

| Event                  | Description                    |
| ---------------------- | ------------------------------ |
| BLE_EVENT_INITIALIZED  | BLE stack has been initialized |
| BLE_EVENT_CONNECTED    | BLE connection established     |
| BLE_EVENT_DISCONNECTED | BLE connection terminated      |
| BLE_EVENT_RECEIVED     | Data received over BLE         |
| BLE_EVENT_SENT         | Data sent over BLE             |
| BLE_EVENT_BLINK        | Blink bytecode received        |
| BLE_EVENT_STATUS       | Status information requested   |
| BLE_EVENT_REBOOT       | Reboot request received        |
| BLE_EVENT_RELOAD       | Reload request received        |

## Error Handling

### Error Notifications

Errors during bytecode transfer or execution are reported through notifications on the Console characteristic. The notification contains an error message string.

### Common Errors

| Error                               | Description                                  |
| ----------------------------------- | -------------------------------------------- |
| "ERROR: Blink version mismatch"     | Protocol version is not supported            |
| "ERROR: Blink data size error"      | Data chunk size does not match expected size |
| "ERROR: Size exceeds buffer limits" | Bytecode size exceeds maximum allowed size   |
| "ERROR: CRC mismatch"               | CRC checksum verification failed             |
| "ERROR: Blink program error"        | Error during bytecode execution              |
| "ERROR: Blink unknown type"         | Unknown command type received                |

## Implementation Notes

### Maximum Bytecode Size

The maximum bytecode size is defined by `BLINK_MAX_BYTECODE_SIZE` in the implementation.

### CRC Calculation

CRC16 checksum is calculated using the `crc16_reflect` function with the following parameters:

- Initial value: 0xd175
- Polynomial: 0xFFFF
- Input: bytecode buffer
- Length: bytecode length
