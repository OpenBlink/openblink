# OpenBlink Blink プロトコル仕様

## 概要

本書は、OpenBlink デバイス上の mruby/c バイトコードを転送・制御するための、transport 非依存のワイヤプロトコルを説明します。プロトコルは core の `openblink_receive()` で処理されます。フレームの伝送方法(BLE、UART、TCP など)は transport binding が定義します(BLE binding は `bluetooth_specification.ja.md` を参照)。

## フレーミング

- 1 フレーム = `openblink_receive()` の 1 回の呼び出し。
- フレーミング(ワイヤ上のフレーム区切り)は transport の責務です。例えば BLE binding では 1 回の GATT write が 1 フレームに対応します。
- 複数バイトのフィールドはすべてリトルエンディアンです。
- プロトコルバージョン: `0x01`。

## フレームフォーマット

すべてのフレームは 2 バイトの共通ヘッダで始まります:

| Offset | Size | Field   | 説明                                 |
| ------ | ---- | ------- | ------------------------------------ |
| 0      | 1    | version | プロトコルバージョン (0x01)          |
| 1      | 1    | command | 'D'、'P'、'R'、'L' のいずれか        |

### 'D' — データチャンク

バイトコードのチャンクをデバイスの受信バッファへ転送します。

| Offset | Size     | Field  | 説明                                 |
| ------ | -------- | ------ | ------------------------------------ |
| 0      | 2        | header | version + 'D'                        |
| 2      | 2        | offset | 受信バッファ内のオフセット (LE)      |
| 4      | 2        | size   | ペイロードのバイト数 (LE)            |
| 6      | 可変     | data   | バイトコードペイロード(`size` バイト) |

制約: フレーム長は `6 + size` と一致し、`offset + size` は最大バイトコードサイズ(既定 4016 バイト)を超えてはなりません。'D' フレームの成功時に応答はありません。

### 'P' — プログラム

受信済みバイトコードを検証してスロットへ保存します。フレーム長は 8 バイト固定です。

| Offset | Size | Field    | 説明                                 |
| ------ | ---- | -------- | ------------------------------------ |
| 0      | 2    | header   | version + 'P'                        |
| 2      | 2    | length   | バイトコード全長 (LE)                |
| 4      | 2    | crc      | バイトコードの CRC16 (LE)            |
| 6      | 1    | slot     | 対象スロット(1 始まり)             |
| 7      | 1    | reserved | 将来のための予約                     |

成功するとバイトコードは不揮発ストレージへ保存され、応答 `OK slot:<n>` が送信されます。受信バッファは 'P' フレームごとにクリアされます。

### 'R' — リセット

2 バイトフレーム(ヘッダのみ)。デバイスを再起動します。応答はありません。

### 'L' — リロード

2 バイトフレーム(ヘッダのみ)。mruby/c VM を再起動し、保存済みバイトコードを再読み込みします。VM ロック(`Blink.lock`)が保持されている間、リロード要求は破棄されます。いずれの場合も応答はありません。

## CRC16

'P' コマンドはバイトコード全体(受信バッファの `length` バイト)の CRC16 を含みます:

- アルゴリズム: ビット反転 CRC16(LSB ファースト)
- 多項式: 0xD175(反転形。32751 ビットまでのデータ長でハミング距離 4 の保護)
- 初期値: 0xFFFF
- 最終 XOR なし

参考: https://users.ece.cmu.edu/~koopman/crc/index.html

## 応答

応答は ASCII 文字列(NUL 終端なし)で、`openblink_hal_send_response()` を通じて送出されます。ホストへの到達方法は transport binding が定義します。

| 応答                                | 契機                                                          |
| ----------------------------------- | ------------------------------------------------------------- |
| `OK slot:<n>`                       | 'P' 成功。スロット `<n>` にバイトコードを保存                 |
| `ERROR: Blink size mismatch`        | フレームが 2 バイト未満、または 'P' フレーム長が 8 ではない   |
| `ERROR: Blink version mismatch`     | `version` が 0x01 ではない(以降の処理は行われない)          |
| `ERROR: Blink data size error`      | 'D' フレーム長が `6 + size` と一致しない                      |
| `ERROR: Size exceeds buffer limits` | 'D' の `offset + size` または 'P' の `length` がバッファ超過  |
| `ERROR: Invalid slot`               | 'P' の slot が `1..OPENBLINK_SLOT_COUNT` の範囲外             |
| `ERROR: CRC mismatch`               | 'P' の CRC16 検証に失敗                                       |
| `ERROR: Blink program error`        | 不揮発ストレージへの保存に失敗                                |
| `ERROR: Blink unknown type`         | 未知のコマンドバイト                                          |

互換性の注記: core v0.4.0 で追加された応答は `ERROR: Invalid slot` のみで、その他の応答とフレームフォーマットはファームウェア v0.3.x と同一です。v0.3.x では不正な slot は黙って slot 1 に保存され、version 不一致でも処理が継続されていましたが、いずれも現在は拒否されます。
