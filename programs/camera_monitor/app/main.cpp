#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

// 主线程职责：解析参数、组装依赖、显示画面、处理按键与截图。
// 采集工作由后台的 CameraSession 完成，本文件不直接接触摄像头。
int main(const int argc, const char* const argv[]) {
  try {
    int device = 0;
    int width = 1280;
    int height = 720;

    for (int index = 1; index < argc; ++index) {
      const std::string_view argument{argv[index]};
      if (argument == "--help" || argument == "-h") {
        std::cout << "Usage: " << argv[0]
                  << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
                  << "Keys: S saves a frame; Q exits.\n";
        return 0;
      }

      if (argument != "--device" && argument != "--width" &&
          argument != "--height") {
        throw std::invalid_argument("unknown option: " +
                                    std::string{argument});
      }
      if (index + 1 >= argc) {
        throw std::invalid_argument(std::string{argument} +
                                    " requires an integer value");
      }

      const int value = std::stoi(argv[++index]);
      if (argument == "--device") {
        device = value;
      } else if (argument == "--width") {
        width = value;
      } else {
        height = value;
      }
    }

    if (device < 0 || width <= 0 || height <= 0) {
      throw std::invalid_argument(
          "device must be non-negative and dimensions must be positive");
    }

    // 组装：真插头（真实摄像头）-> 采集会话（后台帮手）。
    auto source =
        std::make_unique<sturdy_guide::camera::OpenCvCamera>(device, width, height);
    sturdy_guide::camera::CameraSession session{std::move(source)};
    session.start();

    constexpr std::string_view window_name = "Sturdy Guide Camera";
    cv::namedWindow(std::string{window_name}, cv::WINDOW_NORMAL);

    std::size_t frames_in_window = 0;
    double frames_per_second = 0.0;
    auto rate_started_at = std::chrono::steady_clock::now();
    cv::Mat last_displayed;  // 最近一次显示的画面，供 S 键截图。
    std::size_t capture_number = 0;

    for (;;) {
      // 1) 从帮手的有界缓冲拿最新一帧；有则显示。取帧在锁内，imshow 在锁外。
      sturdy_guide::camera::Frame frame;
      if (session.take_latest(frame)) {
        ++frames_in_window;
        const auto now = std::chrono::steady_clock::now();
        const auto rate_window = now - rate_started_at;
        if (rate_window >= std::chrono::seconds{1}) {
          const auto seconds = std::chrono::duration<double>{rate_window}.count();
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
        cv::imshow(std::string{window_name}, frame.image);
        last_displayed = frame.image;
      }

      // 2) 后台采集出错？取出、报告并退出。
      if (const std::exception_ptr error = session.take_error()) {
        try {
          std::rethrow_exception(error);
        } catch (const std::exception& reported) {
          std::cerr << "camera: " << reported.what() << '\n';
        }
        break;
      }

      // 3) 处理按键与窗口状态。
      const int key = cv::waitKey(1) & 0xFF;
      const double visibility = cv::getWindowProperty(
          std::string{window_name}, cv::WND_PROP_VISIBLE);
      // GTK 等后端可能以负值表示不支持可见性查询，不能把它当作已关闭。
      if (key == 'q' || key == 'Q' ||
          (visibility >= 0.0 && visibility < 1.0)) {
        break;
      }

      if (key == 's' || key == 'S') {
        if (!last_displayed.empty()) {
          std::filesystem::create_directories("captures");
          std::filesystem::path filename;
          do {
            ++capture_number;
            filename = std::filesystem::path{"captures"} /
                       ("capture-" + std::to_string(capture_number) + ".png");
          } while (std::filesystem::exists(filename));
          if (!cv::imwrite(filename.string(), last_displayed)) {
            throw std::runtime_error("failed to save " + filename.string());
          }
          std::cout << "Saved " << filename << '\n';
        }
      }
    }

    session.stop();  // 干净地停下后台帮手。
    cv::destroyAllWindows();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
