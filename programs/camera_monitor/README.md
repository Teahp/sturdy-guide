# 摄像头监控程序

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

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得足够久？

- FrameSource：通过 std::unique_ptr 独有,构造函数接收所有权，析构时随会话销毁。
- CameraSession：拥有std::thread、std::mutex、条件变量和有界帧缓冲。析构先调用 stop()，保证线程先于对象销毁。
- OpenCvCamera：拥有 cv::VideoCapture。由会话的工作线程调用。open()/read()/close()，线程退出前调用 close() 释放设备。
- PreviewApplication：只持有 CameraSession 引用。app/main.cpp 保证 session 的生命周期覆盖 app.run() 全程。

### 2. CameraSession 有哪些状态，哪些转换是合法的？

Idle -> Running -> Stopping -> Stopped -> Idle

### 3. 哪些公共方法允许被不同线程同时调用？

- stop()、start() 、stop() 、wait_for_frame()、has_error()、error_message()

### 4. mutex 保护哪些成员以及什么不变量？

mutex_ 保护state_、帧缓冲 frames_、frames_produced_、dropped_frames_、background_error_、error_reported_。

不变量：frames_.size() ，frames_produced_

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

丢弃最旧帧，保留最新帧。避免无限增长，又给显示端留出缓冲。

### 6. 为什么不能持锁调用 VideoCapture::read()、imshow() 或 imwrite()？

这些是慢速 I/O，持锁调用会让其他线程阻塞，导致界面卡死。应该把慢速 I/O 放在临界区外。

### 7. 后台异常通过什么机制越过线程边界？

后台异常通过 std::exception_ptr 保存入background_error_，令report_error() 在锁内写入并置 error_reported_ = true，notify_all() 唤醒主线程。主线程通过 has_error() 、 error_message()观察；error_message() 用 std::rethrow_exception 提取 what()。


### 8. 析构时执行 stop()、唤醒和 join() 的顺序是什么？

~CameraSession() 调用 stop()：
1. 锁内把状态置为 Stopping 并 notify_all()，唤醒等待帧的消费者；
2. 释放锁，在锁外 join() 工作线程（持锁 join 会与 worker 抢锁形成死锁）；
3. 重新加锁置 Idle 并再次 notify，等待者据此返回 false。

