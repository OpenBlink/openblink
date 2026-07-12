# OpenBlink Bluetooth 通信仕様

## 概要

本書は、OpenBlink Blink プロトコルの Bluetooth Low Energy (BLE) binding を説明します。BLE は `protocol.ja.md` で定義された core プロトコルの transport の 1 つであり、フレームフォーマット・CRC・応答文字列は transport 非依存としてそちらで規定されています。本 binding は OpenBlink core ではなく、BLE 対応のプラットフォーム統合が実装します。

## サービスとキャラクタリスティック

### OpenBlink サービス

- **Service UUID**: `227da52c-e13a-412b-befb-ba2256bb7fbe`
- **説明**: OpenBlink デバイス通信のためのプライマリサービス

### キャラクタリスティック

| キャラクタリスティック | UUID                                   | プロパティ                            | 説明                                                          |
| ---------------------- | -------------------------------------- | ------------------------------------- | ------------------------------------------------------------- |
| Program                | `ad9fdd56-1135-4a84-923c-ce5a244385e7` | Write, Write Without Response, Notify | Blink プロトコルフレーム(write)とプロトコル応答(notify)   |
| Console                | `a015b3de-185a-4252-aa04-7a87d38ce148` | Notify                                | mruby/c コンソール出力(`puts` など)                         |
| Status                 | `ca141151-3113-448b-b21a-6a6203d253ff` | Read                                  | デバイス状態情報(ネゴシエート済み MTU など)                 |

## core プロトコルへのマッピング

- Program キャラクタリスティックへの 1 回の GATT write が Blink プロトコルフレーム 1 つを運び、そのまま `openblink_receive()` に渡されます。フレーミングは GATT 自体が提供します。
- プロトコル応答(`OK slot:<n>` および `ERROR: ...` 文字列、`protocol.ja.md` 参照)は、プラットフォームの `openblink_hal_send_response()` 実装により **Program** キャラクタリスティックの notification としてホストへ届けられます。
- Ruby スクリプトのコンソール出力は **Console** キャラクタリスティックの notification として届けられます。
- Status キャラクタリスティックは完全にプラットフォーム層が処理します(例: ホストが 'D' チャンクサイズを決められるようネゴシエート済み GATT MTU を返す)。core は関与しません。
- 'D' フレームの最大サイズはネゴシエート済み ATT MTU に制約されます。ホストは Status キャラクタリスティックを読み、バイトコードを適切なチャンクに分割してください。

## フレームのバイトレイアウト

正式な定義は `protocol.ja.md` を参照してください。概要(複数バイトのフィールドはすべてリトルエンディアン):

### 共通ヘッダ(全フレーム)

| Offset | Size | Field   | 説明                          |
| ------ | ---- | ------- | ----------------------------- |
| 0      | 1    | version | プロトコルバージョン (0x01)   |
| 1      | 1    | command | 'D'、'P'、'R'、'L' のいずれか |

### 'D' フレーム(6 バイト + ペイロード)

| Offset | Size | Field  | 説明                          |
| ------ | ---- | ------ | ----------------------------- |
| 2      | 2    | offset | 受信バッファ内のオフセット    |
| 4      | 2    | size   | ペイロードのバイト数          |
| 6      | 可変 | data   | バイトコードペイロード        |

### 'P' フレーム(8 バイト)

| Offset | Size | Field    | 説明                          |
| ------ | ---- | -------- | ----------------------------- |
| 2      | 2    | length   | バイトコード全長              |
| 4      | 2    | crc      | CRC16 チェックサム            |
| 6      | 1    | slot     | 対象スロット(1 始まり)      |
| 7      | 1    | reserved | 将来のための予約              |

## 通信フロー

### バイトコードの転送と実行

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

## エラーハンドリング

プロトコルエラーは Program キャラクタリスティックの notification として報告されます。応答文字列の一覧は `protocol.ja.md` で定義されています。

## 実装上の注意

- 最大バイトコードサイズの既定値は 4016 バイトです(`OPENBLINK_MAX_BYTECODE_SIZE`)。
- CRC16 のパラメータ(反転多項式 0xD175、初期値 0xFFFF)は `protocol.ja.md` で定義されています。
- 上記 UUID は、既存クライアント(WebIDE、VS Code 拡張)との互換性を保つため変更しないでください。
