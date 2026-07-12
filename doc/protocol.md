# OpenBlink Blink Protocol Specification

## Overview

This document describes the transport-independent wire protocol used to transfer and control mruby/c bytecode on an OpenBlink device. The protocol is processed by the core through `openblink_receive()`; how frames are carried (BLE, UART, TCP, ...) is defined by a transport binding (see `bluetooth_specification.md` for the BLE binding).

## Framing

- One frame corresponds to exactly one call to `openblink_receive()`.
- Framing (delimiting frames on the wire) is the responsibility of the transport. For example, the BLE binding maps one GATT write to one frame.
- All multi-byte fields are little-endian.
- Protocol version: `0x01`.

## Frame Formats

Every frame starts with a 2-byte common header:

| Offset | Size | Field   | Description                          |
| ------ | ---- | ------- | ------------------------------------ |
| 0      | 1    | version | Protocol version (0x01)              |
| 1      | 1    | command | 'D', 'P', 'R', or 'L'                |

### 'D' — Data chunk

Transfers a chunk of bytecode into the device's receive buffer.

| Offset | Size     | Field  | Description                          |
| ------ | -------- | ------ | ------------------------------------ |
| 0      | 2        | header | version + 'D'                        |
| 2      | 2        | offset | Offset in the receive buffer (LE)    |
| 4      | 2        | size   | Size of the payload in bytes (LE)    |
| 6      | variable | data   | Bytecode payload (`size` bytes)      |

Constraints: the frame length must equal `6 + size`, and `offset + size` must not exceed the maximum bytecode size (default 4016 bytes). A successful 'D' frame produces no response.

### 'P' — Program

Verifies the received bytecode and stores it into a slot. Frame length must be exactly 8 bytes.

| Offset | Size | Field    | Description                          |
| ------ | ---- | -------- | ------------------------------------ |
| 0      | 2    | header   | version + 'P'                        |
| 2      | 2    | length   | Total bytecode length (LE)           |
| 4      | 2    | crc      | CRC16 of the bytecode (LE)           |
| 6      | 1    | slot     | Target slot (1-origin)               |
| 7      | 1    | reserved | Reserved for future use              |

On success the bytecode is persisted to non-volatile storage and the response `OK slot:<n>` is sent. The receive buffer is cleared after every 'P' frame.

### 'R' — Reset

2-byte frame (header only). Reboots the device. No response is sent.

### 'L' — Reload

2-byte frame (header only). Restarts the mruby/c VM so the stored bytecode is reloaded. No response is sent.

## CRC16

The 'P' command carries a CRC16 of the full bytecode (`length` bytes of the receive buffer):

- Algorithm: bit-reflected CRC16 (LSB-first)
- Polynomial: 0xD175 (reflected form; Hamming Distance 4 protection for data lengths up to 32751 bits)
- Initial value: 0xFFFF
- No final XOR

Reference: https://users.ece.cmu.edu/~koopman/crc/index.html

## Responses

Responses are ASCII strings (no NUL terminator) delivered through `openblink_hal_send_response()`. The transport binding defines how they reach the host.

| Response                            | Trigger                                                      |
| ----------------------------------- | ------------------------------------------------------------ |
| `OK slot:<n>`                       | 'P' succeeded; bytecode stored in slot `<n>`                 |
| `ERROR: Blink size mismatch`        | Frame shorter than 2 bytes, or 'P' frame length is not 8     |
| `ERROR: Blink version mismatch`     | `version` is not 0x01 (frame is not processed further)       |
| `ERROR: Blink data size error`      | 'D' frame length does not equal `6 + size`                   |
| `ERROR: Size exceeds buffer limits` | 'D' `offset + size` or 'P' `length` exceeds the buffer size  |
| `ERROR: Invalid slot`               | 'P' slot is outside `1..OPENBLINK_SLOT_COUNT`                |
| `ERROR: CRC mismatch`               | 'P' CRC16 verification failed                                |
| `ERROR: Blink program error`        | Storing the bytecode to non-volatile storage failed          |
| `ERROR: Blink unknown type`         | Unknown command byte                                         |

Compatibility note: `ERROR: Invalid slot` is the only response added in core v0.4.0; all other responses and the frame formats are identical to firmware v0.3.x. In v0.3.x an invalid slot was silently stored to slot 1, and a version mismatch did not stop processing; both are now rejected.
