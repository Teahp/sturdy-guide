#include "preview_application.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace camera {

PreviewApplication::PreviewApplication(CameraSession& session)
    : session_(session), rate_started_at_(std::chrono::steady_clock::now()) {}

int PreviewApplication::run() {
  constexpr std::string_view window_name = "Sturdy Guide Camera";
  cv::namedWindow(std::string{window_name}, cv::WINDOW_NORMAL);

  for (;;) {
    // The session never blocks on us: it drops old frames when we are slow.
    cv::Mat frame;
    const auto status =
        session_.wait_for_frame(frame, std::chrono::milliseconds{16});

    if (status == CameraSession::FrameStatus::NewFrame) {
      ++frame_number_;
      ++frames_in_window_;

      const auto now = std::chrono::steady_clock::now();
      const auto elapsed = now - rate_started_at_;
      if (elapsed >= std::chrono::seconds{1}) {
        frames_per_second_ = static_cast<double>(frames_in_window_) /
                             std::chrono::duration<double>{elapsed}.count();
        frames_in_window_ = 0;
        rate_started_at_ = now;
      }

      draw_overlay(frame, frame_number_, frames_per_second_);
      cv::imshow(std::string{window_name}, frame);  // Main-thread UI only.
    }

    const int key = cv::waitKey(1) & 0xFF;
    const double visibility =
        cv::getWindowProperty(std::string{window_name}, cv::WND_PROP_VISIBLE);

    if (key == 'q' || key == 'Q' ||
        (visibility >= 0.0 && visibility < 1.0)) {
      break;
    }

    if (key == 's' || key == 'S') {
      save_capture(frame);
    }

    if (status == CameraSession::FrameStatus::Ended) {
      // The worker stopped: either the user asked us to stop (handled above)
      // or the source failed. Report errors the user should see.
      if (session_.has_error()) {
        report_error();
      }
      break;
    }
  }

  session_.stop();
  cv::destroyAllWindows();
  return session_.has_error() ? 1 : 0;
}

void PreviewApplication::draw_overlay(cv::Mat& frame, std::size_t frame_number,
                                      double fps) {
  std::ostringstream overlay;
  overlay << "frame " << frame_number << "  " << std::fixed
          << std::setprecision(1) << fps << " FPS";
  cv::putText(frame, overlay.str(), cv::Point{20, 36},
              cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
              cv::LINE_AA);
}

void PreviewApplication::save_capture(const cv::Mat& frame) {
  if (frame.empty()) {
    return;
  }
  std::filesystem::create_directories("captures");
  std::filesystem::path filename;
  do {
    ++capture_number_;
    filename = std::filesystem::path{"captures"} /
               ("capture-" + std::to_string(capture_number_) + ".png");
  } while (std::filesystem::exists(filename));

  if (!cv::imwrite(filename.string(), frame)) {
    std::cerr << "camera: failed to save " << filename << '\n';
    return;
  }
  std::cout << "Saved " << filename << '\n';
}

void PreviewApplication::report_error() {
  try {
    if (std::exception_ptr error = session_.error(); error != nullptr) {
      std::rethrow_exception(error);
    }
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
  }
}

}  // namespace camera
