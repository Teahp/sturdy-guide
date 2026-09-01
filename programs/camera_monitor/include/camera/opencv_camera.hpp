#pragma once
#include "camera/frame_source.hpp"
#include <opencv2/videoio.hpp>

class OpenCvCamera final : public FrameSource{
    public:
     OpenCvCamera(int device, int width, int height);
     Frame read() override;
    
    private:
     cv::VideoCapture capture_;
     std::size_t next_sequence_ = 0; 
};