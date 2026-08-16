// 抽象帧源接口：真实摄像头与测试假设备遵循同一契约。
//
// 约定：
//  - open() 失败返回 false（不抛异常），可重入（先 release 再 open）；
//  - read() 成功返回 true 并填充 frame；返回 false 表示流正常结束；
//  - read() 可以抛异常表示读取错误（如设备断开），由调用方 CameraSession 捕获并回传。
#pragma once

#include <opencv2/core.hpp>

namespace camera {

class FrameSource {
 public:
  virtual ~FrameSource() = default;

  // 打开设备/准备数据。失败返回 false。
  virtual bool open() = 0;

  // 读取一帧。
  virtual bool read(cv::Mat& frame) = 0;

  // 释放资源，幂等；之后可以再次 open()。
  virtual void release() = 0;
};

}  // namespace camera
