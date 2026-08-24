 #include "camera/preview_application.hpp"

  #include <opencv2/highgui.hpp>
  #include <opencv2/imgcodecs.hpp>
  #include <opencv2/imgproc.hpp>

  #include <filesystem>
  #include <iomanip>
  #include <iostream>
  #include <sstream>
  #include <stdexcept>
  #include <string>
  #include <utility>

  namespace sturdy_guide::camera {

  PreviewApplication::PreviewApplication(CameraSession& session,
                                        std::string window_name)
      : session_(session), window_name_(std::move(window_name)) {}

  int PreviewApplication::run() {
    cv::namedWindow(window_name_, cv::WINDOW_NORMAL);
    for (;;) {
      session_.rethrowError();

      Frame frame;
      const bool has_frame = session_.tryTakeFrame(frame);
      if (has_frame) {
        draw_overlay(frame);
        cv::imshow(window_name_, frame.image);
      }

      const int key = cv::waitKey(1) & 0xFF;
      if (key == 'q' || key == 'Q') {
        break;
      }
      if (has_frame && (key == 's' || key == 'S')) {
        save_screenshot(frame);
      }
    }
    cv::destroyAllWindows();
    return 0;
  }

  void PreviewApplication::draw_overlay(Frame& frame) const {
    std::ostringstream overlay;
    overlay << "frame " << frame.sequence << "  " << std::fixed
            << std::setprecision(1) << session_.captureFps() << " FPS";
    cv::putText(frame.image, overlay.str(), cv::Point{20, 36},
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                cv::LINE_AA);
  }

  void PreviewApplication::save_screenshot(const Frame& frame) {
    std::filesystem::create_directories("captures");
    std::filesystem::path filename;
    do {
      ++capture_number_;
      filename = std::filesystem::path{"captures"} /
                 ("capture-" + std::to_string(capture_number_) + ".png");
    } while (std::filesystem::exists(filename));
    if (!cv::imwrite(filename.string(), frame.image)) {
      throw std::runtime_error("failed to save " + filename.string());
    }
    std::cout << "Saved " << filename << '\n';
  }
  
  }  // namespace sturdy_guide::camera