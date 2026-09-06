#include "preview_application.hpp"

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
#include <utility>

namespace sturdy_guide::camera {

PreviewApplication::PreviewApplication(CameraSession& session,
                                       const std::string_view window_name)
    : session_(session), window_name_(window_name) {}

int PreviewApplication::run() {
  cv::namedWindow(window_name_, cv::WINDOW_NORMAL);

  std::size_t frames_in_window = 0;
  double frames_per_second = 0.0;
  auto rate_started_at = std::chrono::steady_clock::now();
  cv::Mat last_displayed;  // 最近一次显示的画面，供 S 键截图。

  for (;;) {
    // 1) 从会话的有界缓冲取最新一帧；有则叠加并显示。
    //    取帧走会话公共接口（锁内搬走副本），imshow 在此处锁外执行。
    Frame frame;
    if (session_.take_latest(frame)) {
      ++frames_in_window;
      const auto now = std::chrono::steady_clock::now();
      const auto rate_window = now - rate_started_at;
      if (rate_window >= std::chrono::seconds{1}) {
        const double seconds =
            std::chrono::duration<double>{rate_window}.count();
        frames_per_second = static_cast<double>(frames_in_window) / seconds;
        frames_in_window = 0;
        rate_started_at = now;
      }

      std::ostringstream overlay;
      overlay << "frame " << frame.sequence << "  " << std::fixed
              << std::setprecision(1) << frames_per_second << " FPS";
      cv::putText(frame.image, overlay.str(), cv::Point{20, 36},
                  cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                  cv::LINE_AA);
      cv::imshow(window_name_, frame.image);
      last_displayed = frame.image;
    }

    // 2) 后台采集出错？打印给用户并结束循环。
    if (const std::exception_ptr error = session_.take_error()) {
      report_capture_error(error);
      break;
    }

    // 3) 处理按键与窗口状态。
    const int key = cv::waitKey(1) & 0xFF;
    const double visibility =
        cv::getWindowProperty(window_name_, cv::WND_PROP_VISIBLE);
    // GTK 等后端可能以负值表示不支持可见性查询，不能把它当作已关闭。
    if (key == 'q' || key == 'Q' ||
        (visibility >= 0.0 && visibility < 1.0)) {
      break;  // Q / 关闭窗口退出。
    }
    if (key == 's' || key == 'S') {
      save_screenshot(last_displayed);
    }
  }

  cv::destroyAllWindows();
  return 0;
}

void PreviewApplication::report_capture_error(
    const std::exception_ptr& error) const {
  try {
    std::rethrow_exception(error);
  } catch (const std::exception& reported) {
    std::cerr << "camera: " << reported.what() << '\n';
  }
}

void PreviewApplication::save_screenshot(const cv::Mat& frame) {
  if (frame.empty()) {
    return;  // 还没有可显示的画面，忽略 S。
  }
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

}  // namespace sturdy_guide::camera
