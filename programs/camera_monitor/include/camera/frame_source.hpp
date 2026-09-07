#pragma once
#include <opencv2/core.hpp>
#include <optional>

namespace sturdy_guide::camera {

// 所有相机必须遵守的接口约定
class FrameSource {
public:
    virtual ~FrameSource() = default;
    // 读取一帧，成功返回图像，失败返回空
    virtual std::optional<cv::Mat> read() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

} // namespace
