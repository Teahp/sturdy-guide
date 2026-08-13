# Sturdy Guide

[![CI](https://github.com/Teahp/sturdy-guide/actions/workflows/ci.yml/badge.svg)](https://github.com/Teahp/sturdy-guide/actions/workflows/ci.yml)

一个用于 Linux、C++ 工具链、CMake、Ninja、SDK、CI、Git 协作以及
面向对象与并发设计教学的多文件示例项目。

## 构建关系

```text
源文件 + CMakeLists.txt
          |
          v
        CMake 选择 G++ 或 Clang++，生成 build.ninja
          |
          v
        Ninja 判断需要执行的任务
          |
          v
     G++ / Clang++ 实际编译与链接
```

G++ 与 Clang++ 是两套可替换的 C++ 编译器驱动。Ninja 不是编译器，它只执行 CMake 生成的构建规则。

## 依赖

Ubuntu / WSL2：

```bash
sudo apt update
sudo apt install -y build-essential clang cmake ninja-build git
```

## 使用 G++ 构建

```bash
cmake -S . -B build-gcc -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build-gcc
ctest --test-dir build-gcc --output-on-failure
./build-gcc/sturdy-guide Linux
./build-gcc/sturdy-guide-midi
```

## 使用 Clang++ 构建

使用单独的构建目录，不要在已经配置的目录中直接更换编译器：

```bash
cmake -S . -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang
ctest --test-dir build-clang --output-on-failure
./build-clang/sturdy-guide Clang
```

## 项目结构

```text
sturdy-guide/
├── .github/workflows/ci.yml       GitHub Actions CI
├── app/
│   ├── main.cpp                   命令行程序
│   └── CMakeLists.txt             声明 CLI target
├── cmake/                         SDK 的 CMake 包配置
├── docs/SDK.md                    SDK 概念与使用方法
├── examples/consumer/             独立的 SDK 消费项目
├── include/sturdy_guide/          公共 API
├── programs/
│   ├── midi_player/               完整的 OOP 与并发示例
│   └── camera_monitor/            单线程起点与摄像头重构作业
├── src/
│   ├── *.cpp                      库实现
│   └── CMakeLists.txt             声明库 target
├── tests/
│   ├── *_test.cpp                 CTest 测试程序
│   └── CMakeLists.txt             声明并注册测试 target
└── CMakeLists.txt                 全局配置与 add_subdirectory
```

根 `CMakeLists.txt` 负责项目级设置并通过 `add_subdirectory()` 进入各个模块；每个子目录负责自己的 target。`sturdy_guide` 是 SDK 库 target，`sturdy_guide_cli` 是原有应用 target。课程程序位于 `programs/`，可以通过 `STURDY_GUIDE_BUILD_LESSON_PROGRAMS` 选项整体启用或关闭。

`examples/consumer/` 自带 `project()`，故意不加入根项目：它模拟另一个工程只使用已经安装的 SDK。

## 面向对象与并发课程程序

### MIDI 播放器示例

[`programs/midi_player/`](programs/midi_player/) 是完整实现。它将领域模型、运行时多态接口、设备后端、后台播放线程、CLI 和测试拆成独立工程边界：

- 默认把 MIDI 事件打印到终端，不要求安装音频 SDK；
- Linux 上可以通过 `--device /dev/snd/midiC1D0` 写入原始 MIDI 设备；
- 播放器使用条件变量实现可中断定时等待；
- 测试通过假输出后端观察消息，不依赖 MIDI 硬件。

详细结构与运行方式见 [MIDI 示例说明](programs/midi_player/README.md)。

### 摄像头回家作业

[`programs/camera_monitor/ASSIGNMENT.md`](programs/camera_monitor/ASSIGNMENT.md) 提供一个使用 OpenCV 调用笔记本内置摄像头或 UVC USB 摄像头的单线程程序。它功能可用，但所有职责都集中在 `main.cpp`。

学生需要在保持外部行为的前提下，将其重构为设备抽象、后台采集会话和主线程应用，并加入有界帧缓冲与硬件无关测试。目标多线程版本不提供实现。

## 作为 SDK 安装和使用

这里的“安装”不是系统软件商店操作，而是把公共头文件、编译后的库和 `find_package` 所需配置复制到指定前缀。consumer 因而不需要访问本项目的 `src/`。

```bash
cmake --install build-gcc --prefix install

cmake -S examples/consumer -B consumer-build -G Ninja \
  -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build consumer-build
./consumer-build/sturdy_guide_consumer
```

详细说明见 [docs/SDK.md](docs/SDK.md)。

## CI

`.github/workflows/ci.yml` 会在推送到 `main` 或发起 PR 时执行：

1. 分别选择 G++ 和 Clang++ 配置 Ninja 构建目录；
2. 构建库、应用和测试；
3. 运行 CTest；
4. 安装 SDK；
5. 从独立工程通过 `find_package(SturdyGuide)` 构建并运行消费者。

CI 在干净环境中重复本地构建和测试，是 PR 合并前的自动质量检查，但不能替代代码评审。

## 参与协作

本项目采用 Fork → 功能分支 → PR → Review → Merge 的流程。完整命令和规范见 [CONTRIBUTING.md](CONTRIBUTING.md)。
