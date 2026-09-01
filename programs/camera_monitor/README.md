# 摄像头并发重构作业（camera_monitor）

把单线程摄像头程序重构为"后台采集 + 主线程 UI"的并发组件，保持外部行为不变：

```text
sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]
```

按键行为保持不变：`S` 保存截图到 `captures/`，`Q` 或关闭窗口退出。

## 目录结构

```text
camera_monitor/
├── starter/main.cpp       原始单线程版本（保留对照，不参与构建）
├── include/camera/        公共接口（FrameSource、CameraSession 等声明）
├── src/                   实现（设备、假设备、采集会话）
├── app/main.cpp           参数解析、依赖组装和窗口循环
├── tests/                 不依赖真实摄像头的自动测试
├── CMakeLists.txt         查找依赖并编排子目录
└── README.md              本文档
```

## 构建与运行

```bash
cmake -S . -B build-camera -G Ninja -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
./build-camera/sturdy-guide-camera --device 0
```

## 自动测试

测试全部基于 `FakeFrameSource`，不依赖真实摄像头：

```bash
ctest --test-dir build-camera --output-on-failure
```

本地结果：`100% tests passed, 0 tests failed out of 5`（含 `camera_session_test` 七条验证：递增帧号、重复 start、连续 stop、析构 join、丢帧策略、错误传递、空帧拒绝）。

## 重构前后的职责对比

**重构前（starter/main.cpp，单线程）**：`main()` 一个人完成参数解析、打开设备、读帧、FPS 统计、画字、显示、按键、存图、错误处理；读帧阻塞 UI；依赖真实硬件，无法稳定测试；没有采集线程生命周期。

**重构后**：

| 职责 | 归属 |
| --- | --- |
| "给我一帧"的约定 | `FrameSource`（抽象接口） |
| 真摄像头 | `OpenCvCamera`（`cv::VideoCapture`） |
| 测试用假摄像头 | `FakeFrameSource`（确定性帧 / 可模拟失败） |
| 后台采集线程、两帧缓冲、启停、异常传递 | `CameraSession` |
| 窗口、键盘、画面叠加、截图 | `app/main.cpp`（主线程 UI） |
| 硬件无关验证 | `tests/`（CTest） |

核心逻辑编译为 `camera_core` 库；应用与测试分别链接该库；依赖使用 `find_package(OpenCV)` 与 `find_package(Threads)`，未写死库路径。

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得足够久？

- `CameraSession` 通过 `std::unique_ptr<FrameSource>` **独占**摄像头（真或假）；session 析构时自动销毁它，因此 FrameSource 与 session 同寿命。
- `OpenCvCamera` 拥有 `cv::VideoCapture`（已打开的设备），与对象同寿命；构造失败时抛异常，对象不诞生。
- `app` 局部持有 `CameraSession`；窗口由 OpenCV HighGUI 管理。

### 2. CameraSession 有哪些状态，哪些转换是合法的？

```text
Idle → Running（start）
Running → Failed（后台读帧出错）
Running / Failed → Idle（stop 完成 join 后）
```

非法转换：`Running`/`Failed` 时再次 `start()` 抛 `std::logic_error`；`Idle` 时 `stop()` 是空操作（合法且幂等）。

### 3. 哪些公共方法允许被不同线程同时调用？

- `stop()`：任意线程可调，幂等；
- `state()` / `error()` / `consume_latest()`：内部加锁，由控制线程（主线程）调用；
- `start()`：仅控制线程、且状态为 Idle 时调用，否则抛错。

### 4. mutex 保护哪些成员以及什么不变量？

`mutex_` 保护 `stop_requested_`、`state_`、`error_`、`newest_`、`oldest_`。

不变量：

- 缓冲最多两帧：`newest_` 为最新帧，`oldest_` 为次新帧（若存在）；
- 状态与错误只在锁内改变；`kFailed` 与 `error_` 同时设置；
- 慢操作（`read()`、`imshow()`、`imwrite()`）在锁外执行。

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

两个槽位都满时，后台发布新帧会把旧 `newest_` 移到 `oldest_`，**覆盖并丢弃原来的最旧帧**。原因：摄像头是持续数据源，UI 只关心最新画面；丢旧保新保证显示始终最新，内存有界。

### 6. 为什么不能持锁调用 VideoCapture::read()、imshow() 或 imwrite()？

`read()` 会阻塞等待摄像头送帧（可达 33ms 以上）；`imshow()`/`imwrite()` 是慢 I/O。持锁执行会阻塞其他线程取帧和停止，等于退回串行。锁只保护共享状态的一次完整转换，临界区应尽量小。

### 7. 后台异常通过什么机制越过线程边界？

后台循环 `catch(...)` 后用 `std::current_exception()` 把异常**冻结**为 `std::exception_ptr` 存入 `error_`，状态置 `kFailed` 并请求停止；主线程通过 `state()`/`error()` 观察到后，用 `std::rethrow_exception()` 重新抛出，再以普通 `try/catch` 报告。异常类型完整保留。

### 8. 析构时执行 stop()、唤醒和 join() 的顺序是什么？

析构 → `stop()`：锁内置 `stop_requested_ = true` → `notify_all()` 唤醒等待中的线程 → `join()` 等后台线程真正退出 → 锁内置 `state_ = kIdle`。此后才允许成员销毁（如 `source_`），保证后台线程不再访问任何成员。

## 真机测试记录

- 设备：Legion Y7000 2024 内置摄像头（`Integrated Camera`，`/dev/video0`）
- 操作系统：Ubuntu 22.04.5 LTS (jammy)
- 分辨率：1280×720
- 结果：窗口实时显示、`S` 保存、`Q` 干净退出

- 常见问题：摄像头能打开但画面全黑 → `v4l2-ctl --device /dev/video0 --list-ctrls` 查看 `privacy`；若 `privacy=1` 且只读，说明机身隐私拨片/快捷键关闭了摄像头，打开后恢复正常。
