#pragma once

#include "camera/camera_session.hpp"

#include <cstddef>

namespace sturdy_guide::camera {

// Window and file I/O live here, on the thread that calls run().
class PreviewApplication {
 public:
  explicit PreviewApplication(CameraSession& session);//构造函数，传入一个 CameraSession 的引用，表示预览应用程序将使用该会话来获取摄像头捕获的帧。

  int run();//运行预览应用程序，显示摄像头捕获的帧，并允许用户保存帧或退出应用程序。返回值为整数，表示应用程序的退出状态。

 private:
  CameraSession& session_;
  std::size_t capture_number_ = 0;
};

}  // namespace sturdy_guide::camera
