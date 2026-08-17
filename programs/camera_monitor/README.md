# 摄像头监控程序（重构版）

> 这是重构后的最终实现，已替换原 `camera_monitor/` 作业目录。
> `starter/main.cpp` 保留原始单线程版本作对照，**不参与构建**。

把单线程的 `starter/main.cpp` 重构为封装良好、可测试的多线程程序。
外部行为保持不变：

```text
sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]
```

- 后台线程持续读取摄像头；主线程只负责窗口、键盘、画面叠加和截图；
- 内存中最多保留两帧，显示落后时丢弃旧帧；
- `Q`、窗口关闭、读帧失败、对象析构都能停止并 `join()` 工作线程；
- 后台读取异常能被主线程观察并报告；
- 核心逻辑编译为库，应用与测试分别链接该库，测试不依赖真实摄像头。

## 构建与运行

从仓库根目录单独启用摄像头作业：

```bash
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
ctest --test-dir build-camera --output-on-failure
./build-camera/sturdy-guide-camera --device 0
```

按键：`S` 保存截图到 `captures/`，`Q` 或关闭窗口退出。
设备不存在、被占用或权限不足时会打印提示并以非零退出码结束。

## 工程结构

```text
camera_monitor/
├── starter/main.cpp       原始单线程版本（保留对照，不参与构建）
├── include/camera/        公共接口
│   ├── frame_source.hpp           FrameSource 抽象接口
│   ├── opencv_camera.hpp          OpenCvCamera 真实设备后端
│   ├── fake_frame_source.hpp      FakeFrameSource 测试用假设备
│   ├── camera_session.hpp         CameraSession 采集会话
│   └── preview_application.hpp    PreviewApplication 主线程应用
├── src/                   设备与采集会话实现
│   ├── opencv_camera.cpp
│   ├── fake_frame_source.cpp
│   └── camera_session.cpp
├── app/
│   ├── main.cpp                   参数解析、依赖组装
│   └── preview_application.cpp    窗口循环、叠加、截图
├── tests/
│   └── camera_session_test.cpp    硬件无关测试
├── CMakeLists.txt         独立工程配置（也可合并回 camera_monitor/）
└── README.md
```

构建关系：

```text
FrameSource (抽象接口)
├── OpenCvCamera        使用 cv::VideoCapture 访问真实设备
└── FakeFrameSource     测试时产生确定的假帧或模拟失败

CameraSession ──拥有(unique_ptr)──> FrameSource
       │
       │  start / stop / join 状态机
       │  有界最新帧缓冲（最多 2 帧）
       │  后台异常通过 exception_ptr 回传
       ▼
PreviewApplication（主线程）──调用──> CameraSession 公开方法
```

`CameraSession` 通过 `std::unique_ptr<FrameSource>` 独占设备。真实设备与假设备
遵循同一接口，业务代码中不存在"测试模式"分支。

## 重构前后的职责对比

| 职责 | 起始版本（starter/main.cpp） | 重构后 |
|---|---|---|
| 参数解析 | 堆在 `main()` | `app/main.cpp` 独立负责 |
| 设备访问 | `main()` 直接持有 `cv::VideoCapture` | `OpenCvCamera` 实现 `FrameSource` 接口 |
| 后台采集 | 无，采集阻塞 UI | `CameraSession` 后台线程持续读帧 |
| 帧缓冲 | 无，一帧接一帧 | 有界缓冲（最多 2 帧），满时丢最旧帧 |
| 帧率统计 / 叠加 | `main()` 内联 | `PreviewApplication` 主线程完成 |
| 窗口 / 键盘 / 截图 | `main()` 内联 | `PreviewApplication` 主线程完成 |
| 错误处理 | 异常直接抛出 | worker 捕获，`exception_ptr` 回传主线程观察 |
| 测试 | 依赖真实硬件 | `FakeFrameSource` 提供确定性假帧，硬件无关 |

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得足够久？

- `FrameSource`：由 `CameraSession` 通过 `std::unique_ptr` 独占持有；
  构造函数接收所有权，析构时随会话销毁。会话保证其生命周期覆盖工作线程全程。
- `CameraSession`：拥有工作线程 `std::thread`、`std::mutex`、条件变量和有界帧缓冲。
  析构先调用 `stop()`（内部 `join()`），保证线程先于对象销毁。
- `OpenCvCamera`：拥有 `cv::VideoCapture`。由会话的工作线程调用
  `open()/read()/close()`，线程退出前调用 `close()` 释放设备。
