// PreviewApplication：主线程 UI 循环。
// 只通过 CameraSession 的公共 API 工作，不接触其 mutex 或工作线程。
// 职责：imshow / waitKey、画面叠加（帧号 + FPS）、截图、用户可见错误报告。
#pragma once

#include "camera/camera_session.h"

#include <string>

namespace camera {

class PreviewApplication {
 public:
  explicit PreviewApplication(CameraSession& session);
  ~PreviewApplication();

  PreviewApplication(const PreviewApplication&) = delete;
  PreviewApplication& operator=(const PreviewApplication&) = delete;

  // 阻塞主线程运行预览循环，直到 Q / 关闭窗口 / 后台错误退出。
  // 返回进程退出码：0 正常退出，1 出错。
  int run(const std::string& window_name);

 private:
  void save_screenshot(const cv::Mat& frame);  // 仅在主线程调用

  CameraSession& session_;
};

}  // namespace camera
