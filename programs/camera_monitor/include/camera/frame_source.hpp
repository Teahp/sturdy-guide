#ifndef CAMERA_FRAME_SOURCE_HPP
#define CAMERA_FRAME_SOURCE_HPP

#include <opencv2/core.hpp>

namespace camera {

// 帧源抽象：真实摄像头与测试用假设备实现同一个接口。
// 所有权：由 CameraSession 通过 unique_ptr 独占持有，生命周期与会话一致。
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  // 打开设备。返回 false 表示无法打开（设备不存在、被占用或权限不足）。
  virtual bool open() = 0;

  // 读取一帧。返回 true 且 frame 非空表示成功；
  // 返回 false 或 frame 为空都表示采集失败。
  virtual bool read(cv::Mat& frame) = 0;

  // 释放设备，之后可以再次 open()。
  virtual void close() = 0;
};

}  // namespace camera

#endif  // CAMERA_FRAME_SOURCE_HPP
