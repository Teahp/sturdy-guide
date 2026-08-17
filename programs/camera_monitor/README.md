# 摄像头回家作业：多线程重构

把单线程起始程序 `starter/main.cpp` 重构为「设备抽象 + 后台采集会话 + 主线程预览应用」。
外部行为保持不变：参数、按键、叠加层和截图逻辑与原程序一致。

```text
sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]
S 保存截图（captures/capture-N.png），Q 或关闭窗口退出
```

## 目录结构

```text
camera_monitor/
├── starter/main.cpp                原始单线程版本（保留对照，默认不构建）
├── include/camera/                 公共接口
│   ├── frame_source.hpp            FrameSource 抽象设备接口
│   ├── opencv_camera.hpp           OpenCvCamera：cv::VideoCapture 真实设备
│   ├── fake_frame_source.hpp       FakeFrameSource：测试替身
│   ├── camera_session.hpp          CameraSession：后台采集会话
│   └── preview_application.hpp     PreviewApplication：主线程预览应用
├── src/                            库实现（camera_core）
├── app/main.cpp                    参数解析、依赖组装
├── tests/                          硬件无关测试（CTest）
├── CMakeLists.txt                  查找依赖并编排子目录
└── README.md                       本文档
```

## 构建与运行

```bash
# 从仓库根目录启用摄像头作业
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
ctest --test-dir build-camera --output-on-failure
./build-camera/sturdy-guide-camera --device 0
```

如需与起始版本对照，可额外开启 `-DSTURDY_GUIDE_BUILD_CAMERA_STARTER=ON`，
起始版本会以 `sturdy-guide-camera-starter` 输出，与正式 target 不冲突。
正式 target `sturdy-guide-camera` 只编译 `app/main.cpp`，不包含 `starter/`。

## 重构前后职责对比

| 职责 | 起始版本 | 重构后 |
| --- | --- | --- |
| 参数解析 | `main()` 内联 | `app/main.cpp` |
| 设备访问 | `main()` 直接操作 `cv::VideoCapture` | `OpenCvCamera`（实现 `FrameSource`） |
| 帧率统计 | `main()` 局部变量 | `PreviewApplication`（按显示帧统计） |
| 窗口与按键 | `main()` 循环 | `PreviewApplication::run()` |
| 截图 | `main()` 内联 | `PreviewApplication::saveScreenshot()` |
| 采集线程与生命周期 | 无（单线程） | `CameraSession`（start/stop/join 状态机） |
| 帧缓冲 | 无（直接显示） | `CameraSession` 有界环形缓冲（容量 2） |
| 错误处理 | 直接 throw 到 main | 同步错误直接抛出；后台异常经 `error()` 传回 |

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得足够久？

- `CameraSession` 通过 `std::unique_ptr<FrameSource>` 独占设备；`CameraSession`
  析构先 `stop()`（join 工作线程）再释放成员，因此设备生命周期严格包含工作线程。
- `PreviewApplication` 独占 `CameraSession`（`unique_ptr`），主循环期间会话一直存活。
- 工作线程只访问会话的成员，不访问应用层对象；主线程只在 `waitForFrame()`
  返回后使用取出的 `cv::Mat`（引用计数句柄，取走后与缓冲解耦）。

### 2. `CameraSession` 有哪些状态，哪些转换是合法的？

```text
Idle --start()--> Running        （open() 失败则保持 Idle 并同步抛出）
Running --stop()--> Stopping --> Stopped
Running --后台失败--> Stopping --> Stopped（error() 非空）
Running --start()--> 无操作（幂等）
Stopping --start()--> 无操作（幂等；join 之后才能重新 start）
Stopped --start()--> Running     （清空上次的错误与统计）
Stopped / Idle --stop()--> 无操作（幂等）
```

`running()` 表示处于 `Running`；`Stopped` 且 `error()` 非空表示上次运行失败。

### 3. 哪些公共方法允许被不同线程同时调用？

- 任意线程：`tryTakeFrame()`、`waitForFrame()`、`running()`、`error()`、
  `capturedFrames()`、`droppedFrames()`。
- `stop()` 允许重复调用，也允许并发调用（内部 `lifecycle_mutex_` 保证
  `join()` 恰好执行一次），但不应与 `start()` 并发。
- `start()`/`stop()` 的正常用法是同一调用方（主线程）顺序调用；
  `PreviewApplication` 即如此。

