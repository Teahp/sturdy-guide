#pragma once

#include "camera/camera_session.hpp"

#include <string>

namespace sturdy_guide::camera {

// 在主线程运行 OpenCV 窗口事件循环：取帧、叠加信息、imshow、
// 处理截图与退出，并把后台读取异常报告给调用者。
// 只通过 CameraSession 的公共接口访问，不触碰其 mutex 或工作线程。
class PreviewApplication {
 public:
  PreviewApplication(std::string window_name, CameraSession& session);

  PreviewApplication(const PreviewApplication&) = delete;
  PreviewApplication& operator=(const PreviewApplication&) = delete;

  // 运行事件循环，直到退出条件满足（Q / 窗口关闭 / 后台停止或异常）。
  // 后台读取异常在此重新抛出。
  void run();

 private:
  std::string window_name_;
  CameraSession& session_;
};

}  // namespace sturdy_guide::camera