- `PreviewApplication`：只持有 `CameraSession&` 引用，不拥有它。
  `app/main.cpp` 保证 `session` 的生命周期覆盖 `app.run()` 全程。

### 2. CameraSession 有哪些状态，哪些转换是合法的？

```text
Idle -> Running -> Stopping -> Stopped -> Idle
```

- `Idle`：未启动，或已 join。`start()` 从这里启动，返回 true。
- `Running`：工作线程正在采集。重复 `start()` 返回 false（非法转换被拒绝）。
- `Stopping`：已请求停止。worker 读到该状态即退出。
- `Stopped`：worker 已退出但尚未 join（例如读帧失败自行退出），等待 `stop()` 完成 join。
- 合法转换：`Idle→Running`（start）、`Running→Stopping`（stop）、
  `Running→Stopped`（worker 出错）、`Stopping→Stopped`（worker 正常退出）、
  `Stopped→Idle`（stop 完成 join）。`Idle→Running` 允许停止后重启。

### 3. 哪些公共方法允许被不同线程同时调用？

- `stop()`：可被多个线程并发调用。内部用 mutex 保证只发生一次 join，
  后续调用立即返回（幂等）。
- `wait_for_frame()`、`has_error()`、`error_message()` 和统计方法：
  可由消费者线程与停止流程并发调用，全部内部加锁。
- `start()` 与 `stop()`/析构约定由同一所有者线程串行调用；
  若并发调用，`stop()` 可能先于 `start()` 完成（状态仍为 Idle），
  因此本程序由主线程串行管理会话生命周期。

### 4. mutex 保护哪些成员以及什么不变量？

`mutex_` 保护：`state_`、帧缓冲 `frames_`、`frames_produced_`、
`dropped_frames_`、`background_error_`、`error_reported_`。

不变量：

- `frames_.size() <= kMaxBufferedFrames`（2）；
- `frames_produced_ == 已消费帧数 + dropped_frames_ + buffered_frames()`；
- 状态只在上述合法转换之间移动。

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

丢弃**最旧帧**（`pop_front`），保留最新帧。显示端需要尽可能新的画面，
过时的帧价值最低；固定保留 2 帧既能避免无限增长，又给显示端留出缓冲。

### 6. 为什么不能持锁调用 VideoCapture::read()、imshow() 或 imwrite()？

这些是慢速 I/O：摄像头读取可能阻塞几十毫秒，图片写入更慢。
持锁执行会让其他线程（如 UI 线程的 `wait_for_frame`）长时间阻塞，
导致界面卡死并放大锁竞争。正确做法是慢速 I/O 全部放在临界区之外，
锁只保护共享状态的短暂转换。

### 7. 后台异常通过什么机制越过线程边界？

worker 捕获异常后用 `std::exception_ptr`（`std::current_exception()`）保存，
置 `error_reported_` 并 notify。主线程通过 `has_error()` / `error_message()`
观察；`error_message()` 用 `std::rethrow_exception` 提取 `what()`。
`exception_ptr` 管理异常对象的生命周期，跨线程安全。

### 8. 析构时执行 stop()、唤醒和 join() 的顺序是什么？

`~CameraSession()` 调用 `stop()`：

1. 锁内把状态置为 `Stopping` 并 `notify_all()`，唤醒等待帧的消费者；
2. 释放锁，在**锁外** `join()` 工作线程（持锁 join 会与 worker 抢锁形成死锁）；
3. 重新加锁置 `Idle` 并再次 notify，等待者据此返回 false。

worker 侧退出顺序：发现 `Stopping` 或错误 → 跳出采集循环 →
`source_->close()` → 标记 `Stopped` → notify → 线程结束，join 返回。

## 自动测试

`tests/camera_session_test.cpp` 使用 `FakeFrameSource`，不打开任何真实设备。
覆盖内容：

1. `start()` 后能取得带递增序号的帧；
2. 重复 `start()` 符合契约（运行中拒绝，停止后可重启）；
3. 连续调用 `stop()` 不会崩溃或死锁；
4. 对象析构前工作线程已被 `join()`（析构不崩溃）；
5. 消费较慢时缓冲不超过两帧，并按约定丢帧；
6. 假设备抛出的读取错误能在主线程被观察；
7. 空帧不会被发布为有效画面，并作为错误上报；
8. （额外）`read()` 返回 false 的读失败同样被观察并停止。

## 真机测试记录

> 待补充：在装有摄像头的原生 Linux 机器上运行
> `./build-camera/sturdy-guide-camera --device 0`，并记录：
> 设备型号、操作系统、分辨率、`S` 保存 / `Q` 退出是否正常，并附运行截图。
