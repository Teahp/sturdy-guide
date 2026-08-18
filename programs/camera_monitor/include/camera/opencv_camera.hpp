#pragma once

#include "camera/frame_source.hpp"

#include <atomic>

#include <opencv2/videoio.hpp>

namespace sturdy_guide::camera {

class OpenCvCamera final : public FrameSource {
 public:
  OpenCvCamera(int device, int width, int height);//设置摄像头设备索引和分辨率，初始化 OpenCV 的 VideoCapture 对象。

  cv::Mat read() override;//覆写 FrameSource 类的 read() 方法，从摄像头读取一帧图像。（cv::Mat）
  void request_stop() noexcept override;//覆写 FrameSource 类的 request_stop() 方法，请求停止摄像头读取操作，设置 stop_requested_ 标志为 true，并尝试释放摄像头资源。

 private:
  cv::VideoCapture camera_;// OpenCV 的 VideoCapture 类用于从摄像头或视频文件中捕获视频帧。
  std::atomic<bool> stop_requested_{false};//原子布尔变量，用于标记是否请求停止摄像头读取操作，确保在多线程环境下的安全访问。
};

}  // namespace sturdy_guide::camera
