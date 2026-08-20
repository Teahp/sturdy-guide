# camera_monitor 摄像头作业

把单线程摄像头程序重构为并发组件。保留原有外部行为（`--device`/`--width`/`--height` 参数、`S` 保存截图、`Q` 退出），只改变内部结构。

## 目录结构

```text
camera_monitor/
├── starter/main.cpp       原始单线程版本（保留对照，不再编译）
├── include/camera/        公共接口
│   ├── frame_source.hpp       FrameSource 抽象接口
│   ├── opencv_camera.hpp      真实设备实现
│   ├── fake_frame_source.hpp  测试假源
│   └── camera_session.hpp     采集会话（状态机 + 后台线程）
├── src/                   设备与采集会话实现
│   ├── opencv_camera.cpp
│   ├── fake_frame_source.cpp
│   └── camera_session.cpp
├── app/                   参数解析、依赖组装和窗口循环
│   ├── preview_application.hpp/.cpp
│   └── main.cpp
├── tests/                 不依赖真实摄像头的测试
│   └── camera_session_test.cpp
├── CMakeLists.txt         查找依赖并编排子目录
└── README.md              构建、运行与设计说明
```

## 构建与运行

```bash
# 依赖：OpenCV 4（core/highgui/imgcodecs/imgproc/videoio）、CMake、C++17 编译器

cmake -S . -B build-camera -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
./build-camera/programs/camera_monitor/app/sturdy-guide-camera --device 0
```

按键：`S` 保存截图到 `captures/`，`Q` 或关闭窗口退出。

## 自动测试

```bash
ctest --test-dir build-camera -C Release
```

测试只使用 `FakeFrameSource`，不打开任何真实设备。覆盖 7 个验证点：

1. `start()` 后能取得带递增序号的帧；
2. 重复 `start()` 符合契约（安全 no-op）；
3. 连续调用 `stop()` 不会崩溃或死锁；
4. 对象析构前工作线程已经被 `join()`；
5. 消费较慢时缓冲区始终不超过两帧，并按约定丢弃最旧帧；
6. 假设备抛出的读取错误能在主线程被观察；
7. 空帧不会被发布为有效画面。

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得足够久？

- `FrameSource`（及其实现）由 `CameraSession` 通过 `std::unique_ptr` **独占所有**。`CameraSession` 的构造参数是 `std::unique_ptr<FrameSource>`，所有权在构造时转移，`CameraSession` 析构时释放。
- `CameraSession` 拥有 `std::thread worker_`，在 `stop()` 或析构时 `join()`，保证线程生命周期结束于对象之前。
- `PreviewApplication` 只持有对 `CameraSession` 的**引用**，不拥有它；`main()` 中 `CameraSession` 先于 `PreviewApplication` 构造、后于其析构，保证引用始终有效。
- 真实设备 `cv::VideoCapture` 是 `OpenCvCamera` 的成员，由 `OpenCvCamera` 拥有，后者又由 `CameraSession` 拥有，随对象图一起析构。

### 2. `CameraSession` 有哪些状态，哪些转换是合法的？

状态：`stopped`（未启动/已停止）→ `running`（工作线程运行中）。

| 转换 | 合法？ | 说明 |
|------|--------|------|
| `stopped → running` | ✅ | `start()` |
| `running → stopped` | ✅ | `stop()`：置 `stop_requested_`，唤醒并 `join()` |
| `running → running` | ✅ | 重复 `start()` 是 **no-op**，不会创建第二个线程 |
| `stopped → stopped` | ✅ | 重复 `stop()`、未启动时 `stop()` 都是安全 no-op |
| `running → (错误)` | ✅ | 后台读帧失败/抛异常：记录 `error_`，工作线程自行退出并唤醒等待者 |

`stop()` 不是析构函数的一部分却必须在析构前调用：析构函数内部调用 `stop()`，因此直接析构也是合法的收束路径。

### 3. 哪些公共方法允许被不同线程同时调用？

- `start()` / `stop()` / `running()`：任意线程可调用，内部有互斥锁保护。`stop()` 可在主线程（Q 键/窗口关闭）或任何其他线程触发。
- `wait_for_frame()`：允许多个消费者线程并发调用，互斥锁保护缓冲区。
- `has_error()` / `error()`：任意线程可调用，只读查询。
- `worker_loop()`：仅工作线程调用（私有）。

