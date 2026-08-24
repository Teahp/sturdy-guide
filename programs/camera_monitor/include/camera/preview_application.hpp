#pragma once

#include "camera/camera_session.hpp"
#include "camera/frame.hpp"

#include <cstddef>
#include <string>

namespace sturdy_guide::camera {

// 主线程 UI：负责窗口、键盘、画面叠加与截图。
// 只通过 CameraSession 的公开接口取帧，不接触其 mutex 或工作线程。
class PreviewApplication {
 public:
  explicit PreviewApplication(CameraSession& session,
                              std::string window_name = "Sturdy Guide Camera");

  // 运行窗口循环，直到 Q / 窗口关闭，返回 0；后台错误以异常抛出。
  int run();

 private:
  void draw_overlay(Frame& frame) const;
  void save_screenshot(const Frame& frame);

  CameraSession& session_;
  std::string window_name_;
  std::size_t capture_number_ = 0;
};

}  // namespace sturdy_guide::camera
