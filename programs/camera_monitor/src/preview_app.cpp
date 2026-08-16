#include "camera/preview_app.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace camera {

PreviewApplication::PreviewApplication(CameraSession& session)
    : session_(session) {}

PreviewApplication::~PreviewApplication() = default;

int PreviewApplication::run(const std::string& window_name) {
  cv::namedWindow(window_name, cv::WINDOW_NORMAL);

  double frames_per_second = 0.0;
  std::size_t frames_in_window = 0;
  auto rate_started_at = std::chrono::steady_clock::now();

  cv::Mat last_frame;  // 供 S 键截图使用；仅在取到帧时更新
  for (;;) {
    cv::Mat frame;
    if (session_.wait_frame(frame, std::chrono::milliseconds{10})) {
      last_frame = frame;
      ++frames_in_window;

      const auto now = std::chrono::steady_clock::now();
      const auto elapsed = now - rate_started_at;
      if (elapsed >= std::chrono::seconds{1}) {
        frames_per_second =
            static_cast<double>(frames_in_window) /
            std::chrono::duration<double>{elapsed}.count();
        frames_in_window = 0;
        rate_started_at = now;
      }

      std::ostringstream overlay;
      overlay << "frame " << session_.frame_count() << "  " << std::fixed
              << std::setprecision(1) << frames_per_second << " FPS";
      cv::putText(frame, overlay.str(), cv::Point{20, 36},
                  cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                  cv::LINE_AA);
      cv::imshow(window_name, frame);
    }

    // 后台异常：在主线程观察并报告（take_error 取出后清除，可重入）。
    if (const std::exception_ptr error = session_.take_error()) {
      try {
        std::rethrow_exception(error);
      } catch (const std::exception& e) {
        std::cerr << "camera: " << e.what() << '\n';
      }
      cv::destroyAllWindows();
      return 1;
    }

    const int key = cv::waitKey(1) & 0xFF;
    const double visibility =
        cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE);
    // GTK 等后端可能以负值表示不支持可见性查询，不能把它当作已关闭。
    if (key == 'q' || key == 'Q' ||
        (visibility >= 0.0 && visibility < 1.0)) {
      break;
    }

    if (key == 's' || key == 'S') {
      save_screenshot(last_frame);
    }
  }

  cv::destroyAllWindows();
  return 0;
}

void PreviewApplication::save_screenshot(const cv::Mat& frame) {
  if (frame.empty()) {
    std::cerr << "camera: no frame available to save\n";
    return;
  }
  std::filesystem::create_directories("captures");
  std::filesystem::path filename;
  std::size_t number = 0;
  do {
    ++number;
    filename =
        std::filesystem::path{"captures"} /
        ("capture-" + std::to_string(number) + ".png");
  } while (std::filesystem::exists(filename));
  if (!cv::imwrite(filename.string(), frame)) {
    throw std::runtime_error("failed to save " + filename.string());
  }
  std::cout << "Saved " << filename << '\n';
}

}  // namespace camera
