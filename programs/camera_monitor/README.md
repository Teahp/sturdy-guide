# sturdy-guide-camera：把单线程摄像头程序重构为并发组件

把 `starter/main.cpp`（单线程、一切堆在 `main()`）重构为封装良好、可测试的多线程程序：
后台线程采集，主线程只负责窗口、叠加与截图，二者通过有界的最新帧缓冲解耦。

## 构建与运行

```bash
# 从仓库根目录
cmake -S . -B build-camera -G Ninja -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera

# 运行（设备编号、分辨率与起始版本一致）
./build-camera/sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]

# 无摄像头环境的演示模式：用假帧源跑完整窗口/叠加/截图流程
./build-camera/sturdy-guide-camera --fake --width 640 --height 480

# 按键：S 保存截图到 captures/；Q 或关闭窗口退出
```

运行自动测试（不依赖真实摄像头）：

```bash
ctest --test-dir build-camera --output-on-failure
```

## 目录结构

```text
programs/camera_monitor/
├── starter/main.cpp       原始单线程版本（保留对照，不参与最终构建）
├── include/camera/        公共接口
│   ├── frame_source.h     FrameSource 抽象接口
│   ├── opencv_camera.h    真实设备实现
│   ├── fake_frame_source.h 测试假设备
│   ├── camera_session.h   采集会话（状态机 + 有界缓冲）
│   └── preview_app.h      主线程 UI 循环
├── src/                   实现（静态库 target: sturdy_guide_camera_core）
├── app/main.cpp           参数解析、依赖组装与窗口循环（target: sturdy-guide-camera）
├── tests/                 硬件无关测试（target: camera_session_test，注册到 CTest）
├── CMakeLists.txt         查找 OpenCV/Threads 并编排子目录
└── README.md              本文件
```

## 重构前后职责对比

| 职责 | 起始版本 | 重构后 |
|---|---|---|
| 参数解析 | `main()` 内联 | `app/main.cpp` |
| 设备访问 | `main()` 直接持有 `cv::VideoCapture` | `OpenCvCamera`（实现 `FrameSource` 接口）|
| 采集线程生命周期 | 无（单线程）| `CameraSession::start/stop/join` 状态机 |
| 帧缓冲 | 无（读一帧显示一帧）| `CameraSession` 有界两帧缓冲（满则丢最旧）|
| 帧率统计与叠加 | `main()` 内联 | `PreviewApplication` |
| 截图 | `main()` 内联 | `PreviewApplication::save_screenshot` |
| 可测试性 | 依赖真实硬件，无法稳定测试 | `FakeFrameSource` + CTest，硬件无关 |

核心逻辑编译为库 `sturdy_guide_camera_core`；应用与测试分别链接该库；各子目录用嵌套
`CMakeLists.txt` 管理自己的 target；通过 `find_package(OpenCV)` / `find_package(Threads)`
表达依赖，不写死 `.so` 路径。

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得足够久？

- `CameraSession` 通过 `std::unique_ptr<FrameSource>` **独占**帧源：会话运行期间帧源只被后台线程
  读写（`open/read`），`stop()` 的 `join()` 之后才在锁外 `release()`。
- `CameraSession` 拥有后台 `std::thread`；析构函数调用 `stop()` 完成 `join()`，保证线程先于对象销毁。
- `PreviewApplication` 只持有 `CameraSession&`（不拥有）；`app/main.cpp` 保证 `session` 先于 `app`
  创建、后于 `app` 销毁（局部对象逆序析构自然满足）。
- 使用约定：调用方保证不在对象销毁的同时从其他线程调用其方法（单一控制线程 + 析构同步）。

### 2. `CameraSession` 有哪些状态，哪些转换是合法的？

- 状态：`Idle`（未启动）→ `Running`（运行中）→ `Stopped`（已停止）。
- 合法转换：

```text
Idle --start()--> Running --stop()--> Stopped --start()--> Running（可重启）
```

- `start()` 在 `Running` 时是 **no-op**（幂等）；`stop()` 在非 `Running` 时是 **no-op**（幂等，可连续调用）。
- 后台线程因读帧失败而退出时：`error_` 记录异常、`stop_requested_` 置位，`state()` 仍为 `Running`，
  直到调用 `stop()` 完成收尾 —— 保证**任何时刻至多一个 worker 线程**（重启必须先 `stop()`）。

