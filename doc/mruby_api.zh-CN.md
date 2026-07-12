# mruby API 规范

本文档描述 OpenBlink 脚本可用的 Ruby API。

OpenBlink 核心仅提供 `Blink` 类。`LED`、`Input`、`BLE` 等硬件类由各平台集成定义(通过 `openblink_hal_define_api()`);其规范请参阅相应的平台仓库。

此外,mruby/c 4.0.0 的内置类均可使用,包括用于多任务控制的 `Task`、`Mutex` 和 `VM`。

---

## Blink 类

### lock 方法 & unlock 方法

持有锁期间,字节码重载('L' 命令)会被拒绝,从而保护临界区不被中断。

#### 参数

无

#### 返回值 (bool)

- true: 成功
- false: 失败

#### 代码示例

```ruby
if Blink.lock
  # 不允许 Blink 的处理
  Blink.unlock
end
```

注:已弃用的 `Blink.req_reload?` 方法在核心 v0.4.0 中被移除。该方法始终返回 `false`;请删除相关调用或重构循环。
