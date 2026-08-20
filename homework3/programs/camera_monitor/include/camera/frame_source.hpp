#pragma once

#include <opencv2/core.hpp>

namespace sturdy_guide::camera {

// 抽象帧来源，隔离 CameraSession 与具体设备。
// read() 返回空 Mat 表示来源不再产生有效帧；抛异常表示设备或读取错误。
// 实现对象必须比调用它的 CameraSession 活得更久（通常由 CameraSession 独占）。
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  virtual cv::Mat read() = 0;
};

}  // namespace sturdy_guide::camera
