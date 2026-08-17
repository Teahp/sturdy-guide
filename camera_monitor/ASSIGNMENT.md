# 回家作业：把单线程摄像头程序重构为并发组件

## 作业背景

`starter/main.cpp` 是一个可以直接运行的 C++17 摄像头程序。它通过 OpenCV 调用系统已经识别的设备：

- 笔记本电脑内置摄像头；或
- 支持 UVC（USB Video Class）的常见免驱 USB 摄像头。

“免驱”表示操作系统已经提供通用驱动。Linux 通常把设备暴露为 `/dev/video0`、`/dev/video1` 等节点，应用不需要编写内核驱动。

起始版本已经能够：

- 通过参数选择设备编号与分辨率；
- 单线程读取并显示画面；
- 叠加帧序号和采集帧率；
- 按 `S` 保存截图；
- 按 `Q` 或关闭窗口退出；
- 对常见输入和设备错误输出提示。

它的功能基本完整，但工程结构有意写得不好：参数解析、设备访问、帧率统计、窗口事件和文件保存全部堆在 `main()`；读取摄像头会阻塞 UI；逻辑依赖真实硬件，无法稳定测试；也没有明确的采集线程生命周期。

你的任务不是重新发明功能，而是在保持外部行为的前提下，把它重构为封装良好、可测试的多线程程序。

## 先运行起始版本

Ubuntu / 原生 Linux 可安装：

```bash
sudo apt update
sudo apt install -y libopencv-dev v4l-utils
v4l2-ctl --list-devices
ls -l /dev/video*
```

从仓库根目录单独启用摄像头作业：

```bash
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
./build-camera/sturdy-guide-camera --device 0
```

若设备存在但无法打开，检查它是否正被其他应用占用，以及当前用户是否属于 `video` 组。不要长期使用 `sudo` 运行图形程序来绕过权限问题。

WSL2 是否能访问摄像头取决于宿主系统和设备转发配置。若没有 `/dev/video*`，请在原生 Linux 或课程提供的 Linux 机器上完成真机测试。

## 重构目标

最终程序继续命名为 `sturdy-guide-camera`，并保持原有参数和按键行为：

```text
sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]
```

在此基础上必须满足：

- 一个后台线程持续读取摄像头；
- 主线程只负责窗口、键盘输入、画面叠加和截图；
- 内存中最多保留两帧，显示落后时丢弃旧帧，队列不得无限增长；
- `Q`、窗口关闭、读帧失败或对象析构都能停止并 `join()` 工作线程；
- 连续调用 `stop()` 是安全的；
- 后台线程中的读取异常能被主线程观察并报告；
- 不在持有内部 mutex 时执行摄像头读取、`imshow()` 或图片写入；
- 不允许使用全局可变状态，也不允许把全部实现换到另一个巨大的类中。

## 要建立的工程边界

类名允许调整，但至少分离以下职责：

```text
FrameSource (抽象接口)
├── OpenCvCamera        使用 cv::VideoCapture 访问真实设备
└── FakeFrameSource     测试时产生确定的假帧或模拟失败

CameraSession
├── 独占一个 FrameSource
├── 管理 start / stop / join 状态机
├── 维护有界的最新帧缓冲区
└── 把后台异常传回调用线程

PreviewApplication
├── 在主线程调用 cv::imshow / cv::waitKey
├── 处理截图、退出和用户可见错误
└── 不直接访问 CameraSession 的 mutex 或工作线程
```

建议让 `CameraSession` 通过 `std::unique_ptr<FrameSource>` 取得设备所有权。这样真实设备和假设备遵循同一接口，测试不需要摄像头，也不需要在业务代码中增加“测试模式”分支。

## 线程安全契约

在 `README.md` 或 PR 描述中明确回答：

1. 每个对象拥有什么资源，谁保证它活得足够久？
2. `CameraSession` 有哪些状态，哪些转换是合法的？
3. 哪些公共方法允许被不同线程同时调用？
4. mutex 保护哪些成员以及什么不变量？
5. 缓冲区满时具体丢弃哪一帧，为什么？
6. 为什么不能持锁调用 `VideoCapture::read()`、`imshow()` 或 `imwrite()`？
7. 后台异常通过什么机制越过线程边界？
8. 析构时执行 `stop()`、唤醒和 `join()` 的顺序是什么？

仅仅“给所有方法加一把锁”不算完成封装。锁应当保护共享状态的完整转换，慢速 I/O 应当位于临界区之外。

## 目录与 CMake 要求

把单文件起始程序重构为以下结构。可以保留 `starter/` 用于对照，但最终 target 不得继续编译 `starter/main.cpp`：

```text
camera_monitor/
├── starter/main.cpp       原始单线程版本
├── include/camera/        公共接口
├── src/                   设备与采集会话实现
├── app/main.cpp           参数解析、依赖组装和窗口循环
├── tests/                 不依赖真实摄像头的测试
├── CMakeLists.txt         查找依赖并编排子目录
└── README.md              构建、运行与设计说明
```

工程要求：

- 核心逻辑编译为库 target；
- 应用和测试分别链接该库；
- 在下级目录使用嵌套 `CMakeLists.txt` 管理各自 target；
- 使用 `find_package(OpenCV ...)` 和 OpenCV 提供的变量或 target，不写死 `.so` 路径；
- 使用 `find_package(Threads REQUIRED)` 和 `Threads::Threads` 表达线程依赖；
- 使用 `add_test()` 把硬件无关测试注册到 CTest。

## 本地自动测试

自动测试不得打开真实的设备编号 `0`。至少使用 `FakeFrameSource` 验证：

1. `start()` 后能取得带递增序号的帧；
2. 重复 `start()` 的行为符合你写明的契约；
3. 连续调用 `stop()` 不会崩溃或死锁；
4. 对象析构前工作线程已经被 `join()`；
5. 消费较慢时缓冲区始终不超过两帧，并按约定丢帧；
6. 假设备抛出的读取错误能在主线程被观察；
7. 空帧不会被发布为有效画面。

真机测试属于本地集成测试。记录设备型号、操作系统、分辨率和结果，但不要用“摄像头上能运行”替代上述自动测试。

## 提交要求

使用 Fork → 功能分支 → PR → Review → Merge 流程。PR 至少包含：

- 重构前后的职责对比；
- 新增源码、测试和嵌套 CMake 配置；
- 线程安全契约与停止状态机说明；
- 一张本地摄像头运行截图；
- 本地构建和 CTest 结果。
