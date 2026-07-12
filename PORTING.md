# Porting Guide

This document describes how to integrate the OpenBlink core into a platform (RTOS, bare metal, Arduino, ESP-IDF, Zephyr, POSIX, ...). The core is platform-independent C11; a platform supplies the transport, storage, threading, and the mruby/c HAL.

## Architecture

```
+---------------------------------------------------------------+
| Platform repository (e.g. openblink-nrf, openblink-m5stack)   |
|  - transport (BLE/UART/TCP), storage, reboot, threads         |
|  - hardware Ruby classes (LED, Input, ...)                    |
|  - mruby/c HAL (hal.h / hal.c)                                |
+------------------------------+--------------------------------+
                               | openblink_hal_* functions
+------------------------------v--------------------------------+
| OpenBlink core (this repository, used as a git submodule)     |
|  - Blink protocol parser (src/blink.c)                        |
|  - slot management        (src/blink.c)                       |
|  - mruby/c VM lifecycle   (src/vm.c, src/blink_class.c)       |
+------------------------------+--------------------------------+
                               | mrbc_* / mrbc_hal_* functions
+------------------------------v--------------------------------+
| mruby/c (mrubyc/ submodule, release4.0.0)                     |
+---------------------------------------------------------------+
```

## Build Integration

Compile these sources into your firmware:

- `src/blink.c`, `src/vm.c`, `src/blink_class.c`
- all `mrubyc/src/*.c` files
- your platform's mruby/c HAL (`hal.c`), unless you use one shipped in `mrubyc/hal/<platform>`

Include paths:

- `-I <core>/include` (public headers, included as `openblink/blink.h` etc.)
- `-I <core>/mrubyc/src`
- `-I <core>/mrubyc/hal/<platform>` — `mrubyc.h` includes `"hal.h"`, so a directory providing `hal.h` **must** be on the include path. Use a HAL bundled with mruby/c (`mrubyc/hal/zephyr`, `mrubyc/hal/posix`, ...) or your own.

Required compiler definitions (mruby/c configuration):

- `MRBC_SCHEDULER_EXIT=1` — **mandatory**; `mrbc_run()` must return when all tasks end so the core can reload bytecode.
- `MAX_VM_COUNT` — must be `>= OPENBLINK_SLOT_COUNT` (mruby/c default is 5).
- `MRBC_USE_FLOAT=1`, `MRBC_USE_MATH=1` — recommended for compatibility with existing OpenBlink scripts.
- Optionally override `OPENBLINK_*` constants from `include/openblink/config.h` (e.g. `-DOPENBLINK_SLOT_COUNT=3`).

Autogen files: `mrubyc/src` contains generated `_autogen_*.h` headers. If they are missing, run `make autogen` in `mrubyc/` (requires Ruby).

Host smoke compile of the core:

```sh
gcc -std=c11 -Wall -Wextra -Werror -c src/*.c \
    -I include -isystem mrubyc/src -isystem mrubyc/hal/posix
```

## Initialization Order

1. `openblink_hal_storage_init()` (via your platform init) — storage must be ready before the VM loads bytecode.
2. Start the transport (BLE advertising, UART, ...).
3. Start the VM: call `openblink_vm_main()` from a dedicated thread or your main loop. It never returns.

Since the bytecode buffers and the mruby/c heap are static in `src/vm.c`, the VM thread only needs a stack large enough for mruby/c execution itself (a few KiB; measure on your platform). It does not need the 50 KiB stack the pre-0.4 firmware used.

## HAL Contracts

All functions below must be implemented by the platform. Declarations are in `include/openblink/blink.h` and `include/openblink/vm.h`.

### Storage

