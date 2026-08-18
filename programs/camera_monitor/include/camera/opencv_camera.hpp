#ifndef CAMERA_OPENCV_CAMERA_HPP
#define CAMERA_OPENCV_CAMERA_HPP

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

namespace camera {

// 真实设备后端：封装 cv::VideoCapture，只负责打开、读取、释放。
// 不含线程、缓冲区或 UI 逻辑，慢速 I/O 全部由调用方放在临界区之外。
class OpenCvCamera final : public FrameSource {
 public:
  OpenCvCamera(int device, int width, int height);

  bool open() override;
  bool read(cv::Mat& frame) override;
  void close() override;

 private:
  int device_;
  int width_;
  int height_;
  cv::VideoCapture capture_;
};

}  // namespace camera

#endif  // CAMERA_OPENCV_CAMERA_HPP
