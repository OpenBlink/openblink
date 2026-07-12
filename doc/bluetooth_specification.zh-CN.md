# OpenBlink 蓝牙通信规范

## 概述

本文档描述 OpenBlink Blink 协议的低功耗蓝牙 (BLE) 绑定。BLE 是 `protocol.zh-CN.md` 中定义的核心协议的一种传输方式;帧格式、CRC 和响应字符串与传输层无关,均在该文档中规定。本绑定由支持 BLE 的平台集成实现,而不是由 OpenBlink 核心实现。

## 服务与特征

### OpenBlink 服务

- **Service UUID**: `227da52c-e13a-412b-befb-ba2256bb7fbe`
- **说明**: OpenBlink 设备通信的主服务

### 特征

| 特征    | UUID                                   | 属性                                  | 说明                                                  |
| ------- | -------------------------------------- | ------------------------------------- | ----------------------------------------------------- |
| Program | `ad9fdd56-1135-4a84-923c-ce5a244385e7` | Write, Write Without Response, Notify | Blink 协议帧(写入)与协议响应(通知)                |
| Console | `a015b3de-185a-4252-aa04-7a87d38ce148` | Notify                                | mruby/c 控制台输出(`puts` 等)                       |
| Status  | `ca141151-3113-448b-b21a-6a6203d253ff` | Read                                  | 设备状态信息(如协商后的 MTU)                        |

## 与核心协议的映射

- 对 Program 特征的每次 GATT 写入恰好承载一个 Blink 协议帧,并原样传递给 `openblink_receive()`。分帧由 GATT 本身提供。
- 协议响应(`OK slot:<n>` 及 `ERROR: ...` 字符串,见 `protocol.zh-CN.md`)通过平台实现的 `openblink_hal_send_response()`,以 **Program** 特征的通知形式送达主机。
- Ruby 脚本的控制台输出以 **Console** 特征的通知形式送达。
- Status 特征完全由平台层处理(例如返回协商后的 GATT MTU,便于主机确定 'D' 数据块大小);核心不参与。
- 'D' 帧的最大尺寸受协商后 ATT MTU 限制;主机应读取 Status 特征并据此拆分字节码。

## 帧字节布局

权威定义见 `protocol.zh-CN.md`。概要(所有多字节字段均为小端序):

### 公共头(所有帧)

| 偏移 | 大小 | 字段    | 说明                          |
| ---- | ---- | ------- | ----------------------------- |
| 0    | 1    | version | 协议版本 (0x01)               |
| 1    | 1    | command | 'D'、'P'、'R' 或 'L'          |

### 'D' 帧(6 字节 + 载荷)

| 偏移 | 大小 | 字段   | 说明                          |
| ---- | ---- | ------ | ----------------------------- |
| 2    | 2    | offset | 接收缓冲区内的偏移            |
| 4    | 2    | size   | 载荷字节数                    |
| 6    | 可变 | data   | 字节码载荷                    |

### 'P' 帧(8 字节)

| 偏移 | 大小 | 字段     | 说明                          |
| ---- | ---- | -------- | ----------------------------- |
| 2    | 2    | length   | 字节码总长度                  |
| 4    | 2    | crc      | CRC16 校验和                  |
| 6    | 1    | slot     | 目标槽位(从 1 开始)         |
| 7    | 1    | reserved | 保留供将来使用                |

## 通信流程

### 字节码传输与执行

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

## 错误处理

协议错误以 Program 特征的通知形式报告。响应字符串的完整列表在 `protocol.zh-CN.md` 中定义。

## 实现说明

- 最大字节码大小默认为 4016 字节(`OPENBLINK_MAX_BYTECODE_SIZE`)。
- CRC16 参数(反射多项式 0xD175,初始值 0xFFFF)在 `protocol.zh-CN.md` 中定义。
- 上述 UUID 必须保持稳定,以保证现有客户端(WebIDE、VS Code 扩展)的兼容性。
