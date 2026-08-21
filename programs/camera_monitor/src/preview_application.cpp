#include "camera/preview_application.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace sturdy_guide::camera {

PreviewApplication::PreviewApplication(
    std::unique_ptr<CameraSession> session, const std::string& window_name)
    : session_(std::move(session)), window_name_(window_name) {}

PreviewApplication::~PreviewApplication() {
  session_->stop();
  try {
    session_->join();
  } catch (...) {
  }
  cv::destroyAllWindows();
}

int PreviewApplication::run() {
  session_->start();

  cv::namedWindow(window_name_, cv::WINDOW_NORMAL);

  std::size_t frame_number = 0;
  std::size_t capture_number = 0;
  std::size_t frames_in_window = 0;
  double frames_per_second = 0.0;
  auto rate_started_at = std::chrono::steady_clock::now();

  try {
    for (;;) {
      // Check for background thread errors.
      if (auto err = session_->error()) {
        session_->stop();
        session_->join();
        std::rethrow_exception(err);
      }

      auto frame_opt = session_->latest_frame();
      if (!frame_opt.has_value()) {
        // No frame ready yet; yield to avoid busy-spinning and
        // still process UI events so the window stays responsive.
        cv::waitKey(1);
        continue;
      }

      cv::Mat frame = *frame_opt;
      ++frame_number;
      ++frames_in_window;

      const auto now = std::chrono::steady_clock::now();
      const auto rate_window = now - rate_started_at;
      if (rate_window >= std::chrono::seconds{1}) {
        const auto seconds =
            std::chrono::duration<double>{rate_window}.count();
        frames_per_second =
            static_cast<double>(frames_in_window) / seconds;
        frames_in_window = 0;
        rate_started_at = now;
      }

      std::ostringstream overlay;
      overlay << "frame " << frame_number << "  " << std::fixed
              << std::setprecision(1) << frames_per_second << " FPS";
      cv::putText(frame, overlay.str(), cv::Point{20, 36},
                  cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                  cv::LINE_AA);
      cv::imshow(window_name_, frame);

      const int key = cv::waitKey(1) & 0xFF;
      const double visibility =
          cv::getWindowProperty(window_name_, cv::WND_PROP_VISIBLE);
      if (key == 'q' || key == 'Q' ||
          (visibility >= 0.0 && visibility < 1.0)) {
        break;
      }

      if (key == 's' || key == 'S') {
        std::filesystem::create_directories("captures");
        std::filesystem::path filename;
        do {
          ++capture_number;
          filename = std::filesystem::path{"captures"} /
                     ("capture-" + std::to_string(capture_number) + ".png");
        } while (std::filesystem::exists(filename));
        if (!cv::imwrite(filename.string(), frame)) {
          throw std::runtime_error("failed to save " + filename.string());
        }
        std::cout << "Saved " << filename << '\n';
      }
    }
  } catch (...) {
    session_->stop();
    session_->join();
    cv::destroyAllWindows();
    throw;
  }

  session_->stop();
  session_->join();
  cv::destroyAllWindows();
  return 0;
}

}  // namespace sturdy_guide::camera
