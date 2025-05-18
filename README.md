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

## How to Get OpenBlink

To clone the repository and initialize the submodules, run the following commands:

```console
$ git clone https://github.com/OpenBlink/openblink.git
$ git submodule init
$ git submodule update
```

## Documentation

For more detailed documentation, please check the [doc](./doc)

For AI-powered comprehensive documentation that helps you understand the codebase, visit [DeepWiki](https://deepwiki.com/OpenBlink/openblink).

## Verified Hardware

The following hardware platforms have been tested with OpenBlink:

- Nordic nRF54L15-DK (Board target: nrf54l15dk/nrf54l15/cpuapp)
- Nordic nRF52840-DK (Board target: nrf52840dk/nrf52840)

## Development Environment Versions

- nRF Connect SDK toolchain v2.9.0
- nRF Connect SDK v2.9.0
