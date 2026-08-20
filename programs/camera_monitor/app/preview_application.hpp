#pragma once

#include "camera/camera_session.hpp"

#include <chrono>
#include <cstddef>
#include <string>

namespace camera {

// Main-thread UI: owns the window, keyboard input, overlay drawing and
// screenshot saving. It never touches CameraSession's mutex or worker thread
// directly — it only calls the public frame/error API.
class PreviewApplication {
 public:
  explicit PreviewApplication(CameraSession& session);

  // Runs the window loop until the user presses Q, closes the window, or the
  // session ends. Returns the process exit code.
  int run();

 private:
  void draw_overlay(cv::Mat& frame, std::size_t frame_number, double fps);
  void save_capture(const cv::Mat& frame);
  void report_error();

  CameraSession& session_;

  std::size_t frame_number_ = 0;
  std::size_t capture_number_ = 0;
  std::size_t frames_in_window_ = 0;
  double frames_per_second_ = 0.0;
  std::chrono::steady_clock::time_point rate_started_at_;
};

}  // namespace camera
