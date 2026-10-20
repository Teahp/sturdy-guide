#include "camera/fake_frame_source.hpp"

#include <stdexcept>
#include <thread>
#include <utility>

namespace camera {
namespace {

// 记录"一次 read 正在进行"，让测试能证明设备对象不是在读帧中途被销毁的。
class ReadScope {
 public:
  explicit ReadScope(SourceProbe* probe) : probe_(probe) {
    if (probe_ != nullptr) {
      probe_->reads.fetch_add(1);
      probe_->reads_in_flight.fetch_add(1);
    }
  }

  ~ReadScope() {
    if (probe_ != nullptr) {
      probe_->reads_in_flight.fetch_sub(1);
    }
  }

  ReadScope(const ReadScope&) = delete;
  ReadScope& operator=(const ReadScope&) = delete;
  ReadScope(ReadScope&&) = delete;
  ReadScope& operator=(ReadScope&&) = delete;

 private:
  SourceProbe* probe_;
};

}  // namespace

FakeFrameSource::FakeFrameSource(Config config,
                                 std::shared_ptr<SourceProbe> probe)
    : config_(config), probe_(std::move(probe)) {}

FakeFrameSource::~FakeFrameSource() {
  if (probe_ != nullptr) {
    probe_->destroyed.store(true);
  }
}

bool FakeFrameSource::open() {
  if (probe_ != nullptr) {
    probe_->open_calls.fetch_add(1);
  }
  open_ = true;
  return true;
}

void FakeFrameSource::close() noexcept {
  if (probe_ != nullptr) {
    probe_->close_calls.fetch_add(1);
  }
  open_ = false;
}

bool FakeFrameSource::is_open() const noexcept { return open_; }

bool FakeFrameSource::read(cv::Mat& frame) {
  const ReadScope scope{probe_.get()};

  if (!open_) {
    return false;
  }

  if (config_.period.count() > 0) {
    std::this_thread::sleep_for(config_.period);
  }

  const std::size_t index = ++reads_;
  if (index == config_.throw_at) {
    throw std::runtime_error("fake frame source: simulated read failure");
  }
  if (config_.total_frames != 0 && index > config_.total_frames) {
    return false;
  }
  if (index == config_.empty_at) {
    frame = cv::Mat{};
    return true;
  }

  const auto shade = static_cast<double>(index % 200);
  frame = cv::Mat(240, 320, CV_8UC3, cv::Scalar{shade, 80.0, 200.0});
  return true;
}

std::string FakeFrameSource::description() const {
  return "fake frame source";
}

}  // namespace camera
