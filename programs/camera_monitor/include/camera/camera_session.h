#pragma once

#include "camera/frame.h"
#include "camera/frame_source.h"
#include <array>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace camera {

// CameraSession 管理采集线程的生命周期和有界帧缓冲
// 
// 线程安全契约：
// - start()/stop() 可以从任何线程调用，但通常由主线程调用
// - tryGetLatestFrame() 可以从任何线程调用
// - 内部使用 mutex 保护帧缓冲和状态
// - 不使用全局可变状态
class CameraSession {
public:
    explicit CameraSession(std::unique_ptr<FrameSource> source);
    ~CameraSession();

    // 禁止拷贝和移动（因为包含线程和互斥锁）
    CameraSession(const CameraSession&) = delete;
    CameraSession& operator=(const CameraSession&) = delete;
    CameraSession(CameraSession&&) = delete;
    CameraSession& operator=(CameraSession&&) = delete;

    // 启动采集线程
    // 如果已经启动，则忽略（幂等）
    void start();

    // 停止采集线程并等待其结束
    // 如果未启动，则忽略（幂等）
    // 可以安全地连续调用多次
    void stop();

    // 尝试获取最新帧
    // 如果有新帧，返回 true 并填充 frame
    // 如果没有新帧（或已停止），返回 false
    bool tryGetLatestFrame(Frame& frame);

    // 检查是否有后台错误
    bool hasError() const;

    // 获取错误消息
    std::string getErrorMessage() const;

    // 是否正在运行
    bool isRunning() const;

private:
    void workerLoop();

    std::unique_ptr<FrameSource> source_;
    std::thread worker_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    // 双缓冲：最多保留两帧
    std::array<Frame, 2> buffers_;
    int write_index_{0};  // 当前写入的缓冲区索引
    bool has_new_frame_{false};
    std::uint64_t frame_sequence_{0};

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};

    std::exception_ptr worker_exception_;
    std::string error_message_;
};

}  // namespace camera
