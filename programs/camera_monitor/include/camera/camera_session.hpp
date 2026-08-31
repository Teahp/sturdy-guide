#pragma once
#include "camera/frame_source.hpp"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <opencv2/core.hpp>

namespace camera {

// 并发采集会话：管理后台采集线程、有界帧缓冲区、状态机与异常传递
class CameraSession {
public:
    struct Frame {
        size_t id;       // 帧序号（采集端递增）
        cv::Mat image;   // 帧图像数据
    };

    explicit CameraSession(std::unique_ptr<FrameSource> source);
    ~CameraSession();

    // 启动采集线程：重复调用安全，已运行则为空操作
    void start();

    // 请求停止采集：多次调用安全，幂等
    void stop();

    // 等待采集线程结束
    void join();

    // 尝试获取最新一帧，返回是否成功
    // 若后台有异常，会在主线程重新抛出
    bool try_get_latest_frame(Frame& out_frame);

    // 检查是否存在后台异常
    bool has_error() const;

    // 获取后台异常指针
    std::exception_ptr get_error() const;

private:
    void worker_loop();

    std::unique_ptr<FrameSource> source_;
    std::thread worker_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic<bool> running_{false};
    bool thread_joined_ = true;

    std::deque<Frame> frame_buffer_; // 有界缓冲区：最多2帧，队首旧、队尾新
    std::exception_ptr error_;
};

} // namespace camera
