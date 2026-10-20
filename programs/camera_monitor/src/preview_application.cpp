#include "camera/preview_application.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

namespace camera {
namespace {

using Clock = std::chrono::steady_clock;

// 把跨线程传来的异常还原成一行 stderr 输出。
void report(const std::exception_ptr& error) {
  if (error == nullptr) {
    return;
  }
  try {
    std::rethrow_exception(error);
  } catch (const std::exception& exception) {
    std::cerr << "camera: " << exception.what() << '\n';
  } catch (...) {
    std::cerr << "camera: unknown background error\n";
  }
}

}  // namespace

PreviewApplication::PreviewApplication(std::unique_ptr<CameraSession> session,
                                       Options options)
    : session_(std::move(session)), options_(std::move(options)) {
  if (session_ == nullptr) {
    throw std::invalid_argument("preview application requires a camera session");
  }
}

PreviewApplication::~PreviewApplication() {
  if (session_ != nullptr) {
    session_->stop();
  }
}

int PreviewApplication::run() {
  int exit_code = 0;
  try {
    if (!session_->start()) {
      std::cerr << "camera: cannot open " << session_->description()
                << "; check the device index, permissions, and whether another "
                   "program is using it\n";
      return 1;
    }

    cv::namedWindow(options_.window_title, cv::WINDOW_NORMAL);
    rate_started_at_ = Clock::now();
    exit_code = event_loop();

    // 先停采集再取异常：确保 join 之后不会再有新的错误写入。
    session_->stop();
    if (const std::exception_ptr error = session_->take_error()) {
      report(error);
      exit_code = 1;
    }
  } catch (const std::exception& exception) {
    session_->stop();
    std::cerr << "camera: " << exception.what() << '\n';
    exit_code = 1;
  }

  cv::destroyAllWindows();
  return exit_code;
}

int PreviewApplication::event_loop() {
  for (;;) {
    Frame frame;
    if (!session_->wait_for_frame(frame, options_.frame_timeout)) {
      if (session_->has_error()) {
        return 1;
      }

      // 没有新帧时仍然要调用 waitKey()，否则窗口会失去响应。
      const int idle_key = cv::waitKey(1) & 0xFF;
      if (idle_key == 'q' || idle_key == 'Q' || window_closed()) {
        return 0;
      }
      if (!session_->is_running()) {
        return 0;
      }
      continue;
    }

    ++frames_in_window_;
    update_display_rate();
    draw_overlay(frame);

    // imshow 在任何会话锁之外执行：显示卡顿不会阻塞采集线程。
    cv::imshow(options_.window_title, frame.image);

    const int key = cv::waitKey(1) & 0xFF;
    // GTK 等后端可能用负值表示"不支持可见性查询"，不能把它当成已关闭。
    if (key == 'q' || key == 'Q' || window_closed()) {
      return 0;
    }
    if (key == 's' || key == 'S') {
      save_capture(frame);
    }
  }
}

void PreviewApplication::update_display_rate() {
  const auto now = Clock::now();
  const auto window = now - rate_started_at_;
  if (window < std::chrono::seconds{1}) {
    return;
  }

  const auto seconds = std::chrono::duration<double>{window}.count();
  display_fps_ = static_cast<double>(frames_in_window_) / seconds;
  frames_in_window_ = 0;
  rate_started_at_ = now;
}

void PreviewApplication::draw_overlay(Frame& frame) const {
  // 叠加文字与起始版本保持一致：帧序号 + 实际显示帧率。
  // 丢帧和空帧统计通过 CameraSession::stats() 暴露，不改变用户可见的窗口内容。
  std::ostringstream overlay;
  overlay << "frame " << frame.sequence << "  " << std::fixed
          << std::setprecision(1) << display_fps_ << " FPS";

  cv::putText(frame.image, overlay.str(), cv::Point{20, 36},
              cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
              cv::LINE_AA);
}

void PreviewApplication::save_capture(const Frame& frame) const {
  std::error_code error;
  std::filesystem::create_directories(options_.capture_directory, error);
  if (error) {
    std::cerr << "camera: cannot create " << options_.capture_directory.string()
              << ": " << error.message() << '\n';
    return;
  }

  // 与起始版本一致：跳过已经存在的编号，避免覆盖上次的截图。
  std::filesystem::path filename;
  std::size_t number = capture_number_;
  do {
    ++number;
    filename = options_.capture_directory /
               ("capture-" + std::to_string(number) + ".png");
  } while (std::filesystem::exists(filename, error) && !error);

  // 截图失败只报告，不终止预览：采集仍然可用。
  if (!cv::imwrite(filename.string(), frame.image)) {
    std::cerr << "camera: failed to save " << filename.string() << '\n';
    return;
  }
  std::cout << "Saved " << filename.string() << '\n';
}

bool PreviewApplication::window_closed() const {
  const double visible =
      cv::getWindowProperty(options_.window_title, cv::WND_PROP_VISIBLE);
  return visible >= 0.0 && visible < 1.0;
}

}  // namespace camera
