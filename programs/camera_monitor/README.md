# 摄像头回家作业：单线程程序重构为并发组件

把 `starter/main.cpp`（单文件、单线程的可用摄像头程序）重构为
**设备抽象 + 后台采集会话 + 主线程应用 + 硬件无关测试**的多线程工程。
外部行为不变：`sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]`，
按键 `S` 存图、`Q`/关闭窗口退出。

## 构建与运行

从仓库根目录单独启用本作业（依赖 OpenCV）：

```bash
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
ctest --test-dir build-camera --output-on-failure
./build-camera/sturdy-guide-camera --device 0
```

说明：

- 自动测试不打开真实设备 `0`，用 `FakeFrameSource` 驱动 `CameraSession`；
- 真机测试需在原生 Linux（或课程 Linux 机）上完成，并记录设备型号、操作系统、
  分辨率与运行结果，不要用“摄像头上能跑”替代上述自动测试。

## 工程结构

```text
camera_monitor/
├── include/camera/      公共接口
│   ├── frame_source.hpp     画面来源抽象（接口）
│   ├── opencv_camera.hpp    真设备实现声明
│   └── camera_session.hpp   采集会话（后台线程 + 有界缓冲）声明
├── src/
│   ├── opencv_camera.cpp    真设备实现
│   ├── camera_session.cpp   采集会话实现
│   └── CMakeLists.txt       核心库 target
├── app/
│   ├── main.cpp              参数解析与依赖组装（摄像头 -> 会话 -> PreviewApplication）
│   ├── preview_application.hpp/.cpp  PreviewApplication：主线程窗口循环（显示/按键/截图/错误）
│   └── CMakeLists.txt        应用 target
├── tests/camera_session_test.cpp  硬件无关测试（FakeFrameSource）
├── starter/              原始单线程版本，仅作对照，不再编译
├── CMakeLists.txt        查找 OpenCV/Threads，编排子目录
└── README.md
```

## 设计关系

```text
FrameSource（接口：read() -> cv::Mat）
├── OpenCvCamera            真设备：cv::VideoCapture
└── FakeFrameSource（测试）  假设备：确定帧 / 模拟失败 / 空帧

CameraSession  --拥有(unique_ptr)-->  FrameSource
├── 后台工作线程 run()：循环 read() -> 放入有界缓冲（≤ 2 帧）
├── start / stop / 析构状态机
└── std::exception_ptr 把后台异常传回主线程

PreviewApplication（主线程 = 消费者，见 app/preview_application.*）
├── 持有 CameraSession&，take_latest() 取最新帧，锁内取、锁外 imshow
└── 按键 / 截图 / 用户可见错误，不直接访问会话内部锁与线程

app/main.cpp  解析参数并组装：OpenCvCamera -> CameraSession -> PreviewApplication
```

`CameraSession` 通过 `std::unique_ptr<FrameSource>` 独占设备，真/假设备遵循
同一接口，测试不需要摄像头，业务代码也没有“测试模式”分支。

## 线程安全契约

1. **每个对象拥有什么资源，谁保证它活得足够久？**
   `OpenCvCamera` 拥有 `cv::VideoCapture`（OS 摄像头句柄）；
   `CameraSession`通过 `std::unique_ptr<FrameSource>` 拥有设备；
   `Frame` 拥有 `cv::Mat`（引用计数共享）。
   `CameraSession` 析构先 `stop()`（join 工作线程）再释放成员，
   因此工作线程使用 `source_` 期间它一定存活；
   主线程的 `CameraSession`是局部对象，窗口循环结束后才 `stop()` 与销毁。

2. **CameraSession 有哪些状态，哪些转换合法？**
   `Idle → Running → Stopped`。标志位 `running_`（工作线程是否在运行）与
   `stop_requested_`（是否请求停止）。仅在 `Idle` 且无残留线程时 `start()`
   合法（运行中或未 join 的旧线程时抛 `std::logic_error`）；`stop()` 任何
   时候都可调用且幂等；线程自然结束（读帧失败）会把 `running_` 置为 `false`；
   析构隐式执行 `stop()`。

3. **哪些公共方法允许被不同线程同时调用？**
   公开方法 `start/stop/take_latest/buffered_frames/take_error` 约定由单个
   控制线程（主线程）串行调用；后台只有私有 `run()` 一条线程访问成员。
   两条线程通过同一把 `mutex_` 互斥访问共享成员，不存在第三个并发写入方。

4. **mutex 保护哪些成员以及什么不变量？**
   `mutex_` 保护 `buffer_`、`stop_requested_`、`running_`、`next_sequence_`、
   `capture_error_` 与 `worker_` 的生命周期转移。不变量：`buffer_` 不超过
   两帧；`next_sequence_` 单调递增；后台错误被记录后工作线程已结束。
   `source_->read()` 在锁外执行，因为 `source_` 只被工作线程触碰，不属于
   跨线程共享成员。

5. **缓冲区满时具体丢弃哪一帧，为什么？**
   生产者 `push_back` 新帧后，若 `buffer_.size() > 2` 则 `pop_front()` 丢弃
   **最旧**的一帧；消费者 `take_latest()` 只取 `back()` 并清空缓冲。原因是
   显示侧只关心最新画面：旧帧一旦未被及时显示即已过时，保留它既浪费内存又
   增加显示延迟。丢弃最旧使内存有界，且画面总是尽量新。

6. **为什么不能持锁调用 `read()`、`imshow()` 或 `imwrite()`？**
   这些操作是慢 I/O：读摄像头可阻塞到下一帧到来，`imshow`/`imwrite` 也很慢。
   若持锁执行，其他线程（尤其 `stop()`）想拿锁就只能干等，导致停止不及时、
   界面卡顿。锁只应保护“短小的共享状态转换”。本设计把读帧放在 `run()` 的
   锁外；`imshow`/`imwrite` 由主线程对已经从缓冲取出的帧副本执行，不持锁。

7. **后台异常通过什么机制越过线程边界？**
   `run()` 用 `try/catch(...)` 捕获异常后，以 `std::current_exception()`
   存入 `capture_error_`（类型 `std::exception_ptr`），线程随后正常返回。
   主线程每次循环调用 `take_error()` 取出（`std::exchange` 同时清空）并
   `std::rethrow_exception` 打印。`stop()` 与析构不抛出后台错误。

8. **析构时执行 `stop()`、唤醒和 `join()` 的顺序是什么？**
   析构调用 `stop()`：先加锁把 `stop_requested_` 置为 `true`；解锁后把
   `worker_` 移动到局部变量（锁内判断 `joinable()`），再在**锁外**执行
   `worker.join()`。工作线程没有长睡眠，每次循环都会检查停止标志，因此
   无需额外的“唤醒”通道。`join()` 返回后，成员再按声明逆序销毁——保证
   工作线程退出之后才释放缓冲与设备。

## 自动测试（不依赖真实摄像头）

`tests/camera_session_test.cpp` 使用 `FakeFrameSource` 验证 7 件事：

1. `start()` 后能取得带递增序号的帧；
2. 重复 `start()` 抛 `std::logic_error`（写明的契约）；
3. 连续调用 `stop()` 不会崩溃或死锁（含从未 `start()` 的情况）；
4. 对象析构前工作线程已被 `join()`（销毁干净）；
5. 消费较慢时缓冲始终不超过两帧，最新帧仍可取到；
6. 假设备抛出的读取错误能在主线程被观察到；
7. 空帧不会作为有效画面被发布。
