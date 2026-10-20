# 摄像头监控作业：把单线程程序重构为并发组件

本目录把原来的 `starter/` 单线程程序重构为三个可独立测试的组件，并保留完全相同的外部行为：

```text
sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]
```

| 按键 / 事件 | 行为 |
| --- | --- |
| `S` | 把当前画面保存成 `captures/capture-N.png` |
| `Q`、关闭窗口 | 停止采集、`join` 工作线程后退出 |

窗口左上角的叠加文字与起始版本一致：帧序号 + 实际显示帧率。丢帧数与空帧数通过
`CameraSession::stats()` 暴露，不改变用户可见的窗口内容。

## 与原程序的三处有意差异

| 现象 | 起始版本 | 本次实现 | 原因 |
| --- | --- | --- | --- |
| 读到空帧 | 直接抛异常退出 | 计入 `stats().empty` 后跳过 | 部分 USB 摄像头在切换分辨率时会偶发空帧，直接退出对用户不友好；空帧仍然**不会**被当作有效画面发布（自动测试 7 覆盖） |
| 保存截图失败 | 抛异常退出 | `stderr` 报告后继续预览 | 一次写盘失败不该中断正在进行的采集；错误仍然是可见的 |
| 叠加文字 | `frame N  X.X FPS` | 保持不变 | 「保持外部行为」要求窗口内容与命令行完全一致 |

## 重构前后职责对比

| 关注点 | 重构前（`starter/main.cpp`） | 重构后 |
| --- | --- | --- |
| 摄像头 | 主线程直接 `cv::VideoCapture::read()`，读一帧就卡住整个程序 | 交给 `FrameSource` 实现；默认 `OpenCvCamera`，测试用 `FakeFrameSource` |
| 线程 | 只有主线程，读帧 / 显示 / 保存互相阻塞 | 工作线程只读帧；主线程只做窗口、按键、叠加、截图 |
| 帧缓冲 | 单个 `cv::Mat`，显示慢就整体变慢 | `CameraSession` 内有界队列，容量 2 帧，满时丢最旧的一帧 |
| 停止 | 靠 `break` 跳出循环，没有显式状态 | `Idle ⇄ Running` 状态机 + `stop()` / `join()` / `close()`，`stop()` 可重复调用 |
| 错误 | 就地 `std::cerr` 输出后静默继续 | 后台异常转存为 `std::exception_ptr`，由主线程取出并报告 |
| 可测试性 | 必须插着摄像头才能跑 | 7 项 CTest 用例全程不碰设备 0 |
| 并发正确性 | 不涉及 | `mutex` 只保护状态与缓冲；读帧、`imshow`、文件写入都在锁外 |

## 目录结构

```text
programs/camera_monitor/
├── CMakeLists.txt          # find_package(OpenCV) / Threads，子目录注册
├── README.md
├── include/camera/         # 对外接口（header-only 声明）
│   ├── frame.hpp           # Frame：图像 + 序号 + 采集时刻
│   ├── frame_source.hpp    # FrameSource 抽象接口
│   ├── opencv_camera.hpp   # 真实设备实现
│   ├── fake_frame_source.hpp # 测试替身 + SourceProbe 观察点
│   ├── camera_session.hpp  # 采集线程 + 有界缓冲 + 状态机
│   └── preview_application.hpp # 主线程窗口/按键/截图
├── src/                    # 实现，编译进 camera_homework_core 库
├── app/main.cpp            # 命令行解析与对象组装
├── tests/                  # 硬件无关的 CTest 用例
└── starter/main.cpp        # 重构前的原始程序，保留作对照，不再参与构建
```

`app/main.cpp` 里的一次组装就体现了分层：

```cpp
auto source = std::make_unique<camera::OpenCvCamera>(settings);
auto session = std::make_unique<camera::CameraSession>(std::move(source));
camera::PreviewApplication application{std::move(session), {}};
return application.run();
```

`CameraSession` 用 `std::unique_ptr<FrameSource>` 取得所有权，因此换设备只需要换这一行，
业务代码里不需要「测试模式」分支。

## 构建与运行

需要 OpenCV 4 的开发包（Ubuntu / WSL）：

```bash
sudo apt update && sudo apt install -y libopencv-dev
```

```bash
cmake -S . -B build-camera -G Ninja \
      -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON \
      -DCMAKE_CXX_COMPILER=g++
cmake --build build-camera
ctest --test-dir build-camera --output-on-failure
./build-camera/sturdy-guide-camera            # 默认 0 号设备、1280x720
./build-camera/sturdy-guide-camera --device 2 --width 640 --height 480
```

这个开关默认是 `OFF`，所以根目录的 CI（只装 `ninja-build`，不装 OpenCV）不会编译本目录。

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得够久？

- `FrameSource` 由 `CameraSession` 以 `std::unique_ptr` 独占，生命周期随会话；
  会话析构时先 `join` 再销毁 `source_`，所以设备对象永远不会在读帧中途被释放。
- 截图目录由 `PreviewApplication` 按需创建，不持有句柄。
- `FakeFrameSource` 的 `SourceProbe` 用 `shared_ptr` 共享，测试因此可以在设备销毁后继续观察计数。

### 2. `CameraSession` 有哪些状态，哪些转换合法？

```text
Idle ──start() 成功──▶ Running ──stop() / 后台异常──▶ Idle
  ▲                        │
  └────────stop()──────────┘        （Idle 时 stop() 是空操作）
```

