# OpenBlink Blink 协议规范

## 概述

本文档描述了用于在 OpenBlink 设备上传输和控制 mruby/c 字节码的、与传输层无关的线路协议。协议由核心的 `openblink_receive()` 处理;帧的承载方式(BLE、UART、TCP 等)由传输层绑定定义(BLE 绑定参见 `bluetooth_specification.zh-CN.md`)。

## 分帧

- 一帧对应一次 `openblink_receive()` 调用。
- 分帧(在线路上划分帧边界)是传输层的责任。例如,BLE 绑定将一次 GATT 写入映射为一帧。
- 所有多字节字段均为小端序。
- 协议版本:`0x01`。

## 帧格式

每帧都以 2 字节的公共头开始:

| 偏移 | 大小 | 字段    | 说明                                 |
| ---- | ---- | ------- | ------------------------------------ |
| 0    | 1    | version | 协议版本 (0x01)                      |
| 1    | 1    | command | 'D'、'P'、'R' 或 'L'                 |

### 'D' — 数据块

将一段字节码传输到设备的接收缓冲区。

| 偏移 | 大小 | 字段   | 说明                                 |
| ---- | ---- | ------ | ------------------------------------ |
| 0    | 2    | header | version + 'D'                        |
| 2    | 2    | offset | 接收缓冲区内的偏移 (LE)              |
| 4    | 2    | size   | 载荷字节数 (LE)                      |
| 6    | 可变 | data   | 字节码载荷(`size` 字节)            |

约束:帧长必须等于 `6 + size`,且 `offset + size` 不得超过最大字节码大小(默认 4016 字节)。'D' 帧成功时不产生响应。

### 'P' — 程序

校验已接收的字节码并存储到槽位。帧长固定为 8 字节。

| 偏移 | 大小 | 字段     | 说明                                 |
| ---- | ---- | -------- | ------------------------------------ |
| 0    | 2    | header   | version + 'P'                        |
| 2    | 2    | length   | 字节码总长度 (LE)                    |
| 4    | 2    | crc      | 字节码的 CRC16 (LE)                  |
| 6    | 1    | slot     | 目标槽位(从 1 开始)                |
| 7    | 1    | reserved | 保留供将来使用                       |

成功时字节码被持久化到非易失性存储,并发送响应 `OK slot:<n>`。每个 'P' 帧处理后接收缓冲区都会被清空。

### 'R' — 复位

2 字节帧(仅头部)。重启设备。不发送响应。

### 'L' — 重载

2 字节帧(仅头部)。重启 mruby/c 虚拟机以重新加载已存储的字节码。当 VM 锁(`Blink.lock`)被持有时,重新加载请求会被丢弃。两种情况下都不发送响应。

## CRC16

'P' 命令携带完整字节码(接收缓冲区的 `length` 字节)的 CRC16:

- 算法:位反射 CRC16(LSB 优先)
- 多项式:0xD175(反射形式;对不超过 32751 位的数据提供汉明距离 4 的保护)
- 初始值:0xFFFF
- 无最终异或

参考:https://users.ece.cmu.edu/~koopman/crc/index.html

## 响应

响应为 ASCII 字符串(无 NUL 结尾),通过 `openblink_hal_send_response()` 发出。如何送达主机由传输层绑定定义。

| 响应                                | 触发条件                                                     |
| ----------------------------------- | ------------------------------------------------------------ |
| `OK slot:<n>`                       | 'P' 成功;字节码已存入槽位 `<n>`                             |
| `ERROR: Blink size mismatch`        | 帧短于 2 字节,或 'P' 帧长不是 8                             |
| `ERROR: Blink version mismatch`     | `version` 不是 0x01(帧不再继续处理)                        |
| `ERROR: Blink data size error`      | 'D' 帧长不等于 `6 + size`                                    |
| `ERROR: Size exceeds buffer limits` | 'D' 的 `offset + size` 或 'P' 的 `length` 超出缓冲区大小     |
| `ERROR: Invalid slot`               | 'P' 的 slot 超出 `1..OPENBLINK_SLOT_COUNT` 范围              |
| `ERROR: CRC mismatch`               | 'P' 的 CRC16 校验失败                                        |
| `ERROR: Blink program error`        | 向非易失性存储写入字节码失败                                 |
| `ERROR: Blink unknown type`         | 未知的命令字节                                               |

兼容性说明:core v0.4.0 仅新增了 `ERROR: Invalid slot` 一种响应;其余响应和帧格式与固件 v0.3.x 完全一致。在 v0.3.x 中,无效的槽位会被静默存入槽位 1,版本不匹配也不会中止处理;现在这两种情况都会被拒绝。
