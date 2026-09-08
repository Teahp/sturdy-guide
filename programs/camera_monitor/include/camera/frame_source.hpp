//
// Created by xzhou on 2026/9/7.
//

#ifndef STURDYGUIDE_FRAME_SOURCE_HPP
#define STURDYGUIDE_FRAME_SOURCE_HPP

#endif //STURDYGUIDE_FRAME_SOURCE_HPP
#pragma once

#include <opencv2/core.hpp>
#include <optional>
#include <string>
#include <memory>

namespace camera {

    // 帧数据封装
    struct FrameResult {
        cv::Mat frame;          // 图像数据
        int frame_id = 0;       // 自增序号
        double timestamp = 0.0; // 采集时间（秒）
    };

    // 帧源抽象接口 - 所有摄像头/假设备都实现这个接口
    class FrameSource {
    public:
        virtual ~FrameSource() = default;

        // 读取下一帧，失败返回 nullopt
        virtual std::optional<FrameResult> read() = 0;

        // 设备属性
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual bool isOpened() const = 0;
        virtual std::string description() const = 0;
    };

    // 工厂函数：创建真实摄像头
    std::unique_ptr<FrameSource> create_real_camera(int device_id, int width, int height);

} // namespace camera