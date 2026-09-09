#pragma once

#include <opencv2/core.hpp>
#include <chrono>
#include <cstdint>

namespace camera {

struct Frame {
    cv::Mat image;
    std::uint64_t sequence{0};
    std::chrono::steady_clock::time_point timestamp;
    bool valid{false};

    bool isEmpty() const { return !valid || image.empty(); }
};

}  // namespace camera
