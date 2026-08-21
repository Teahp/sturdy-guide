#pragma once

#include <opencv2/core.hpp>
#include <optional>

class FrameSource {
public:
    virtual ~FrameSource() = default;

    // 获取一帧图像，失败时返回 std::nullopt
    virtual std::optional<cv::Mat> getFrame() = 0;
};
