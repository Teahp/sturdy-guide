# 摄像头监控程序

把 `starter/main.cpp` 的单线程摄像头程序，重构为设备抽象、后台采集会话、主线程预览三者分离的可测试多线程程序。功能与参数、按键行为保持与起始版本一致。

## 构建与运行

```bash
cmake -S . -B build-camera -G Ninja -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
./build-camera/sturdy-guide-camera --device 0        # S 保存截图，Q 或关窗退出
ctest --test-dir build-camera --output-on-failure
```

参数：`sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]`。自动测试不打开真实设备。

## 工程结构

```text
camera_monitor/
├── starter/main.cpp        原始单线程版，仅作对照，不再编译
├── include/camera/         FrameSource / OpenCvCamera / CameraSession / PreviewApplication
├── src/                    设备与会话实现
├── app/main.cpp            参数解析、依赖组装
├── tests/                  硬件无关测试（FakeFrameSource）
├── CMakeLists.txt          查找依赖并编排子目录（app/、tests/ 各有独立 CMakeLists）
└── README.md               本文档
```

设计关系与单向依赖：

```text
app → PreviewApplication → CameraSession → FrameSource ← OpenCvCamera / FakeFrameSource
```

`CameraSession` 通过 `std::unique_ptr<FrameSource>` 独占帧来源，真实设备与假设备遵循同一接口，测试不需要摄像头，业务代码也不存在"测试模式"分支。

## 线程安全契约

### 1. 每个对象拥有什么资源，谁保证它活得足够久？

`OpenCvCamera` 拥有 `cv::VideoCapture`，构造时打开、析构时释放；`CameraSession` 以 `unique_ptr<FrameSource>` 独占来源，并拥有工作线程、互斥量、条件变量和有界缓冲；`PreviewApplication` 只持有 `CameraSession&` 引用，不拥有资源。`app/main.cpp` 按 `source → session → app` 顺序声明，C++ 逆序析构保证 app 先销毁、session 再 stop+join 并释放来源、source 最后销毁，任何对象都活得比引用它的人久。

### 2. CameraSession 有哪些状态，哪些转换是合法的？

状态由 `running_`、`stop_requested_` 与 `worker_` 是否可 join 共同刻画，分 idle、running、stopping 三种。合法转换：idle 仅经 `start()` 进入 running；running 经 `stop()` 或读失败/空帧/异常进入 stopping；stopping 在线程退出并被 join 后回到 idle。`start()` 在 running 或 stopping 时抛 `std::logic_error`，其余转换一律禁止。

### 3. 哪些公共方法允许被不同线程同时调用？

`start()` 与 `wait()` 由控制线程（主线程）配对调用，管理生命周期；`stop()`、`is_running()`、`try_latest_frame()` 线程安全，可被任意线程同时调用。其中 `stop()` 只置标志并 notify、不阻塞；`try_latest_frame()` 只在锁内短促读写缓冲，因此并发调用不会破坏任何不变量。

### 4. mutex 保护哪些成员以及什么不变量？

`mutex_` 保护 `worker_`、`error_`、`frames_`、`stop_requested_`、`running_` 五个成员。不变量：`frames_.size()` 恒不大于 2；`running_` 为真当且仅当 `worker_.joinable()`；线程退出时 `running_` 与 `error_` 在同一临界区内更新，外部永远观察不到中间态；`stop_requested_` 一旦置真，除重新 `start()` 外不再复位。

### 5. 缓冲区满时具体丢弃哪一帧，为什么？

丢弃最旧的一帧（队首 `pop_front`），保留最新。摄像头是实时数据源，显示一旦落后，历史帧已经没有价值，及时丢弃最旧帧才能让消费者始终拿到最新画面，同时把内存严格控制在两帧以内，队列不会无限增长。

### 6. 为什么不能持锁调用 VideoCapture::read()、imshow() 或 imwrite()？

三者都是慢速 I/O：`read()` 会阻塞等待下一帧，`imshow()`/`imwrite()` 涉及窗口系统与磁盘。若在持有 `mutex_` 时调用，`stop()`、`try_latest_frame()` 会被长时间阻塞，甚至因锁顺序造成死锁。因此采集线程在锁外 `read()`，主线程在锁外显示与写盘，锁只覆盖共享状态的短促转换。

### 7. 后台异常通过什么机制越过线程边界？

后台线程 `run()` 用 `try/catch` 捕获异常，调用 `std::current_exception()` 把 `std::exception_ptr` 存入受锁保护的 `error_`，随后线程正常退出。主线程调用 `wait()` 时取出 `error_` 并 `std::rethrow_exception()`，从而在调用线程重新抛出原始异常对象与消息，而不是让线程因未捕获异常而 `terminate`。

### 8. 析构时执行 stop()、唤醒和 join() 的顺序是什么？

