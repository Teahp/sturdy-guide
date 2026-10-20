#pragma once

#include <chrono>
#include <cstdint>

#include <opencv2/core.hpp>

namespace camera {

// 一帧画面及其采集元数据。
//
// sequence 由 CameraSession 按采集顺序分配，从 1 开始递增；
// captured_at 使用 steady_clock，因此只适合做时长计算，不适合当墙钟时间。
struct Frame {
  cv::Mat image;
  std::uint64_t sequence = 0;
  std::chrono::steady_clock::time_point captured_at{};
};

}  // namespace camera