- `Idle → Running`：仅 `start()`；设备打不开时返回 `false` 并停在 `Idle`。
- `Running → Running`：非法，`start()` 抛 `std::logic_error`。
- `Running → Idle`：`stop()`，或工作线程遇到不可恢复错误。
- `Idle → Idle`：`stop()` 可重复调用；`Running → Idle → Running` 允许反复启动。

### 3. 哪些公共方法允许被多线程同时调用？

| 方法 | 线程约定 |
| --- | --- |
| `start()` / `stop()` | 只允许调用线程；两者之间不要求并发安全 |
| `wait_for_frame()` | 只允许调用线程（只有一个消费者） |
| `has_error()` / `take_error()` / `stats()` / `description()` | 任意线程，内部持锁或只读成员 |
| `is_running()` | 任意线程，读原子量，不加锁 |

工作线程只调用 `FrameSource::read()`，不碰任何公共方法。

### 4. `mutex_` 保护哪些成员和什么不变量？

保护 `buffer_`、`stats_`、`error_`、`stop_requested_`，不变量是：

- `stats_.captured == stats_.delivered + stats_.dropped + buffer_.size()`；
- `buffer_.size() <= kMaxBufferedFrames`；
- `error_` 一旦置位就不会被后台线程覆盖（只由 `take_error()` 清除）。

`running_` 是 `std::atomic<bool>`，因为 `is_running()` 需要无锁读取。

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

丢**最旧**的一帧（`buffer_.pop_front()`），然后 `push_back` 新帧。

预览场景关心的是「现在镜头里是什么」，不是补播历史；如果丢最新帧，画面会莫名其妙地停在旧内容上，
而消费者只要慢一点就会被永久甩在后面。丢最旧帧保证消费者下一次取到的永远是当前最新的可用画面。

### 6. 为什么不能持锁调 `VideoCapture::read()` / `imshow()` / `imwrite()`？

- `read()` 会阻塞到下一帧到来（几十毫秒甚至更长）。持锁调用会让调用线程的 `wait_for_frame()`、
  `stats()`、`stop()` 全部排队，停止操作要等一次完整读帧，看起来就是「关窗口卡住」。
- `imshow()` / `imwrite()` 会进入 GUI 或文件系统，同样可能长时间阻塞，而且它们不需要保护会话内部状态。

因此 `worker_loop()` 在锁外读帧，`PreviewApplication` 在锁外显示与写盘，锁只覆盖状态转换本身。

### 7. 后台异常通过什么机制越过线程边界？

`worker_loop()` 用 `catch (...)` 捕获后调用 `record_error(std::current_exception())`：

1. 持锁把 `exception_ptr` 存进 `error_`（已有异常则不覆盖）；
2. `running_.store(false)`；
3. `frame_ready_.notify_all()` 唤醒可能阻塞在 `wait_for_frame()` 的主线程。

主线程在 `wait_for_frame()` 返回 `false` 时检查 `has_error()`，并用 `take_error()` 取出异常、
`std::rethrow_exception()` 还原成普通 `std::exception` 后再打印——stderr 的写入仍然只发生在主线程。

### 8. 析构时 `stop()`、唤醒、`join()` 的顺序是什么？

```cpp
running_.store(false);                    // 1. 先让状态对外可见地停下
{ lock; stop_requested_ = true; }         // 2. 置停止标志（受锁保护）
frame_ready_.notify_all();                // 3. 唤醒等帧的主线程
if (worker_.joinable()) worker_.join();   // 4. 等工作线程退出循环
source_->close();                         // 5. 线程已退出，关闭设备不会与 read() 并发
```

工作线程只在一次 `read()` 返回**之后**观察 `stop_requested_`，所以 `stop()` 最多等一次读帧的耗时；
`close()` 约定可重复调用，因此「先 `stop()` 再析构」不会重复关闭出错。

## 自动测试

全部用例不需要真实摄像头，也不需要图形窗口（`ctest` 可在无显示环境下跑）。

| # | 用例 | 覆盖的契约 |
| --- | --- | --- |
| 1 | `test_start_delivers_frames` | `start()` 后能取到非空帧，`sequence` 严格递增且从 1 开始 |
| 2 | `test_repeated_start_throws` | 重复 `start()` 抛 `std::logic_error`，且不影响已在运行的会话 |
| 3 | `test_repeated_stop_is_safe` | `stop()` 未启动可用、可重复、停止后可再次启动 |
| 4 | `test_worker_joined_before_destruction` | 析构后 `reads_in_flight == 0`、设备已销毁并已 `close()` |
| 5 | `test_buffer_stays_bounded` | 缓冲 `≤ 2` 帧、丢帧数增长、丢的是最旧帧、三者计数守恒 |
| 6 | `test_read_error_is_observable` | 后台异常在主线程可见，异常信息原样传递，`take_error()` 会清除 |
| 7 | `test_empty_frames_are_not_published` | 空帧不发布但被计数，已发布帧的数量守恒 |

另有一组 `fake_frame_source_test`，保证测试替身本身的行为（未 `open` 不能读、`close()` 幂等、
`total_frames` 会耗尽、`empty_at` / `throw_at` 语义正确）是可靠的。

## 关于本机运行截图

`programs/camera_monitor` 的目标环境是直接装有摄像头的 Linux 机器。在 WSL 里默认没有
`/dev/video*`，因此本目录的运行截图取自 …（提交 PR 时补上真实设备截图）。