### 3. 哪些公共方法允许被不同线程同时调用？

- `wait_frame` / `try_frame` / `take_error` / `state` / `frame_count` / `buffered_frames`：
  线程安全，可被任意线程并发调用。
- `start` / `stop`：线程安全且幂等，但设计上期望由同一控制线程（主线程）串行调用；
  交错调用 `start/stop` 不在契约内。
- 析构不得与任何其他方法并发（调用方的责任，见第 1 问）。

### 4. mutex 保护哪些成员以及什么不变量？

`mutex_` 保护：`frames_`（有界缓冲）、`error_`（跨线程异常）、`stop_requested_`、`state_`、`frame_count_`。

不变量：

- `frames_.size() <= kMaxBufferedFrames`（= 2）恒成立；
- 帧源只被后台线程访问（`release()` 在 `join()` 之后执行）；
- 后台异常要么在 `error_` 中等待被 `take_error()` 取走，要么已被取走（取出即清除）。

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

缓冲是**最新帧缓存**：生产者 push 新帧，超过 2 帧时 `pop_front()` 丢弃**最旧**帧；消费者
`wait_frame()` 只取最新一帧并清空缓冲。

原因：预览/显示只需要最新画面；保留旧帧只会增加显示延迟。消费落后时丢弃旧帧，队列永不无限增长。

### 6. 为什么不能持锁调用 `VideoCapture::read()`、`imshow()` 或 `imwrite()`？

这些都是慢速 I/O（`read()` 一帧可达数十毫秒；`imshow`/`imwrite` 涉及窗口系统与磁盘）。若持锁执行，
临界区被长时间霸占：生产线程阻塞消费线程，所有调用方互相等待，UI 卡死、丢帧加剧。

正确做法：锁只保护共享状态的**快速转换**（push/pop 一个 `cv::Mat` 句柄、读写标志与计数），
慢 I/O 一律在锁外。本实现中 `read()/open()/release()` 都在锁外；`imshow/imwrite/putText`
只在主线程（`PreviewApplication`）执行，不持有任何锁。

### 7. 后台异常通过什么机制越过线程边界？

后台线程 `run()` 捕获所有异常（打开失败、读取失败、流结束），在锁内把 `std::exception_ptr`
存入 `error_` 并置 `stop_requested_`；主线程通过 `take_error()` 取出（取出即清除，可重入），
再 `std::rethrow_exception` 或检查 `what()`。`std::exception_ptr` 可在任意线程间安全传递。

### 8. 析构时执行 `stop()`、唤醒和 `join()` 的顺序是什么？

1. 锁内置 `stop_requested_ = true`；
2. `frame_available_.notify_all()` 唤醒可能在 `wait_frame()` 中等待的线程；
3. `worker_.join()` 等待后台线程退出（当前 `read()` 返回后即检查停止标志并退出）；
4. 锁内置 `state_ = Stopped`；
5. `source_->release()`（锁外；`join()` 之后无并发访问）。

顺序的意义：先置标志保证 worker 必然退出；先唤醒再 join 避免等待者卡死；`release()` 放最后，
保证没有线程再访问设备。

## 本地自动测试

`camera_session_test`（全部使用 `FakeFrameSource`，不打开真实设备编号）：

1. `start()` 后能取得带递增序号的帧；
2. 重复 `start()` 符合契约（运行中为 no-op，停止后可重启）；
3. 连续调用 `stop()` 不崩溃、不死锁；
4. 对象析构前工作线程已被 `join()`（析构等待进行中的 `read()`，随后才 `release()`）；
5. 消费较慢时缓冲不超过两帧，并按约定丢帧；
6. 假设备抛出的读取错误能在主线程被观察；
7. 空帧不会被发布为有效画面。

## 真机测试

`/dev/video*` 不可用的 WSL2 环境请使用原生 Linux 或课程提供的 Linux 机器：

```bash
v4l2-ctl --list-devices   # 或 ls -l /dev/video*
./build-camera/sturdy-guide-camera --device 0
```

记录设备型号、操作系统、分辨率与结果（属于本地集成测试，不替代上述自动测试）。
