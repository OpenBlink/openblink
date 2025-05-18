# OpenBlink

[![English](https://img.shields.io/badge/language-English-blue.svg)](README.md)
[![中文](https://img.shields.io/badge/language-中文-red.svg)](README.zh-CN.md)
[![日本語](https://img.shields.io/badge/language-日本語-green.svg)](README.ja.md)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/OpenBlink/openblink)

## OpenBlink 是什么

**_OpenBlink_** 是一个从 **_ViXion Blink_** 分叉的开源项目。

- 可以使用高生产力的轻量级语言 Ruby 来开发嵌入式设备。
- 程序重写和调试控制台完全无线。（蓝牙低功耗）
- 重写时间少于 0.1 秒，不需要微处理器重启。（我们称之为"Blink"）

## 如何获取 OpenBlink

要克隆仓库并初始化子模块，请运行以下命令：

```console
$ git clone https://github.com/OpenBlink/openblink.git
$ git submodule init
$ git submodule update
```

## 文档

有关更详细的文档，请查看[doc](./doc)

如需查阅AI驱动的全面文档以帮助您理解代码库，请访问[DeepWiki](https://deepwiki.com/OpenBlink/openblink)

## 已验证的硬件

以下硬件平台已通过 OpenBlink 测试：

- Nordic nRF54L15-DK (Board target: nrf54l15dk/nrf54l15/cpuapp)
- Nordic nRF52840-DK (Board target: nrf52840dk/nrf52840)

## 开发环境版本

- nRF Connect SDK toolchain v2.9.0
- nRF Connect SDK v2.9.0
