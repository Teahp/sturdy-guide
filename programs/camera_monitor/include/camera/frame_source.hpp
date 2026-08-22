#pragma once

#include "camera/frame.hpp"

#include <string>

namespace sturdy_guide::camera {

// 采集源的抽象接口。真实设备（OpenCvCamera）与测试替身
// （FakeFrameSource）实现同一接口，业务代码不感知具体设备。
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  // 打开设备并应用分辨率；失败返回 false 并记录 lastError()。
  virtual bool open(int device, int width, int height) = 0;

  // 读取一帧。返回 false 表示流结束或设备失败；也可能抛出异常。
  // 返回 true 时 out.image 可能为空，由上层决定是否发布。
  virtual bool read(Frame& out) = 0;

  // 最近一次失败的可读描述，供上层报告给用户。
  [[nodiscard]] virtual std::string lastError() const = 0;
};

}  // namespace sturdy_guide::camera
