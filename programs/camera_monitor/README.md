# Camera Monitor — Refactored Multi-threaded Architecture

A dual-threaded camera capture and preview application built on OpenCV,
refactored from the original single-threaded `starter/main.cpp`.

## Building

```bash
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera

# Run (requires /dev/video0)
./build-camera/sturdy-guide-camera --device 0

# Run tests (no hardware needed)
cd build-camera && ctest --output-on-failure
```

## Thread Safety Notes

### 1. 每个数据由谁拥有？谁确保它活得够久？

| 数据 | 所有者 | 生命周期保证 |
|---|---|---|
| `FrameSource` (摄像头设备) | `CameraSession` 持有 `unique_ptr` | CameraSession 析构时自动释放 |
| `worker_` (后台线程) | `CameraSession` 持有 `std::thread` | `join()` 回收，析构函数兜底调用 |
| `CameraSession` | `PreviewApplication` 持有 `unique_ptr` | PreviewApplication 析构时先 `stop()+join()` 再销毁 |
| 帧缓冲区 `buffer_[2]` | `CameraSession` 私有成员 | 与 CameraSession 同生命周期 |
| `FrameSource::read()` 返回的 `cv::Mat` | 返回值局部持有 | move 到 buffer 后由 buffer 持有 |

`main()` 中的对象生命周期链：
`main` → `PreviewApplication`(栈) → `unique_ptr<CameraSession>` → `unique_ptr<FrameSource>`。

### 2. CameraSession 有哪些状态、哪些转换是合法的？

```
                start()
  [idle] ──────────────→ [running]
    ↑                       │
    │    stop() + join()    │
    └───────────────────────┘
```

- **idle** → `start()` → **running**：唯一合法启动路径
- **running** → `stop()` → **running**(信号已设) → `join()` → **idle**：正常关闭
- **idle** → `start()` 抛 `std::logic_error`：禁止重复启动
- **running** → `start()` 抛 `std::logic_error`：同上
- 析构函数：自动执行 `stop()` + `join()`，保证线程退出

### 3. 哪些数据在不同线程间同时读写？

| 数据 | 写线程 | 读线程 | 风险 |
|---|---|---|---|
| `buffer_[2]` | 后台 worker（新帧写入） | 主线程 `latest_frame()` 读取 | 数据竞争 |
| `buffer_dirty_[2]` | 后台 worker（标记脏） | 主线程（判断是否有帧） | 数据竞争 |
| `worker_error_` | 后台 worker（catch 中存入） | 主线程 `error()` 读取 | 数据竞争 |
| `stop_requested_` | 主线程 `stop()` 写入 | 后台 worker 循环检查 | 数据竞争 |
| `running_` | 后台 worker `run()` 末尾写 false | 主线程 `is_running()` 读取 | 数据竞争 |
| `worker_` | 主线程 `join()` 中 move | — | 需要同步 |

以上全部通过 `mutex_` 保护，每次访问前后加 `std::scoped_lock`。

### 4. mutex 保护了哪些成员？为什么是那些？

```cpp
mutable std::mutex mutex_;
std::optional<cv::Mat> buffer_[2];        // 跨线程读写的帧数据
bool buffer_dirty_[2];                    // 跨线程读写的脏标记
std::exception_ptr worker_error_;         // 跨线程传递的异常
bool stop_requested_;                     // 跨线程传递的停止信号
bool running_;                            // 跨线程查询的运行状态
std::thread worker_;                      // join 时需要 move 的线程句柄
```

**为什么是这些而不是别的？** 因为这些是唯一被两个线程同时访问的数据。`source_`（FrameSource）只在后台线程中被调用 `read()`，主线程不触碰，所以不需要锁。`window_name_` 等主线程私有数据同理。

### 5. 如果两帧到来，旧帧被丢弃的原因是什么？

帧缓冲区最多容纳 2 帧（双槽：slot 0 = 次新，slot 1 = 最新）。当新帧到达且 slot 1 已有数据时：
```
buffer_[0] = std::move(buffer_[1]);  // 旧帧覆盖到 slot 0（丢弃更旧的）
buffer_[1] = std::move(frame);       // 新帧写入 slot 1
```
**丢弃旧帧是为了解决画面延迟堆积问题。** 如果不丢弃，后台持续采集而主线程渲染较慢，帧会无限堆积，用户看到的画面越来越滞后。丢弃后 `latest_frame()` 永远返回最新一帧，保证实时性。

### 6. 为什么不能在持有 mutex 时执行 read()、imshow()、imwrite()？

```cpp
// 后台线程：read() 在锁外
auto frame = source_->read();   // ← 可能阻塞数毫秒（硬件I/O）
// ... 然后加锁写 buffer

// 主线程：imshow 在锁外
cv::imshow(window_name_, frame); // ← 可能耗时（GPU渲染）
```

原因：
1. **`read()` 是阻塞调用**（等待硬件帧就绪），如果持锁调用，主线程的 `latest_frame()` 也会被阻塞，导致窗口冻结、无法响应按键。
2. **`imshow()`/`imwrite()` 是耗时操作**（图像编码、GPU 渲染、磁盘 I/O），持锁会阻塞后台线程写入新帧，造成采集停滞。
3. **锁的范围应尽量小**——只保护轻量的标志位和指针/move 操作，不包裹 I/O。

### 7. 后台异常通过什么机制跨线程边界传递？

```
后台线程 run():
  try { ... }
  catch (...) {
    const std::scoped_lock lock{mutex_};
    worker_error_ = std::current_exception();  // 存储异常指针
  }

主线程:
  if (auto err = session_->error()) {          // 读取异常指针（加锁）
    session_->stop();
    session_->join();
    std::rethrow_exception(err);               // 在主线程中重新抛出
  }
```

使用 `std::exception_ptr` 跨线程传递异常。这是 C++ 标准提供的线程间异常传递机制，`exception_ptr` 本身是可拷贝的，内部异常对象的生命周期由引用计数管理。

### 8. 调用 stop() 再调 join()，顺序有什么要求？

**必须先 stop() 再 join()，顺序不可颠倒。**

- `stop()` 仅设置标志位 `stop_requested_ = true`，不阻塞，立即返回。
- `join()` 阻塞等待后台线程退出。如果先调 `join()` 而不调 `stop()`，后台线程的 `read()` 可能一直阻塞（真实摄像头），`join()` 将永远不返回——**死锁**。

典型调用路径：
```cpp
session_->stop();   // 非阻塞：告诉线程"请退出"
session_->join();   // 阻塞：等线程真的退出了
```

析构函数保证即使上层忘记调用，也会自动执行 `stop()` + `join()`。