### 4. mutex 保护哪些成员以及什么不变量？

`mutex_` 保护：`started_`、`stop_requested_`、`frames_`（缓冲队列）、`error_`。与之配套的 `cv_` 条件变量用于唤醒等待帧或等待结束的线程。

不变量：

- 缓冲队列长度 `<= kMaxBufferedFrames (2)`；
- `error_ != nullptr` ⇒ 工作线程已退出且不会再有新帧；
- `stop_requested_ == true` ⇒ 工作线程在下一个安全点退出；
- 队列中的每个元素都是非空帧（空帧在入队前被丢弃）。

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

**丢弃最旧的帧**（`frames_.pop_front()`）。实时预览场景中，旧帧的信息价值最低——画面已经过时，观众看到的是最新画面，延迟越小越好。保留最新帧还能让叠加的帧序号连续递增，避免 UI 上的序号跳变。

### 6. 为什么不能持锁调用 `VideoCapture::read()`、`imshow()` 或 `imwrite()`？

- `VideoCapture::read()` 是**慢速阻塞 I/O**：摄像头帧率通常 30 FPS，一次读取可能阻塞数十毫秒。若持锁读取，所有消费者（如 `wait_for_frame`）和 `stop()` 都会被阻塞，UI 出现明显卡顿，甚至 `stop()` 无法及时唤醒线程。
- `imshow()` / `imwrite()` 同样是慢速操作（窗口刷新、磁盘写入），持锁执行会让其他线程（工作线程入队、等待者）全部排队等待。
- 锁应当只保护**共享状态的短暂转换**（入队/出队/标志位），把耗时 I/O 放在临界区之外，这是本实现的基本纪律。

### 7. 后台异常通过什么机制越过线程边界？

`std::exception_ptr` + 互斥锁：

1. 工作线程的 `read()` 抛异常时，在锁内执行 `error_ = std::current_exception()`，然后 `cv_.notify_all()` 唤醒可能正在等待的消费者，工作线程退出。
2. 主线程通过 `has_error()` 观察、通过 `error()` 取出 `std::exception_ptr`，再用 `std::rethrow_exception()` 在**调用线程**重新抛出异常以读取 `what()` 消息。

这样异常对象本身跨线程传递（引用计数保证生命周期），错误信息在主线程可见。

### 8. 析构时执行 `stop()`、唤醒和 `join()` 的顺序是什么？

析构函数直接调用 `stop()`，顺序为：

1. **置标志**：锁内设置 `stop_requested_ = true`；
2. **唤醒**：`cv_.notify_all()`——可能正在 `wait_for_frame` 等待帧的消费者会被唤醒，立即看到队列为空 + `stop_requested_`，返回 `Ended`；
3. **移动并解锁**：把 `worker_` 移出锁保护范围，立刻释放互斥锁（**不能在持锁时 join**，否则工作线程退出前需要拿锁，形成死锁）；
4. **join**：锁外调用 `worker_to_join.join()`，等待工作线程完成当前 `read()` 并退出；
5. **收尾**：重新加锁把 `started_` 置为 `false`。

工作线程内部每个循环也会检查 `stop_requested_`，保证即使在两次读取之间也能及时退出。

## 重构前后职责对比

| 职责 | 重构前（starter/main.cpp） | 重构后 |
|------|--------------------------|--------|
| 参数解析 | `main()` 内联 | `main()` 的 `parse_options()` |
| 设备访问 | `main()` 直接持有 `cv::VideoCapture` | `OpenCvCamera`（实现 `FrameSource`） |
| 采集循环 | 主线程 `for(;;)` 串行 | `CameraSession` 后台工作线程 |
| 帧率统计 | `main()` 内联 | `PreviewApplication` |
| 窗口/按键/叠加 | `main()` 内联 | `PreviewApplication` |
| 截图保存 | `main()` 内联 | `PreviewApplication::save_capture()` |
| 错误处理 | `main()` 的 try/catch | 后台异常经 `exception_ptr` 传回主线程 |
| 线程生命周期 | 无（单线程） | `CameraSession` 状态机 + `join()` |
