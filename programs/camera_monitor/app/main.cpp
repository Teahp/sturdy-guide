#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

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

    auto camera = std::make_unique<OpenCvCamera>(device, width, height);
    CameraSession session{std::move(camera)};
    
    session.start();

    constexpr std::string_view window_name = "Sturdy Guide Camera";
    cv::namedWindow(std::string{window_name}, cv::WINDOW_NORMAL);

    std::size_t frames_in_window = 0;
    double frames_per_second = 0.0;
    auto rate_started_at = std::chrono::steady_clock::now();
    std::size_t capture_number = 0;

    bool failed = false;
    for (;;) {

      const auto frame = session.consume_latest();
      if (frame) {
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
        overlay << "frame " << frame->sequence << "  " << std::fixed
                << std::setprecision(1) << frames_per_second << " FPS";
        cv::putText(frame->image, overlay.str(), cv::Point{20, 36},
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                    cv::LINE_AA);
        cv::imshow(std::string{window_name}, frame->image);
      }

      const int key = cv::waitKey(1) & 0xFF;
      const double visibility = cv::getWindowProperty(
          std::string{window_name}, cv::WND_PROP_VISIBLE);
      if (key == 'q' || key == 'Q' ||
          (visibility >= 0.0 && visibility < 1.0)) {
        break;
      }

      if ((key == 's' || key == 'S') && frame) {
        std::filesystem::create_directories("captures");
        std::filesystem::path filename;
        do {
          ++capture_number;
          filename = std::filesystem::path{"captures"} /
                     ("capture-" + std::to_string(capture_number) + ".png");
        } while (std::filesystem::exists(filename));
        if (!cv::imwrite(filename.string(), frame->image)) {
          throw std::runtime_error("failed to save " + filename.string());
        }
        std::cout << "Saved " << filename << '\n';
      }

      if (session.state() == SessionState::kFailed) {
        try {
          std::rethrow_exception(session.error());
        } catch (const std::exception& error) {
          std::cerr << "camera: " << error.what() << '\n';
        }
        failed = true;
        break;
      }
    }

    session.stop(); 
    cv::destroyAllWindows();
    return failed ? 1 : 0;
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}