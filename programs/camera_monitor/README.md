# 摄像头采集：并发重构

把起始的单线程 `starter/main.cpp` 重构为库、应用与硬件无关测试。
外部行为不变：同样的参数、按键、叠加与错误提示。

## 构建与运行

```bash
# 安装依赖（Ubuntu / 原生 Linux）
sudo apt update
sudo apt install -y libopencv-dev v4l-utils

# 从仓库根目录构建
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera

# 运行（按 Q 退出，按 S 截图到 captures/）
./build-camera/sturdy-guide-camera --device 0

# 硬件无关自动测试
ctest --test-dir build-camera --output-on-failure
```

## 目录结构

```text
camera_monitor/
├── starter/                 原始单线程版本，仅作对照，不参与构建
├── include/camera/          公共接口（抽象与具体类的声明）
│   ├── frame.hpp            Frame：一帧图像 + 采集序号
│   ├── frame_source.hpp     FrameSource 抽象接口
│   ├── camera_session.hpp   CameraSession：状态机 + 有界缓冲
│   ├── opencv_camera.hpp    OpenCvCamera：cv::VideoCapture 实现
│   └── preview_application.hpp  PreviewApplication：主线程 UI
├── src/                     设备与采集会话实现（核心库）
│   ├── camera_session.cpp
│   └── opencv_camera.cpp
├── app/                     参数解析、依赖组装与窗口循环
│   ├── main.cpp
│   └── preview_application.cpp
├── tests/                   不依赖真实摄像头的测试
│   ├── fake_frame_source.*  FakeFrameSource 测试替身
│   └── camera_session_test.cpp
└── CMakeLists.txt           查找依赖并编排子目录
```

核心逻辑编译为库 target `sturdy_guide_camera_core`
（别名 `SturdyGuide::camera_core`），应用与测试分别链接它。

## 三类职责

| 组件 | 职责 | 线程 |
|---|---|---|
| `FrameSource` / `OpenCvCamera` / `FakeFrameSource` | 提供帧；真实设备与测试替身同一接口 | 仅被工作线程调用 |
| `CameraSession` | 独占 `FrameSource`；`start`/`stop`/`join` 状态机；有界两帧缓冲；把后台异常传回调用线程 | 1 个工作线程 + 任意读线程 |
| `PreviewApplication` | 主线程 `imshow`/`waitKey`、截图、退出、用户可见错误 | 仅主线程 |

`CameraSession` 通过 `std::unique_ptr<FrameSource>` 取得设备所有权，
真实与假设备遵循同一接口，业务代码无“测试模式”分支。

## 线程安全契约

1. **资源与生命周期**：`OpenCvCamera` 拥有 `cv::VideoCapture`；
   `CameraSession` 用 `unique_ptr` 独占 `FrameSource`，并拥有工作线程、
   mutex、条件变量、两帧缓冲与 `exception_ptr`；`main()` 在栈上拥有
   `CameraSession`（其析构必先 `stop()`→`join()`），保证 `source_`
   比工作线程活得久（成员析构在 join 之后按逆序进行）。帧像素由
   `cv::Mat` 引用计数管理，无裸指针传递。

2. **状态与转换**：见下方「停止状态机」。

3. **可并发调用的方法**：`tryTakeFrame`、`isRunning`、`hasError`、
   `rethrowError`、`captureFps` 可从任意线程并发调用；`stop()` 幂等且
   可从任意线程调用（不能是工作线程自身）；`start()` 与 `stop()` 由
   控制线程串行调用。

4. **mutex 保护与不变量**：mutex 保护 `state_`、`ready_`、`error_`、
   `capture_fps_`，以及发布时 `ready_` 的翻转。`slots_[2]` 不经“整块
   加锁”保护，而由下标纪律保护：worker 只写 `slots_[write]`（私有
   下标），consumer 在锁内只读 `slots_[ready_]`，且恒有 `write !=
   ready`。不变量：`ready_ == -1` 表示无未消费帧；`error_` 有值 ⇔
   工作线程因错误退出；`state_ == kStopped` ⇒ 线程不可 join；像素
   数据的写先于发布（锁内翻转），发布先于 consumer 的读（同一把锁）。

5. **满时丢弃哪一帧**：丢弃**最旧**帧——worker 发布时把 `ready_` 指向
   新写入的槽位，旧槽位立即成为下一个写入槽并被覆盖。实时预览只关心
   最新画面，丢旧帧使延迟最小、内存有界（恒为两帧，永不增长）。

6. **为何不能持锁调用 `read`/`imshow`/`imwrite`**：这三者会阻塞或很慢
   （`VideoCapture::read` 等下一帧、`imshow` 等窗口系统、`imwrite` 是
   磁盘 I/O）。持锁执行会卡住对侧线程——UI 冻结、采集无法丢帧，且
   `read` 若长期阻塞还会与 `join` 互相等待而死锁。锁只保护纳秒级的
   纯内存操作（换槽 + 置标志），慢 I/O 全部在临界区外。

7. **后台异常如何跨线程**：工作线程用 `try/catch` 包裹整个循环，
   通过 `std::current_exception()` 把异常存入 `std::exception_ptr`（锁内），
   再 `notify_all`；主线程调用 `rethrowError()`，其中用
   `std::rethrow_exception` 重抛。`read()` 返回 `false`（流结束）与抛出
   异常两条路径都收敛到 `fail()`，统一越过线程边界。

8. **析构顺序**：`~CameraSession` 调 `stop()`。`stop()` 依次：
   ① 锁内把 `state_` 置 `kStopping` 并把 `worker_` 移出；② 释放锁；
   ③ `notify_all()` 唤醒等待者；④ 在**锁外** `join()`。join 必须在锁外，
   因为工作线程退出前还要拿锁把 `state_` 置 `kStopped`——持锁 join 会
   死锁。join 返回后，成员按声明逆序析构，`source_` 最后析构。

## 停止状态机

```text
kStopped ──start()────────────────▶ kRunning
kRunning ──stop()/读失败/异常────▶ kStopping
kStopping ──join() 完成──────────▶ kStopped
kStopped  ──start()────────────────▶ kRunning   （允许重启）
```

- `start()` 在 `kRunning` 时是 no-op（不重新 open）。
- `Q`、窗口关闭、读帧失败、异常、析构五条退出路径最终都收敛到
  `kStopping → kStopped`。

## 与起始版本的差异

外部行为保持一致：相同参数、默认值、`--help` 文案、按键、叠加格式、
截图命名（`captures/capture-N.png`）与错误前缀 `camera:`。

唯一有意变更：起始版本把“读到空帧”当作致命错误；本实现按作业要求
改为**跳过空帧、不发布**（见自动测试第 7 项）。真实设备上
`read()` 返回 `false`（流结束）仍会以
`camera stopped returning valid frames` 报告。
