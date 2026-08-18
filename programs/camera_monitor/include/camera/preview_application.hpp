#ifndef CAMERA_PREVIEW_APPLICATION_HPP
#define CAMERA_PREVIEW_APPLICATION_HPP

#include "camera/camera_session.hpp"

#include <cstddef>
#include <opencv2/core.hpp>
#include <string>

namespace camera {

// 主线程应用：只负责窗口、键盘、画面叠加和截图。
// 不直接访问 CameraSession 的 mutex 或工作线程，只调用其公开方法。
class PreviewApplication {
 public:
  explicit PreviewApplication(CameraSession& session);

  // 运行窗口循环直到退出。返回进程退出码（0 正常，1 出错）。
  int run();

 private:
  void save_screenshot(const cv::Mat& frame);
  void report_error(const std::string& message) const;

  CameraSession& session_;
  std::size_t capture_number_ = 0;
};

}  // namespace camera

#endif  // CAMERA_PREVIEW_APPLICATION_HPP