### 4. mutex 保护哪些成员以及什么不变量？

`mutex_` 保护：状态、停止标志、错误串、环形缓冲（`frames_`/`head_`/`count_`）、
序号与统计。不变量：

- `0 <= count_ <= 2`，缓冲永远不超两帧；
- `frames_` 中 `[head_, head_+count_)`（环形）按从旧到新排列；
- `captured_ == 最新序号`，`dropped_ == 因满被丢弃的帧数`；
- 帧序号严格递增（同一运行内）。

`lifecycle_mutex_` 只用于串行化 `stop()`/析构，保证并发停止时只有一个线程 join。

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

生产者（工作线程）写入时若 `count_ == 2`，丢弃**最旧**的一帧，把新帧作为最新帧
写入。消费者取走**最新**帧并清空缓冲。原因：实时预览只关心最新画面，旧的未消费
帧已经过时；丢最旧帧保证缓冲不增长、延迟最小。`droppedFrames()` 记录被丢弃的帧数。

### 6. 为什么不能持锁调用 `VideoCapture::read()`、`imshow()` 或 `imwrite()`？

这些都是慢速 I/O：摄像头读取可能阻塞数毫秒到数百毫秒，`imshow`/`imwrite` 涉及
窗口系统与磁盘。持锁执行会长时间阻塞所有取帧者（包括 UI 线程），等于把并发程序
退化成串行，还可能引入与高层的意外交互。因此工作线程在锁外调用 `read()`，
主线程在锁外调用 `imshow()`/`imwrite()`，锁只保护内存状态的完整转换。

### 7. 后台异常通过什么机制越过线程边界？

工作线程用 `try/catch` 捕获 `read()` 及循环内的异常（同时把“返回 false”
和“空帧”归一为错误消息），把消息存入 `mutex_` 保护下的 `error_`（保留首个），
置 `stop_requested_` 并唤醒条件变量，然后退出。调用线程通过轮询
`error()`（或在 `waitForFrame()` 返回 false 后检查）观察错误并报告。
异常对象本身不跨线程，跨线程的是它的消息，避免悬垂引用。

### 8. 析构时执行 `stop()`、唤醒和 `join()` 的顺序是什么？

析构函数只调用 `stop()`。`stop()` 的顺序：

1. 持 `mutex_`：状态置 `Stopping`、置 `stop_requested_`；
2. 释放 `mutex_` 后 `notify_all()`，唤醒阻塞在 `waitForFrame()` 的消费者；
3. `join()` 工作线程（在 `lifecycle_mutex_` 内，保证只 join 一次）；
4. 持 `mutex_`：状态置 `Stopped`。

工作线程看到停止标志后从 `read()` 返回并退出（若 `read()` 正阻塞，join 会等待
该次读取返回）。join 完成后才析构成员（含 `source_`），保证设备先于线程销毁。

## 有界缓冲与丢帧验证

测试 `tests/camera_session_test.cpp` 通过 `FakeFrameSource` 验证（不打开真实设备）：

1. `start()` 后能取得带递增序号的帧；
2. 重复 `start()`：已运行时是幂等 no-op（不会再次 open），停止后可重启且统计清零；
3. 连续/并发 `stop()` 不会崩溃或死锁；
4. 析构前工作线程已被 join（共享标志确认假设备在线程停止后才被销毁）；
5. 突发 5 帧后缓冲不超过两帧，消费方只取到最新一帧（序号 5），丢弃最旧 3 帧；
6. 假设备抛出的读取错误能在主线程观察（错误消息保留）；
7. `read()` 返回 false 与空帧都不会被发布为有效画面；
8. 额外：`open()` 失败时 `start()` 同步抛出且不创建线程。

```bash
ctest --test-dir build-camera --output-on-failure
```

## 真机集成测试记录

自动测试不依赖摄像头；真机验证请在原生 Linux / 课程机器上完成并补全下表与截图：

| 项目 | 记录 |
| --- | --- |
| 设备型号 | （如：笔记本内置摄像头 / Logitech C270） |
| 操作系统 | （如：Ubuntu 22.04） |
| 分辨率 | 1280x720 |
| 启动命令 | `./build-camera/sturdy-guide-camera --device 0` |
| 结果 | （正常显示 / 报错内容） |
| 运行截图 | （将 `S` 保存的 `captures/capture-1.png` 或窗口截图附到 PR） |
