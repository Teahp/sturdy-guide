#include "camera/preview_application.hpp"

#include "camera/camera_session.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace camera {

namespace {
constexpr std::chrono::milliseconds kFramePollInterval{16};
}  // namespace

PreviewApplication::PreviewApplication(
    std::unique_ptr<CameraSession> session, std::string window_name)
    : session_(std::move(session)), window_name_(std::move(window_name)) {}

PreviewApplication::~PreviewApplication() = default;

int PreviewApplication::run() {
  // 与起始版本一致：先打开设备，再创建窗口；打开失败时不会留下空窗口。
  session_->start();

  cv::namedWindow(window_name_, cv::WINDOW_NORMAL);
  rate_started_at_ = std::chrono::steady_clock::now();

  int exit_code = 0;
  for (;;) {
    cv::Mat frame;
    std::size_t sequence = 0;
    if (session_->waitForFrame(frame, kFramePollInterval, &sequence)) {
      showFrame(frame, sequence);
    }

    const int key = cv::waitKey(1) & 0xFF;
    if (shouldExit(key)) {
      break;
    }
    if (key == 's' || key == 'S') {
      saveScreenshot();
    }

    // 后台读取失败：在调用线程观察并报告。
    const std::string error = session_->error();
    if (!error.empty()) {
      std::cerr << "camera: " << error << '\n';
      exit_code = 1;
      break;
    }
  }

  session_->stop();
  cv::destroyAllWindows();
  return exit_code;
}

void PreviewApplication::showFrame(cv::Mat& frame,
                                   const std::size_t sequence) {
  ++frames_in_window_;
  const auto now = std::chrono::steady_clock::now();
  const auto rate_window = now - rate_started_at_;
  if (rate_window >= std::chrono::seconds{1}) {
    const auto seconds = std::chrono::duration<double>{rate_window}.count();
    frames_per_second_ = static_cast<double>(frames_in_window_) / seconds;
    frames_in_window_ = 0;
    rate_started_at_ = now;
  }

  std::ostringstream overlay;
  overlay << "frame " << sequence << "  " << std::fixed
          << std::setprecision(1) << frames_per_second_ << " FPS";
  cv::putText(frame, overlay.str(), cv::Point{20, 36},
              cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
              cv::LINE_AA);
  cv::imshow(window_name_, frame);
  last_shown_ = frame;  // 引用计数共享像素，截图时仍有效
}

bool PreviewApplication::shouldExit(const int key) const {
  if (key == 'q' || key == 'Q') {
    return true;
  }
  if (last_shown_.empty()) {
    return false;  // 窗口尚未显示过画面，跳过可见性检查
  }
  const double visibility = cv::getWindowProperty(
      window_name_, cv::WND_PROP_VISIBLE);
  // GTK 等后端可能以负值表示不支持可见性查询，不能把它当作已关闭。
  return visibility >= 0.0 && visibility < 1.0;
}

void PreviewApplication::saveScreenshot() {
  if (last_shown_.empty()) {
    return;  // 还没有显示过任何画面
  }
  std::filesystem::create_directories("captures");
  std::filesystem::path filename;
  do {
    ++capture_number_;
    filename = std::filesystem::path{"captures"} /
               ("capture-" + std::to_string(capture_number_) + ".png");
  } while (std::filesystem::exists(filename));
  if (!cv::imwrite(filename.string(), last_shown_)) {
    throw std::runtime_error("failed to save " + filename.string());
  }
  std::cout << "Saved " << filename << '\n';
}

}  // namespace camera
