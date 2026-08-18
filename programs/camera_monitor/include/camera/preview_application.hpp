#pragma once

#include "camera/camera_session.hpp"

#include <chrono>
#include <cstddef>
#include <string>

namespace camera {

class PreviewApplication {
 public:
  explicit PreviewApplication(CameraSession& session);

  void run();

 private:
  void updateRate(std::chrono::steady_clock::time_point now);
  void drawOverlay(cv::Mat& frame, std::size_t sequence) const;
  void saveScreenshot(const cv::Mat& frame);
  bool windowClosed() const;

  CameraSession& session_;
  std::string window_name_{"Sturdy Guide Camera"};
  std::size_t capture_number_{0};
  std::size_t displayed_in_window_{0};
  double frames_per_second_{0.0};
  std::chrono::steady_clock::time_point rate_started_at_{
      std::chrono::steady_clock::now()};
};

}  // namespace camera
