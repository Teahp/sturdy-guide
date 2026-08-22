#pragma once

#include <opencv2/core.hpp>

#include <cstdint>

namespace sturdy_guide::camera {

// 一帧采集结果：像素数据加上单调递增的采集序号。
// 序号由 FrameSource 在每次成功读取时赋值；因丢帧会出现跳号。
struct Frame {
  cv::Mat image;
  std::uint64_t sequence = 0;

  [[nodiscard]] bool empty() const { return image.empty(); }
};

}  // namespace sturdy_guide::camera
