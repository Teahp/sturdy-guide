#pragma once

#include "camera/frame.h"
#include <opencv2/core.hpp>
#include <string>

namespace camera {

// 帧源抽象接口：所有摄像头实现（真实/假）都必须实现此接口
class FrameSource {
public:
    virtual ~FrameSource() = default;

    // 打开设备，失败时抛出异常
    virtual void open() = 0;

    // 读取一帧，返回 false 表示无法继续读取
    // 成功时 frame 会被填充有效数据
    virtual bool read(Frame& frame) = 0;

    // 关闭设备
    virtual void close() = 0;

    // 设备是否已打开
    virtual bool isOpened() const = 0;

    // 设备描述（用于错误消息）
    virtual std::string description() const = 0;
};

}  // namespace camera
