#pragma once

#include "camera/frame_source.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace camera {

class FakeFrameSource final : public FrameSource {
 public:
  enum class StepKind {
    Frame,
    EmptyFrame,
    Failure,
  };

  struct Step {
    StepKind kind{StepKind::Frame};
    int value{0};
    std::chrono::milliseconds delay{0};
    std::string message;
  };

  explicit FakeFrameSource(std::vector<Step> steps);
  FakeFrameSource(std::vector<Step> steps,
                  std::shared_ptr<std::atomic_bool> destroyed_flag);
  ~FakeFrameSource() override;

  static Step frame(int value, std::chrono::milliseconds delay = {});
  static Step empty(std::chrono::milliseconds delay = {});
  static Step failure(std::string message,
                      std::chrono::milliseconds delay = {});

  void open() override;
  cv::Mat read() override;
  void close() override;

  std::size_t openCount() const;
  std::size_t closeCount() const;
  std::size_t readCount() const;

 private:
  std::vector<Step> steps_;
  std::shared_ptr<std::atomic_bool> destroyed_flag_;
  mutable std::mutex mutex_;
  std::size_t next_step_{0};
  std::size_t open_count_{0};
  std::size_t close_count_{0};
  std::size_t read_count_{0};
  bool opened_{false};
};

}  // namespace camera
