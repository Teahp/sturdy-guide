#pragma once

#include <opencv2/core.hpp>

namespace sturdy_guide::camera {

//抽象类 FrameSource 定义了一个接口，用于从摄像头或其他视频源读取帧。
//它提供了两个纯虚函数：read() 用于获取下一帧图像，返回一个 cv::Mat 对象；
//request_stop() 用于请求停止帧读取操作，允许实现类在后台线程中安全地终止读取过程。
//这个接口的设计使得不同的帧源实现可以互换使用，同时确保在应用程序关闭时，后台线程能够正确地退出，避免资源泄漏或未定义行为。

class FrameSource {
 public:
  virtual ~FrameSource() = default;//纯虚析构函数，

  virtual cv::Mat read() = 0;
  virtual void request_stop() noexcept = 0;
};

}  // namespace sturdy_guide::camera
