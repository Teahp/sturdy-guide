# WSL 下完成摄像头重构作业说明

这份实现把原来集中在 `main.cpp` 里的摄像头读取、UI 显示、状态管理和错误处理拆成了几个工程组件：

- `FrameSource`：帧输入接口，用来隔离真实摄像头和测试假源。
- `OpenCvCamera`：真实 OpenCV 摄像头实现，内部独占 `cv::VideoCapture`。
- `FakeFrameSource`：测试替身，可以提供正常帧、空帧和读取异常。
- `CameraSession`：异步采集组件，负责 `start / stop / join`、后台线程、两帧缓冲和异常回传。
- `starter/main.cpp`：应用入口，只负责窗口显示、按键和保存图片。
- `tests/camera_session_test.cpp`：不依赖真实摄像头的测试程序。

## 1. WSL 安装依赖

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git pkg-config
sudo apt install -y libopencv-dev v4l-utils
```

检查 OpenCV：

```bash
pkg-config --modversion opencv4
```

检查 WSL 是否能看到摄像头：

```bash
ls -l /dev/video*
v4l2-ctl --list-devices
```

如果没有 `/dev/video0`，说明 WSL 当前没有拿到摄像头设备。这不影响核心重构和测试，真实摄像头预览可以在 Windows 原生环境或能访问摄像头的 Linux 环境里跑。

## 2. 编译

在本目录执行：

```bash
cmake -S . -B build-camera-wsl -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-camera-wsl --parallel
```

如果要合并到课程原仓库，并且课程要求打开开关：

```bash
cmake -S . -B build-camera-wsl -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON

cmake --build build-camera-wsl --parallel
```

## 3. 运行测试

```bash
ctest --test-dir build-camera-wsl --output-on-failure
```

也可以直接运行：

```bash
./build-camera-wsl/camera_monitor_test
```

测试覆盖了：

- 后台线程能从 `FakeFrameSource` 读取帧。
- 空帧不会进入显示缓冲。
- 缓冲区最多保留两帧。
- 后台读取异常会在 `join()` 中重新抛出。
- `stop()` 可以重复调用，线程能正常收尾。

## 4. 运行真实摄像头

如果 WSL 中存在 `/dev/video0`：

```bash
./build-camera-wsl/camera_monitor --device 0
```

窗口中：

- 按 `s` 保存当前帧到 `captures/`。
- 按 `q` 退出程序。

如果窗口打不开，先检查 WSLg：

```bash
echo $DISPLAY
```

Windows 11 的 WSLg 通常可以显示 OpenCV 窗口；如果没有 GUI 能力，只能先跑测试，或者改成无窗口保存模式。

## 5. 合并到课程仓库时怎么放

如果课程仓库里已有 `programs/camera_monitor/`，可以按下面方式对应：

```text
programs/camera_monitor/
  include/camera_monitor/*.hpp
  src/*.cpp
  starter/main.cpp
  tests/camera_session_test.cpp
  CMakeLists.txt
```

如果课程仓库顶层已经有总 CMake，不一定要照搬这里的完整 `CMakeLists.txt`，但核心思想要保留：

- 一个核心库 target：`camera_monitor_core`
- 一个应用 target：`camera_monitor`
- 一个测试 target：`camera_monitor_test`
- OpenCV 和 Threads 依赖挂到真正需要它们的 target 上

## 6. 设计要点

`CameraSession` 的后台线程只负责采集帧，不做 UI：

- `source_.read()` 在锁外执行，避免摄像头读取阻塞 `stop()`。
- `cv::imshow()`、`cv::waitKey()` 和 `cv::imwrite()` 留在主线程或锁外执行。
- 共享状态只包括停止标志、运行状态、缓冲队列和异常指针。
- 缓冲队列最多两帧，主线程落后时丢弃旧帧，优先显示最新帧。
- 后台线程异常通过 `std::exception_ptr` 保存，再由 `join()` 在主线程重新抛出。

这正对应作业里的核心要求：工程化不是把 `main.cpp` 平均切成几个文件，而是让资源所有权、并发状态、错误出口和测试替换点都成为可检查的契约。
