#pragma once

  #include "camera/frame_source.hpp"

  #include <atomic>
  #include <chrono>
  #include <cstdint>
  #include <string>

  namespace sturdy_guide::camera {

  class FakeFrameSource final : public FrameSource {
   public:
    struct Options {
      int width = 320;
      int height = 240;
      std::chrono::milliseconds delay{};
      std::int64_t fail_after = -1;
      std::int64_t throw_after = -1;
      std::int64_t empty_every = -1;

      Options() {}
    };

    explicit FakeFrameSource(Options options = Options());
    bool open(int device, int width, int height) override;
    bool read(Frame& out) override;
    [[nodiscard]] std::string lastError() const override;

    [[nodiscard]] std::uint64_t produced() const { return produced_.load(); }
    [[nodiscard]] int open_calls() const { return open_calls_.load(); }
    [[nodiscard]] int reads_in_flight() const { return reads_in_flight_.load(); }

   private:
    Options options_;
    std::atomic<std::uint64_t> produced_{0};
    std::atomic<int> open_calls_{0};
    std::atomic<int> reads_in_flight_{0};
    std::string last_error_;
  };

  }  // namespace sturdy_guide::camera
