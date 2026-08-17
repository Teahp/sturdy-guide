#pragma once

#include <opencv2/core.hpp>

namespace camera {

// 设备抽象：真实摄像头与测试假设备实现同一接口。
//
// CameraSession 通过 std::unique_ptr<FrameSource> 独占一个设备，
// 业务代码不需要知道帧来自 cv::VideoCapture 还是测试替身。
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  // 打开设备。失败时抛出 std::runtime_error。
  // 该调用发生在 start() 的调用线程上，异常同步传播给调用者。
  virtual void open() = 0;

  // 读取下一帧。返回 true 时 frame 必须是有效（非空）画面；
  // 返回 false 表示设备不再返回有效帧（如断开、权限失效）。
  // 实现也可以直接抛出异常；CameraSession 会捕获并传回调用线程。
  virtual bool read(cv::Mat& frame) = 0;

  // 请求的分辨率，用于打开设备时设置。
  virtual int width() const = 0;
  virtual int height() const = 0;
};

}  // namespace camera
