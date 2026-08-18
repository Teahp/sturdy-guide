# Camera Monitor Homework

This directory contains the refactored version of the camera homework. The
external behavior is kept from `starter/main.cpp`:

```text
sturdy-guide-camera [--device INDEX] [--width PIXELS] [--height PIXELS]
Keys: S saves a frame; Q exits.
```

The original single-file implementation remains in `starter/` for comparison.
It is not built by default. The final `sturdy-guide-camera` target is built
from `app/main.cpp` and links the camera core library.

## Build And Run

From the repository root:

```bash
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON
cmake --build build-camera
ctest --test-dir build-camera --output-on-failure
./build-camera/sturdy-guide-camera --device 0
```

To build the original starter for comparison:

```bash
cmake -S . -B build-camera -G Ninja \
  -DSTURDY_GUIDE_BUILD_CAMERA_HOMEWORK=ON \
  -DSTURDY_GUIDE_BUILD_CAMERA_STARTER=ON
```

The starter binary is named `sturdy-guide-camera-starter` so it does not
conflict with the refactored application.

## Structure

```text
camera_monitor/
├── starter/main.cpp        original single-threaded reference
├── include/camera/         public interfaces
├── src/                    camera core library implementation
├── app/main.cpp            CLI parsing and dependency assembly
├── tests/                  hardware-free CTest tests
└── CMakeLists.txt          OpenCV, Threads, and nested targets
```

`FrameSource` is the hardware boundary. `OpenCvCamera` owns a real
`cv::VideoCapture`, while `FakeFrameSource` supplies deterministic frames and
failures for tests.

`CameraSession` owns exactly one `FrameSource` through `std::unique_ptr`. It
starts a background worker that repeatedly reads frames outside the internal
mutex, publishes valid frames into a two-slot buffer, and records the first
background error for the main thread.

`PreviewApplication` stays on the main thread. It calls `imshow`, `waitKey`,
overlay drawing, and `imwrite`; it does not touch the session mutex or worker.

## Thread-Safety Contract

1. `CameraSession` owns the `FrameSource`; the source outlives the worker
   because `CameraSession::~CameraSession()` calls `stop()` and joins before
   member destruction.
2. The lifecycle states are `Idle`, `Running`, `Stopping`, and `Stopped`.
   `start()` moves an idle or stopped session to running. `stop()` is
   idempotent and moves a started session through stopping to stopped.
3. `tryTakeFrame()`, `waitForFrame()`, `running()`, `state()`,
   `errorMessage()`, and the counters are safe to call while the worker runs.
   Normal application code calls `start()` and `stop()` from the main thread;
   `stop()` is also safe to call repeatedly.
4. The session mutex protects lifecycle flags, the optional error message,
   the two-frame buffer, and frame counters. It does not cover camera reads,
   `imshow`, or `imwrite`.
5. The buffer capacity is two frames. When a new frame arrives while full, the
   oldest buffered frame is discarded and `droppedFrames()` increases. The
   consumer takes the newest frame and clears stale buffered frames to keep
   preview latency low.
6. Slow I/O is outside the lock. `VideoCapture::read()` can block on hardware,
   and `imshow()` or `imwrite()` can block on the window system or disk. Holding
   the mutex across those calls would serialize the UI with capture and make
   shutdown harder to reason about.
7. Background failures do not throw across threads. The worker catches
   exceptions or empty frames, stores the first message in `errorMessage()`,
   requests stop, wakes waiters, and exits.
8. `stop()` first marks `stop_requested_`, then notifies waiters, joins the
   worker outside the session mutex, closes the source, and finally records the
   stopped state. Calling `stop()` again repeats the same safe no-op path.

## Tests

`camera_session_test` uses `FakeFrameSource` only. It verifies:

- start publishes sequential frames;
- repeated `start()` while running is idempotent;
- repeated `stop()` is safe;
- destruction stops the worker and destroys the source;
- slow consumers keep at most two frames and drop old frames;
- fake read failures are visible on the main thread;
- empty frames are never published;
- restarting after stop resets per-run sequence numbers.

真机截图需要在可访问摄像头的 Linux 环境中补充到 PR。WSL2 是否能访问
`/dev/video*` 取决于宿主机设备转发配置，因此自动测试不依赖真实摄像头。
