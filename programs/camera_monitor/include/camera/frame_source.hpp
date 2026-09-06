#pragma once
#include <opencv2/core/mat.hpp>
namespace sturdy_guide::camera 
{class FrameSource 
{
public:
    virtual ~FrameSource() = default;
    virtual cv::Mat read() = 0;
    virtual void request_stop() noexcept {}
};

}