析构函数先 `stop()`：在锁内置 `stop_requested_` 后 `notify_all()` 唤醒可能阻塞的等待路径；再 `wait()`：把 `worker_` 移出临界区后 `join()`，确认线程完全退出；之后成员按逆序析构，`FrameSource` 最后释放。`wait()` 中可能抛出的后台异常在析构内 `catch(...)` 吞掉，保证析构不向外抛错。

## 课上个人总结——工程要求核对

### 1. 单向依赖与领域隔离

**要求**：工程分层的目的是建立单向依赖，每个领域的参数不被跨领域调用和阅读。

**完成**：依赖链严格单向 `app → PreviewApplication → CameraSession → FrameSource`。参数解析 `Options` 只在 app 内部；设备参数 device/width/height 在 `OpenCvCamera` 构造边界被消费；`CameraSession` 只认 `FrameSource` 接口、不知道设备细节；`PreviewApplication` 只走会话公共接口，不读取任何内部状态。

### 2. CMake target 携带依赖，不做全局抛洒

**要求**：CMake target 进行编译构建，让 target 携带平台相关要求（不写死 CMake 里的路径）；依赖附着在真正需要的 target 上，不用全局 `include_directories` 和 `link_libraries` 把依赖抛洒到整个工程。

**完成**：用 `find_package(OpenCV/Threads)` 与 imported targets，路径不写死。依赖全部挂在 `camera_core` 上：头文件暴露的 `opencv_core`/`opencv_videoio`/`Threads::Threads` 为 PUBLIC，仅实现使用的 `opencv_highgui`/`imgcodecs`/`imgproc` 为 PRIVATE。全程 `target_include_directories`/`target_link_libraries`，无任何全局 include/link。

### 3. 封装与不变量

**要求**：对象要能拒绝非法请求，调用者不应重复记忆规则（范围检查集中在对象边界，一旦构造成功即相信合法）；每次 add 保持按 offset 排序；一个公开方法返回后对象必须仍满足所有安全承诺。

**完成**：`OpenCvCamera` 构造集中校验参数范围与设备打开，构造成功即可信任；`CameraSession::start()` 在运行中抛 `logic_error` 拒绝非法状态；帧缓冲在入队处强制 `size ≤ 2`。每个公开方法返回时锁已释放、不变量成立，调用者无需额外"记得"约束。（offset 排序属 midi 的事件序列，camera 无事件概念，对应承诺是缓冲有界。）

### 4. 继承、生命周期与多态分工

**要求**：继承用于替换外部能力；生命周期与析构顺序符合逻辑；多态只解决调用哪个实现，不解决线程安全和所有权。

**完成**：`FrameSource` 抽象让 `OpenCvCamera`（真实设备）与 `FakeFrameSource`（测试）替换外部能力；app 按 `source → session → app` 声明，逆序析构顺序正确；`CameraSession` 经 `FrameSource` 接口多态调用 `read()`，而线程与所有权由自身的 `unique_ptr`/`mutex`/`thread` 负责，与多态无关。

### 5. 线程类：状态机、单 worker 与不可复制

**要求**：先定义状态与合法转换；同一时刻只能有一个 worker；`stop()` 不是杀死线程，而是改变状态变量提示结束（notify 与 stop request）；线程、互斥量、外部借用不可被复制。

**完成**：状态机 `idle → running → stopping → idle` 在契约中明确定义；`start()` 检查 `running_` 保证单一 worker；`stop()` 只置 `stop_requested_` 并 `notify_all()`，由线程自行观察到标志后退出；`CameraSession` 删除拷贝构造/赋值，`mutex`/`thread` 成员使移动也隐式删除，会话唯一。

### 6. 不用 sleep 调度

**要求**：不要用 sleep 实现调度（否则 stop 析构可能被迫等完整个时间）；采用 wait_until，用 steady_clock 参考相对时间。

**完成**：生产代码 `src/` 中没有任何 sleep，不存在 sleep 导致析构被迫等待的问题。节奏由 `read()` 的硬件 I/O 阻塞与 `waitKey` 驱动；FPS 统计用 `steady_clock` 相对时间。`wait_until` 是 midi 定时音符调度所用，camera 无定时事件，故无对应场景，也未用 sleep 替代。

### 7. 后台异常与析构顺序

**要求**：后台异常不可直接跨线程，析构也不能把错误再抛出去（worker 必须捕获异常）；析构函数先调用 stop 再调用 wait。

**完成**：`run()` 内 `try/catch` 捕获异常存入 `exception_ptr`，避免线程 `terminate`；异常经 `wait()` rethrow 到调用线程，不"直接"跨线程。析构严格先 `stop()` 再 `wait()`，`wait()` 可能抛出的异常在析构内 `catch(...)` 吞掉，保证析构不向外抛错。
