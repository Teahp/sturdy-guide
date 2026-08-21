#pragma once

#include "camera/camera_session.hpp"

#include <cstddef>
#include <memory>

namespace sturdy_guide::camera {

// PreviewApplication runs the main-thread UI loop: rendering frames,
// handling key input, computing FPS, and saving screenshots.
//
// It does not perform any blocking I/O itself; all capture work is
// delegated to CameraSession on a background thread.
class PreviewApplication {
 public:
  PreviewApplication(std::unique_ptr<CameraSession> session,
                     const std::string& window_name);
  ~PreviewApplication();

  PreviewApplication(const PreviewApplication&) = delete;
  PreviewApplication& operator=(const PreviewApplication&) = delete;

  // Enters the main event loop. Returns 0 on clean exit, 1 on error.
  [[nodiscard]] int run();

 private:
  std::unique_ptr<CameraSession> session_;
  std::string window_name_;
};

}  // namespace sturdy_guide::camera
