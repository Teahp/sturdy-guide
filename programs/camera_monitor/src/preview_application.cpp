#include "camera/preview_application.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace sturdy_guide::camera {
namespace {

constexpr char kWindowName[] = "Sturdy Guide Camera";

void save_frame(const cv::Mat& frame, std::size_t& capture_number) {
  std::filesystem::create_directories("captures");
  std::filesystem::path filename;
  do {
    ++capture_number;
    filename = std::filesystem::path{"captures"} /
               ("capture-" + std::to_string(capture_number) + ".png");
  } while (std::filesystem::exists(filename));

  // The application owns this Mat after try_take_latest(), so image I/O does
  // not run while CameraSession protects its shared frame buffer.
  if (!cv::imwrite(filename.string(), frame)) {
    throw std::runtime_error("failed to save " + filename.string());
  }
  std::cout << "Saved " << filename << '\n';
}//处理截图保存帧，并生成文件名

}  // namespace

PreviewApplication::PreviewApplication(CameraSession& session) : session_(session) {}

int PreviewApplication::run() {
  cv::namedWindow(kWindowName, cv::WINDOW_NORMAL);//窗口名
  std::optional<CapturedFrame> displayed_frame;//用于存储当前显示的帧

  try {
    for (;;) {
      session_.rethrow_if_error();
      if (auto latest = session_.try_take_latest()) {
        displayed_frame = std::move(latest);

        std::ostringstream overlay;
        overlay << "frame " << displayed_frame->sequence << "  "
                << std::fixed << std::setprecision(1)
                << displayed_frame->capture_fps << " FPS";
        cv::putText(displayed_frame->image, overlay.str(), cv::Point{20, 36},
                    cv::FONT_HERSHEY_SIMPLEX, 0.8,
                    cv::Scalar{40, 230, 90}, 2, cv::LINE_AA);
        cv::imshow(kWindowName, displayed_frame->image);
      }//如果有最新的帧，则将其显示在窗口中，并在图像上叠加帧号和帧率信息

      const int key = cv::waitKey(1) & 0xFF;
      const double visibility =
          cv::getWindowProperty(kWindowName, cv::WND_PROP_VISIBLE);
      // GTK and some other backends use a negative value for an unsupported
      // visibility query, which does not mean that the window was closed.
      if (key == 'q' || key == 'Q' ||
          (visibility >= 0.0 && visibility < 1.0)) {
        break;
      }
      if ((key == 's' || key == 'S') && displayed_frame &&
          !displayed_frame->image.empty()) {
        save_frame(displayed_frame->image, capture_number_);
      }
    }

    session_.stop();
    cv::destroyWindow(kWindowName);
    return 0;
  } catch (...) {
    session_.stop();
    cv::destroyWindow(kWindowName);
    throw;
  }
}

}  // namespace sturdy_guide::camera
