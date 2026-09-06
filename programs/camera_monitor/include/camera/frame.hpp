#pragma once
#include <chrono>
#include <cstdint>
#include <opencv2/core/mat.hpp>

namespace sturdy_guide::camera 
{

struct Frame 
{
    cv::Mat image;
    std::uint64_t sequence{};
    std::chrono::steady_clock::time_point captured_at{};
};

}