# OpenBlink Bluetooth Communication Specification

## Overview

This document describes the Bluetooth Low Energy (BLE) binding of the OpenBlink Blink protocol. BLE is one possible transport for the core protocol defined in `protocol.md`; the frame formats, CRC, and response strings are transport-independent and specified there. This binding is implemented by BLE-capable platform integrations, not by the OpenBlink core itself.

## Service and Characteristics

### OpenBlink Service

- **Service UUID**: `227da52c-e13a-412b-befb-ba2256bb7fbe`
- **Description**: Primary service for OpenBlink device communication

### Characteristics

| Characteristic | UUID                                   | Properties                            | Description                                                    |
| -------------- | -------------------------------------- | ------------------------------------- | -------------------------------------------------------------- |
| Program        | `ad9fdd56-1135-4a84-923c-ce5a244385e7` | Write, Write Without Response, Notify | Blink protocol frames (write) and protocol responses (notify)  |
| Console        | `a015b3de-185a-4252-aa04-7a87d38ce148` | Notify                                | mruby/c console output (`puts`, etc.)                          |
| Status         | `ca141151-3113-448b-b21a-6a6203d253ff` | Read                                  | Device status information (e.g. negotiated MTU)                |

## Mapping to the Core Protocol

- Each GATT write to the Program characteristic carries exactly one Blink protocol frame and is passed to `openblink_receive()` unmodified. Framing is therefore provided by GATT itself.
- Protocol responses (`OK slot:<n>` and `ERROR: ...` strings, see `protocol.md`) are delivered to the host as notifications on the **Program** characteristic via the platform's `openblink_hal_send_response()` implementation.
- Console output from Ruby scripts is delivered as notifications on the **Console** characteristic.
- The Status characteristic is handled entirely by the platform layer (e.g. returning the negotiated GATT MTU so the host can size its 'D' chunks); the core is not involved.
- The maximum 'D' frame size is bounded by the negotiated ATT MTU; hosts should query the Status characteristic and split bytecode into chunks accordingly.

## Frame Byte Layouts

See `protocol.md` for the authoritative definition. Summary (all multi-byte fields little-endian):

### Common header (all frames)

| Offset | Size | Field   | Description                   |
| ------ | ---- | ------- | ----------------------------- |
| 0      | 1    | version | Protocol version (0x01)       |
| 1      | 1    | command | 'D', 'P', 'R', or 'L'         |

### 'D' frame (6 bytes + payload)

| Offset | Size     | Field  | Description                   |
| ------ | -------- | ------ | ----------------------------- |
| 2      | 2        | offset | Offset in the receive buffer  |
| 4      | 2        | size   | Payload size in bytes         |
| 6      | variable | data   | Bytecode payload              |

### 'P' frame (8 bytes)

| Offset | Size | Field    | Description                   |
| ------ | ---- | -------- | ----------------------------- |
| 2      | 2    | length   | Total bytecode length         |
| 4      | 2    | crc      | CRC16 checksum                |
| 6      | 1    | slot     | Target slot (1-origin)        |
| 7      | 1    | reserved | Reserved for future use       |

## Communication Flow

### Bytecode Transfer and Execution

```
Client                                      OpenBlink Device
  |                                               |
  |--- Discover OpenBlink Service --------------->|
  |<-- Service and Characteristics Found ---------|
  |--- Subscribe to Program Notifications ------->|
  |                                               |
  |--- Write Data Chunk 1 to Program Char ------->|
  |--- Write Data Chunk 2 to Program Char ------->|
  |--- Write Data Chunk n to Program Char ------->|
  |                                               |
  |--- Write Program Command to Program Char ---->|
  |                   (CRC check)                 |
  |<-- Notify "OK slot:<n>" on Program Char ------|
  |                                               |
  |--- Write Reset ('R') or Reload ('L') -------->|
  |                                               |
```

## Error Handling

Protocol errors are reported as notifications on the Program characteristic. The full list of response strings is defined in `protocol.md`.

## Implementation Notes

- The maximum bytecode size defaults to 4016 bytes (`OPENBLINK_MAX_BYTECODE_SIZE`).
- CRC16 parameters (reflected polynomial 0xD175, seed 0xFFFF) are defined in `protocol.md`.
- The UUIDs above must be kept stable so existing clients (WebIDE, VS Code extension) remain compatible.
