#include "camera/preview_application.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace camera {

namespace {
constexpr std::string_view kWindowName = "Sturdy Guide Camera";
}  // namespace

PreviewApplication::PreviewApplication(CameraSession& session)
    : session_(session) {}

int PreviewApplication::run() {
  if (!session_.start()) {
    report_error(session_.error_message());
    return 1;
  }

  std::size_t frame_number = 0;
  std::size_t frames_in_window = 0;
  double frames_per_second = 0.0;
  auto rate_started_at = std::chrono::steady_clock::now();

  cv::Mat frame;  // 最近一次显示的帧；按 S 时保存它
  bool window_created = false;
  int exit_code = 0;
  constexpr std::chrono::milliseconds frame_timeout{16};

  for (;;) {
    // 有帧就显示；没有就继续处理按键，UI 不被采集速度阻塞。
    if (session_.wait_for_frame(frame, frame_timeout)) {
      if (!window_created) {
        cv::namedWindow(std::string{kWindowName}, cv::WINDOW_NORMAL);
        window_created = true;
      }
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
      cv::imshow(std::string{kWindowName}, frame);
    }

    const int key = cv::waitKey(1) & 0xFF;
    double visibility = 1.0;
    if (window_created) {
      visibility = cv::getWindowProperty(std::string{kWindowName},
                                         cv::WND_PROP_VISIBLE);
    }
    // GTK 等后端可能以负值表示不支持可见性查询，不能把它当作已关闭。
    if (key == 'q' || key == 'Q' ||
        (visibility >= 0.0 && visibility < 1.0)) {
      break;
    }

    if ((key == 's' || key == 'S') && !frame.empty()) {
      save_screenshot(frame);
    }

    if (session_.has_error()) {
      report_error(session_.error_message());
      exit_code = 1;  // 与起始版本一致：设备错误以非零退出码结束
      break;
    }
  }

  session_.stop();
  cv::destroyAllWindows();
  return exit_code;
}

void PreviewApplication::save_screenshot(const cv::Mat& frame) {
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

void PreviewApplication::report_error(const std::string& message) const {
  std::cerr << "camera: " << message << '\n';
}

}  // namespace camera
