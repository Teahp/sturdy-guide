#pragma once

#include <opencv2/core.hpp>

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

namespace camera {

class CameraSession;

// 主线程应用：负责窗口、键盘输入、画面叠加和截图。
// 它只通过 CameraSession 的公开接口取帧，不接触会话的 mutex 或工作线程。
class PreviewApplication {
 public:
  explicit PreviewApplication(std::unique_ptr<CameraSession> session,
                              std::string window_name = "Sturdy Guide Camera");
  ~PreviewApplication();

  PreviewApplication(const PreviewApplication&) = delete;
  PreviewApplication& operator=(const PreviewApplication&) = delete;

  // 运行主循环，返回进程退出码：0 正常退出，1 出错。
  // 设备打开失败等同步错误会以 std::runtime_error 抛出。
  int run();

 private:
  void showFrame(cv::Mat& frame, std::size_t sequence);
  bool shouldExit(int key) const;
  void saveScreenshot();

  std::unique_ptr<CameraSession> session_;
  std::string window_name_;

  cv::Mat last_shown_;  // 最近一次显示的帧（含叠加层），供截图使用
  std::size_t capture_number_ = 0;

  std::size_t frames_in_window_ = 0;
  double frames_per_second_ = 0.0;
  std::chrono::steady_clock::time_point rate_started_at_;
};

}  // namespace camera