| Function | Contract |
| --- | --- |
| `openblink_hal_storage_init()` | Mount/prepare non-volatile storage. Called once before any other storage call. |
| `openblink_hal_storage_read(slot, data, capacity, read_len)` | Read the stored bytecode. Return `OPENBLINK_STATUS_NOT_FOUND` if the slot is empty. |
| `openblink_hal_storage_write(slot, data, len, written_len)` | Persist the bytecode. `*written_len` must be set to the number of bytes written. |
| `openblink_hal_storage_get_length(slot, len)` | Return the stored length; `OPENBLINK_STATUS_NOT_FOUND` if empty. |
| `openblink_hal_storage_delete(slot)` | Delete the slot contents. |

Thread safety: storage functions are called from the VM thread (bytecode load) and from the transport context (`openblink_receive()` → store). The implementation **must** be safe against concurrent calls, e.g. by using an internal mutex.

### Transport / System

| Function | Contract |
| --- | --- |
| `openblink_hal_send_response(data, len)` | Deliver a protocol response to the host (e.g. GATT notify on the Program characteristic). May drop the data and return OK when no host is subscribed. |
| `openblink_hal_reboot()` | Reboot the device; not expected to return on success. |

### VM

| Function | Contract |
| --- | --- |
| `openblink_hal_vm_lock(timeout_ms)` | Acquire a recursive-unfriendly plain mutex within `timeout_ms` ms; return `true` on success. Must support acquisition from both the VM task context (Blink.lock) and the context calling `openblink_vm_restart()`. |
| `openblink_hal_vm_unlock()` | Release the mutex. |
| `openblink_hal_define_api()` | Register hardware Ruby classes (LED, Input, ...) with `mrbc_define_class`/`mrbc_define_method`. Called once per VM start. |
| `openblink_hal_load_default_bytecode(slot, data, capacity, len)` | Provide factory-default bytecode when storage is empty; return `OPENBLINK_STATUS_NOT_FOUND` if there is no default for the slot. |

### mruby/c HAL

The platform must also satisfy mruby/c's own HAL (see `mrubyc/hal/` for reference implementations):

| Function | Contract |
| --- | --- |
| `mrbc_hal_init()` | Called from `mrbc_init()`. **Owns starting the 1 ms tick timer** that calls `mrbc_tick()`. Do not start a second timer elsewhere — the pre-0.4 firmware's duplicated tick must not be reproduced. |
| `mrbc_hal_write(fd, buf, nbytes)` | Console output sink. `console_print`/`puts` from Ruby end up here; route it to your debug channel (e.g. GATT notify on the Console characteristic). |
| `mrbc_hal_enable_irq()` / `mrbc_hal_disable_irq()` | Scheduler critical sections. mruby/c uses these internally around task queue mutation (including `mrbc_create_task`/`mrbc_terminate_task`), so the core does not need platform-specific scheduler locks. |
| `mrbc_hal_idle_cpu()` | Sleep approximately one tick. Also used by the core when no task exists, to idle until a reload arrives. |
| `mrbc_hal_abort(s)` | Fatal error handler. |

## Concurrency Model

- `openblink_receive()` is **not re-entrant**: call it serially from a single context (e.g. the BLE receive thread or a work queue).
- `openblink_vm_restart()` may be called from the transport context; it synchronizes with the VM via `openblink_hal_vm_lock()` and mruby/c's internal critical sections. The platform mutex provides the cross-context exclusion between the VM thread (Blink.lock/unlock) and restart callers.
- Responses (`openblink_hal_send_response`) are emitted from within `openblink_receive()`, i.e. on the caller's context.

## Checklist

- [ ] `MRBC_SCHEDULER_EXIT=1` defined
- [ ] `MAX_VM_COUNT >= OPENBLINK_SLOT_COUNT`
- [ ] `hal.h` provider on the include path
- [ ] Tick timer started only by `mrbc_hal_init()`
- [ ] Storage HAL thread-safe
- [ ] `openblink_hal_storage_init()` called before `openblink_vm_main()`
- [ ] `openblink_receive()` called serially from one context
- [ ] Protocol responses routed per your transport binding (for BLE, see `doc/bluetooth_specification.md`)
