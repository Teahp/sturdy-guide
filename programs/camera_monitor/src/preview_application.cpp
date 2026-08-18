#include "camera/preview_application.hpp"

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
    : session_{session} {}

void PreviewApplication::run() {
  cv::Mat last_displayed;

  try {
    session_.start();
    cv::namedWindow(window_name_, cv::WINDOW_NORMAL);

    for (;;) {
      auto captured = session_.waitForFrame(std::chrono::milliseconds{50});
      if (captured) {
        cv::Mat display = captured->image.clone();
        updateRate(std::chrono::steady_clock::now());
        drawOverlay(display, captured->sequence);
        cv::imshow(window_name_, display);
        last_displayed = std::move(display);
      }

      if (const auto error = session_.errorMessage()) {
        throw std::runtime_error(*error);
      }

      const int key = cv::waitKey(1) & 0xFF;
      if (key == 'q' || key == 'Q' || windowClosed()) {
        break;
      }
      if ((key == 's' || key == 'S') && !last_displayed.empty()) {
        saveScreenshot(last_displayed);
      }

      if (!captured && !session_.running()) {
        break;
      }
    }
  } catch (...) {
    session_.stop();
    cv::destroyAllWindows();
    throw;
  }

  session_.stop();
  cv::destroyAllWindows();
}

void PreviewApplication::updateRate(
    const std::chrono::steady_clock::time_point now) {
  ++displayed_in_window_;
  const auto window = now - rate_started_at_;
  if (window >= std::chrono::seconds{1}) {
    const auto seconds = std::chrono::duration<double>{window}.count();
    frames_per_second_ = static_cast<double>(displayed_in_window_) / seconds;
    displayed_in_window_ = 0;
    rate_started_at_ = now;
  }
}

void PreviewApplication::drawOverlay(cv::Mat& frame,
                                     const std::size_t sequence) const {
  std::ostringstream overlay;
  overlay << "frame " << sequence << "  " << std::fixed
          << std::setprecision(1) << frames_per_second_ << " FPS";
  cv::putText(frame, overlay.str(), cv::Point{20, 36},
              cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
              cv::LINE_AA);
}

void PreviewApplication::saveScreenshot(const cv::Mat& frame) {
  std::filesystem::create_directories("captures");
  std::filesystem::path filename;
  do {
    ++capture_number_;
    filename = std::filesystem::path{"captures"} /
               ("capture-" + std::to_string(capture_number_) + ".png");
  } while (std::filesystem::exists(filename));

  if (!cv::imwrite(filename.string(), frame)) {
    throw std::runtime_error("failed to save " + filename.string());
  }
  std::cout << "Saved " << filename << '\n';
}

bool PreviewApplication::windowClosed() const {
  const double visibility =
      cv::getWindowProperty(window_name_, cv::WND_PROP_VISIBLE);
  return visibility >= 0.0 && visibility < 1.0;
}

}  // namespace camera
