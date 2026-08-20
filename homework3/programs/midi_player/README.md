# MIDI 播放器示例

这个程序是“面向对象与并发工程设计”课程的完整示例。它生成一段 C 大调 MIDI 音阶，并按照事件时间在后台线程发送消息。

MIDI 消息只描述音符、通道和力度，并不包含声音波形。默认的终端后端只显示消息；要听到声音，需要把消息发送给 MIDI 硬件或软件合成器。

## 运行

从仓库根目录构建：

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/sturdy-guide-midi
```

默认输出类似：

```text
NOTE ON  channel=0 note=60 velocity=96  [ 0x90 0x3c 0x60 ]
NOTE OFF channel=0 note=60 velocity=0   [ 0x80 0x3c 0x00 ]
```

默认模式不访问硬件，因此 Windows 和 Linux 上都能运行。要真正听到声音，先列出设备，再选择一个输出。

### Windows

WinMM 是 Windows SDK 自带的多媒体 API，不需要额外安装 MIDI 开发库。在 Visual Studio 的 Developer PowerShell 中构建：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\sturdy-guide-midi.exe --list-devices
.\build\Release\sturdy-guide-midi.exe --device 0
```

`--list-devices` 会输出设备编号和名称，例如 `0: Microsoft GS Wavetable Synth`。请使用本机实际列出的编号；没有输出设备时，程序仍可使用默认终端模式演示。

### Linux

若系统已有可写的 raw MIDI 设备，可以指定设备路径：

```bash
./build/sturdy-guide-midi --list-devices
./build/sturdy-guide-midi --device /dev/snd/midiC1D0
```

程序会在 `/dev/snd/` 下查找名称以 `midiC` 开头的 raw MIDI 设备。设备路径、权限和硬件或软件合成器连接由本机环境决定；WSLg 的音频转发本身不会创建 MIDI 端口。

## 工程结构

```text
midi_player/
├── include/midi/             公共接口与领域模型
├── src/                      播放器及输出后端实现
├── app/                      依赖组装和命令行入口
├── tests/                    使用假后端的确定性测试
└── CMakeLists.txt            核心库及子目录编排
```

主要设计关系：

```text
MidiSequence  --按时间拥有--> MidiEvent --拥有--> MidiMessage
                                      |
MidiPlayer --借用--> MidiOutput <-----+-- ConsoleMidiOutput
                                      +-- RawMidiOutput
                                      +-- WinMmMidiOutput
                                      +-- RecordingOutput（测试）
```

- `MidiMessage` 的工厂函数维护通道、音符和力度范围；
- `MidiSequence` 维护事件按时间排序的不变量；
- `MidiOutput` 是运行时多态接口，隔离调度逻辑与设备访问；
- CMake 在 Linux 编译 `RawMidiOutput`，在 Windows 编译 `WinMmMidiOutput` 并链接系统库 `winmm`；
- `MidiPlayer` 管理一个工作线程，析构时执行 `stop()` 和 `join()`；
- 条件变量让 `stop()` 能中断长时间等待；
- 调用外部 `MidiOutput::send()` 时不持有播放器 mutex；
- 后台线程的设备异常通过 `std::exception_ptr` 在 `wait()` 中重新抛出；
- 测试使用 `RecordingOutput`，无需真实 MIDI 设备，也无需等待完整乐曲。

示例为突出工程边界，没有实现 Standard MIDI File（`.mid`）解析、精确音频时钟、暂停/恢复和多轨同步。这些能力应优先使用成熟 MIDI 库，而不是继续扩大课堂示例。
