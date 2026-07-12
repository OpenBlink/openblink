# mruby API 仕様

本書は OpenBlink スクリプトで利用できる Ruby API を説明します。

OpenBlink core が提供するのは `Blink` クラスのみです。`LED`、`Input`、`BLE` などのハードウェアクラスは各プラットフォーム統合が定義します(`openblink_hal_define_api()` 経由)。仕様は各プラットフォームのリポジトリを参照してください。

さらに、mruby/c 4.0.0 の組み込みクラスが利用できます。マルチタスク制御のための `Task`、`Mutex`、`VM` を含みます。

---

## Blink クラス

### lock メソッド & unlock メソッド

ロック保持中はバイトコードのリロード('L' コマンド)が拒否され、クリティカルセクションが中断から保護されます。

#### 引数

なし

#### 戻り値 (bool)

- true: 成功
- false: 失敗

#### コード例

```ruby
if Blink.lock
  # Blinkを許可しない処理
  Blink.unlock
end
```

注記: 非推奨だった `Blink.req_reload?` メソッドは core v0.4.0 で削除されました。このメソッドは常に `false` を返していたため、呼び出しを削除するかループを書き換えてください。
