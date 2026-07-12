# mruby API Specification

This document describes the Ruby API available to OpenBlink scripts.

The OpenBlink core provides only the `Blink` class. Hardware classes such as `LED`, `Input`, and `BLE` are defined by each platform integration (via `openblink_hal_define_api()`); consult your platform repository for their specifications.

In addition, the built-in classes of mruby/c 4.0.0 are available, including `Task`, `Mutex`, and `VM` for multitasking control.

---

## Blink Class

### lock Method & unlock Method

While the lock is held, bytecode reloads ('L' command) are rejected, protecting critical sections from being interrupted.

#### Arguments

None

#### Return Value (bool)

- true: Success
- false: Failure

#### Code Example

```ruby
if Blink.lock
  # Processing that does not allow Blink
  Blink.unlock
end
```

Note: the deprecated `Blink.req_reload?` method has been removed in core v0.4.0. Scripts using it always received `false`; remove the call or restructure the loop.
