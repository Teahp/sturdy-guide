#pragma once
#include <opencv2/core.hpp>

namespace camera {

// 帧数据源抽象接口：隔离真实硬件与模拟实现
class FrameSource {
public:
    virtual ~FrameSource() = default;

    // 读取一帧：成功则填充frame，失败抛出std::exception
    virtual void read(cv::Mat& frame) = 0;
};

} // namespace camera
