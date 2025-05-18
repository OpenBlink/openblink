# mruby API 规范

---

## Input 类

### pressed? 方法

#### 参数

| 名称  | 值                     | 是否可选 | 类型         | 备注 |
| ----- | ---------------------- | -------- | ------------ | ---- |
| part: | :sw1, :sw2, :sw3, :sw4 | 否       | 关键字(符号) |      |

#### 返回值 (bool)

- true: 按钮被按下
- false: 条件不满足

#### 代码示例

```ruby
Input.pressed?(part: :sw1)
```

### released? 方法

#### 参数

| 名称  | 值                     | 是否可选 | 类型         | 备注 |
| ----- | ---------------------- | -------- | ------------ | ---- |
| part: | :sw1, :sw2, :sw3, :sw4 | 否       | 关键字(符号) |      |

#### 返回值 (bool)

- true: 按钮被释放
- false: 条件不满足

#### 代码示例

```ruby
Input.released?(part: :sw1)
```

---

## LED 类

### set 方法

#### 参数

| 名称   | 值 (**粗体**: 默认) | 是否可选 | 类型         | 备注                 |
| ------ | ------------------- | -------- | ------------ | -------------------- |
| part:  | :led1, :led2, :led3 | 否       | 关键字(符号) | :led3 仅用于系统任务 |
| state: | true, **false**     | 是       | 关键字(bool) |                      |

#### 返回值 (bool)

- true: 成功
- false: 失败

#### 代码示例

```ruby
LED.set(part: :led1, state: true)
```

---

## BLE 类

### state 方法

#### 参数

无

#### 返回值 (int)

- 0: BLE 关闭状态
- 1: BLE 广播状态
- 2: BLE 已连接状态

#### 代码示例

```ruby
case BLE.state()
  when 0
    # 关闭
  when 1
    # 广播中
  when 2
    # 已连接
end
```

---

## Blink 类

### req_reload? 方法

#### 参数

无

#### 返回值 (bool)

- true: 存在重载请求
- false: 无重载请求

#### 代码示例

```ruby
while true
  return if Blink.req_reload?
  # 主要处理
end
```
