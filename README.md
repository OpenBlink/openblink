# OpenBlink

[![English](https://img.shields.io/badge/language-English-blue.svg)](README.md)
[![中文](https://img.shields.io/badge/language-中文-red.svg)](README.zh-CN.md)
[![日本語](https://img.shields.io/badge/language-日本語-green.svg)](README.ja.md)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/OpenBlink/openblink)

## What is OpenBlink

**_OpenBlink_** is an open source project forked from **_ViXion Blink_**.

- Ruby, a highly productive lightweight language, can be used to develop embedded devices.
- Program rewriting and debugging console are completely wireless. (BluetoothLE)
- Rewriting time is less than 0.1 second and does not involve a microprocessor restart. (We call it "Blink".)

**Key ideas:**

- **Instant rewriting** — Ruby code changes are reflected on the real device in < 0.1 sec, without microcontroller restart
- **Fully wireless** — All program transfer and debug console output run over Bluetooth LE — no cables needed
- **Ruby for embedded** — Use [mruby/c](https://github.com/mrubyc/mrubyc), a lightweight Ruby VM, to develop for microcontrollers with high productivity and readability
- **For everyone** — Not just for embedded engineers. Designed for system designers, mechanical engineers, hobbyists, students, and end users who want to customize their own devices ("DIY-able value")

## OpenBlink Ecosystem

OpenBlink is developed as a family of repositories that work together:

- **[openblink](https://github.com/OpenBlink/openblink)** — Core firmware (this repository)
- **[openblink-vscode-extension](https://github.com/OpenBlink/openblink-vscode-extension)** — VSCode extension for Ruby code editing and wireless Blink via BLE
- **[openblink-webide](https://github.com/OpenBlink/openblink-webide)** — Browser-based Web IDE for OpenBlink

## Philosophy & Goals

OpenBlink is driven by the belief that **embedded programming should be accessible to everyone** — not just specialized software engineers.

### A Programmable World

Microcontroller-powered embedded devices are truly **ubiquitous** — woven into every corner of the physical world, from home appliances and wearables to industrial equipment and vehicles — and they directly influence how the real world behaves. Yet the firmware that runs on these devices has traditionally been the exclusive domain of the manufacturer; end users have had no way to modify it.

OpenBlink challenges this status quo. On an OpenBlink-enabled device, **end users themselves can rewrite part of the firmware** — safely and wirelessly — unlocking a future where everyday embedded devices become programmable. Our goal is to realize this vision of a truly _programmable world_.

And the rewrite is fast: it takes no longer than the blink of an eye — that is where the name "Blink" comes from.

### Build & Blink

_Build_ something with your own hands. _Blink_ it into reality.

Build & Blink embodies the spirit of DIY — the simple joy of creating something yourself and watching it come alive on a real device, right in front of you. You do not need to be a professional engineer or ask a manufacturer for permission. If you have an idea, you can try it now, on your own terms, with your own code.

And this is not a toy: the same Build & Blink workflow scales seamlessly into professional development. Engineers use it to tune real products, run factory tests, and iterate on production firmware — all with the same tools and the same instant feedback. OpenBlink exists to put that power in your hands, from your first experiment to your shipping product.

### Thinking Speed Prototyping

When you edit Ruby code and save, the running device reflects the change in under 0.1 seconds. The microcontroller does **not** restart: only the target task is reloaded while everything else — including the BLE connection and debug console — keeps running. This tight feedback loop lets you iterate at the speed of thought.

Conventional approaches to device customization rely on configuration parameters that the firmware developer must define in advance. This creates a dilemma: too few settings and users cannot express what they need; too many and the interface becomes overwhelming and impossible to master. Worse, no pre-designed set of options can ever anticipate every use case.

OpenBlink sidesteps this problem entirely by treating Ruby as a domain-specific language for device behavior. Instead of choosing from a fixed menu, you write code — which means you can **rewrite the logic itself**: replace a state machine, redesign a control algorithm, or completely rethink how the device responds to its inputs, all in a single save. This expressiveness is what sets OpenBlink apart from conventional parameter-tuning approaches.

**Why Ruby?** Ruby was designed from the ground up to make programmers happy — its syntax reads like natural language, and its flexible grammar (optional parentheses, blocks, keyword arguments) makes it one of the best languages for writing clean, self-documenting domain-specific code. This means even someone who has never touched embedded development can read `LED.set(part: :led1, state: true)` and understand what it does. At the same time, [mruby/c](https://github.com/mrubyc/mrubyc) — a lightweight Ruby VM built for microcontrollers — makes it possible to run Ruby on devices with as little as 15 KB of heap memory, and the mruby compiler produces compact bytecode that fits comfortably within a single BLE transfer. Ruby gives OpenBlink the rare combination of **human friendliness and microcontroller fitness**.

### Layered Task Architecture

OpenBlink firmware separates embedded software into three layers:

| Layer                                         | Language | Wirelessly rewritable? |
| --------------------------------------------- | -------- | ---------------------- |
| **Critical tasks** (drivers, BLE stack, RTOS) | C        | No                     |
| **UX tasks** (device behavior, LED patterns)  | Ruby     | Yes                    |
| **DIY tasks** (end-user programs)             | Ruby     | Yes                    |

Only the Ruby layers execute on the [mruby/c](https://github.com/mrubyc/mrubyc) VM. The C layer remains untouched during a Blink, keeping the system stable and the wireless connection alive.

### For Everyone

OpenBlink is designed so that **system designers, mechanical engineers, hobbyists, students, and end users** — not only embedded software engineers — can modify real device behavior.

**For engineers and designers:**

- Tune sensor thresholds, control sequences, and LED feedback patterns on a live product — no rebuild/flash/reboot cycle
- Write factory inspection programs that run alongside production firmware, without embedding test logic into the shipping codebase
- Let mechanical engineers adjust motor timing or haptic feedback on the actual device while it is assembled, rather than going back and forth with the software team
- Achieve **perfect sensory tuning** — in conventional development, dialing in control parameters requires long iteration cycles, but with OpenBlink the device reflects every adjustment instantly, enabling engineers to tune by feel and intuition until the result is exactly right
- Move beyond **one-size-fits-all** products — traditional mass manufacturing targets the statistical average (e.g. the 3σ range), but OpenBlink makes it practical to tailor device behavior to individual users, opening the door to truly personalized products that adapt to each person rather than forcing everyone into the same mold

**For hobbyists and students:**

- Learn embedded programming in Ruby with instant visual feedback — change a line, save, and see the LED pattern change immediately
- Experiment with I2C sensors and actuators interactively, trying different command sequences without recompiling C code
- Build and share creative projects — the wireless connection means you can program a wearable while wearing it
- **Vibe Coding on real hardware** — pair OpenBlink with an AI coding assistant and describe what you want in natural language; the AI writes the Ruby code, you save, and the device does it. The instant feedback loop lets you and the AI iterate together in real time on a physical device, not just a simulator

**For end users ("DIY-able value"):**

- Customize the behavior of a device you own — personalize shortcuts, automate routines, or adapt the product to your specific needs
- Run your own programs on a commercially manufactured device, turning a finished product into a personal platform

Permission-level API restrictions ensure that openness and stability coexist: devices expose only the methods appropriate for each trust level, so manufacturers can safely open their products to user programming.

### From Education to Production

A single ecosystem covers **learning → hobby → production**. The same toolchain, language, and workflow apply whether you are blinking an LED for the first time or shipping a product.

### Happy Hacking

OpenBlink places great importance on the **joy of hacking on real hardware**. Every design decision serves this goal:

- **No fatal mistakes** — A buggy Ruby program cannot brick the device; the C-level firmware and BLE stack keep running, so you can always send a fix. This safety net encourages you to experiment boldly and keep trying — OpenBlink is built for people who learn by doing.
- **Instant feedback** — Changes take effect in under 0.1 seconds, keeping you in a tight edit-test loop without breaking your flow.
- **No restart** — The microcontroller stays running, which means your wireless debug console session is never interrupted.
- **Fully wireless** — Develop on a wearable while you are wearing it, or on a sensor mounted in a hard-to-reach spot — no cables to tether you.

These qualities come together to create a **happy hacking experience** for everyone who builds and blinks.

## How to Get OpenBlink

To clone the repository and initialize the submodules, run the following commands:

```console
$ git clone https://github.com/OpenBlink/openblink.git
$ git submodule init
$ git submodule update
```

## Documentation

For more detailed documentation, please check the [doc](./doc)

For AI-powered comprehensive documentation that helps you understand the codebase, visit [DeepWiki](https://deepwiki.com/OpenBlink/openblink)

## Verified Hardware

The following hardware platforms have been tested with OpenBlink:

- Nordic nRF54L15-DK (Board target: nrf54l15dk/nrf54l15/cpuapp)
- Nordic nRF52840-DK (Board target: nrf52840dk/nrf52840)

## Development Environment Versions

- nRF Connect SDK toolchain v3.2.1
- nRF Connect SDK v3.2.1
