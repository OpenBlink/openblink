# mruby API Specification

---

## Input Class

### pressed? Method

#### Arguments

| Name  | Values                 | Optional | Type            | Notes |
| ----- | ---------------------- | -------- | --------------- | ----- |
| part: | :sw1, :sw2, :sw3, :sw4 | No       | Keyword(Symbol) |       |

#### Return Value (bool)

- true: Button is pressed
- false: Condition not met

#### Code Example

```ruby
Input.pressed?(part: :sw1)
```

### released? Method

#### Arguments

| Name  | Values                 | Optional | Type            | Notes |
| ----- | ---------------------- | -------- | --------------- | ----- |
| part: | :sw1, :sw2, :sw3, :sw4 | No       | Keyword(Symbol) |       |

#### Return Value (bool)

- true: Button is released
- false: Condition not met

#### Code Example

```ruby
Input.released?(part: :sw1)
```

---

## LED Class

### set Method

#### Arguments

| Name   | Values (**bold**: default) | Optional | Type            | Notes                          |
| ------ | -------------------------- | -------- | --------------- | ------------------------------ |
| part:  | :led1, :led2, :led3        | No       | Keyword(Symbol) | :led3 is for system tasks only |
| state: | true, **false**            | Yes      | Keyword(bool)   |                                |

#### Return Value (bool)

- true: Success
- false: Failure

#### Code Example

```ruby
LED.set(part: :led1, state: true)
```

---

## BLE Class

### state Method

#### Arguments

None

#### Return Value (int)

- 0: BLE Off state
- 1: BLE Advertising state
- 2: BLE Connected state

#### Code Example

```ruby
case BLE.state()
  when 0
    # Off
  when 1
    # Advertising
  when 2
    # Connected
end
```

---

## Blink Class

### req_reload? Method

#### Arguments

None

#### Return Value (bool)

- true: Reload request exists
- false: No reload request

#### Code Example

```ruby
while true
  return if Blink.req_reload?
  # Main processing
end
```
