#pragma once

#include "camera/camera_session.hpp"
#include "camera/frame.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace camera {

// 主线程侧的预览应用：窗口、键盘、叠加文字和截图全部在调用线程完成。
//
// 它不访问 CameraSession 的 mutex，也不接触工作线程，只通过公共方法取帧和读统计；
// cv::imshow() 与 cv::imwrite() 都在任何会话锁之外执行。
class PreviewApplication {
 public:
  struct Options {
    std::string window_title = "Sturdy Guide Camera";
    std::filesystem::path capture_directory = "captures";
    // 没有新帧时最多等待多久再处理按键，保证窗口不会因为掉帧而卡死。
    std::chrono::milliseconds frame_timeout{50};
  };

  PreviewApplication(std::unique_ptr<CameraSession> session, Options options);
  ~PreviewApplication();

  PreviewApplication(const PreviewApplication&) = delete;
  PreviewApplication& operator=(const PreviewApplication&) = delete;

  // 启动采集并运行窗口循环，返回进程退出码。
  int run();

 private:
  // 返回进程退出码：0 表示正常退出。
  int event_loop();

  void update_display_rate();
  void draw_overlay(Frame& frame) const;
  void save_capture(const Frame& frame) const;
  [[nodiscard]] bool window_closed() const;

  std::unique_ptr<CameraSession> session_;
  const Options options_;

  std::size_t capture_number_ = 0;
  std::size_t frames_in_window_ = 0;
  double display_fps_ = 0.0;
  std::chrono::steady_clock::time_point rate_started_at_{};
};

}  // namespace camera
