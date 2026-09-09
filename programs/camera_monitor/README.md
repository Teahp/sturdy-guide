# Camera Monitor - 多线程摄像头程序

## 概述

本项目将一个单线程摄像头程序重构为封装良好、可测试的多线程程序。通过抽象接口、会话管理和 UI 分离，实现了清晰的职责边界和线程安全。

## 构建

从仓库根目录：

    cmake -S . -B build-camera -G Ninja -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON -DBUILD_TESTING=ON
    cmake --build build-camera

## 运行

    ./build-camera/programs/camera_monitor/sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]

按键：S 保存截图，Q 退出程序

## 测试

    cd build-camera
    ctest --output-on-failure

## 架构设计

FrameSource (抽象接口)
- OpenCvCamera：使用 cv::VideoCapture 访问真实设备
- FakeFrameSource：测试时产生确定的假帧或模拟失败

CameraSession
- 独占一个 FrameSource
- 管理 start / stop / join 状态机
- 维护有界的最新帧缓冲区（双缓冲）
- 把后台异常传回调用线程

PreviewApplication
- 在主线程调用 cv::imshow / cv::waitKey
- 处理截图、退出和用户可见错误
- 不直接访问 CameraSession 的 mutex 或工作线程

## 线程安全契约

### 1. 资源所有权
- FrameSource：由 CameraSession 通过 unique_ptr 独占
- CameraSession：由 PreviewApplication 通过 unique_ptr 独占
- 工作线程：由 CameraSession 管理，stop() 和析构确保 join()
- 帧缓冲区：CameraSession 内部，由 mutex 保护

### 2. 状态机
Idle <-> Running
- Idle -> Running：调用 start()
- Running -> Idle：调用 stop()
- 重复调用 start()/stop() 被忽略（幂等）

### 3. 线程安全方法
start()、stop()、tryGetLatestFrame()、hasError()、getErrorMessage()、isRunning() 都可以从任何线程调用

### 4. mutex 保护
buffers_、write_index_、has_new_frame_、frame_sequence_、worker_exception_、error_message_

### 5. 丢帧策略
双缓冲，丢弃最旧的帧，保留最新的帧

### 6. 不持锁 I/O
VideoCapture::read()、imshow()、imwrite() 都是慢速 I/O，持锁会阻塞其他线程

### 7. 异常传递
使用 std::exception_ptr 保存工作线程异常，主线程通过 hasError()/getErrorMessage() 获取

### 8. 析构顺序
1. 设置停止标志
2. 唤醒工作线程
3. join() 等待线程结束
4. 更新状态

## 测试结果

WSL2 环境（无摄像头）下，所有硬件无关测试通过（8 个测试用例）

## 真机测试

需要在原生 Linux 环境（有摄像头）下进行
