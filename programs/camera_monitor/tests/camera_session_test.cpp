#include "camera/camera_session.hpp"
#include "camera/fake_frame_source.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template <typename Predicate>
bool wait_until(Predicate predicate)
{
    auto deadline =
        std::chrono::steady_clock::now() + 1s;

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate())
        {
            return true;
        }

        std::this_thread::sleep_for(1ms);
    }

    return false;
}

std::vector<cv::Mat> make_frames()
{
    return {
        cv::Mat(4, 4, CV_8UC3, cv::Scalar(255, 0, 0)),
        cv::Mat(4, 4, CV_8UC3, cv::Scalar(0, 255, 0)),
        cv::Mat(4, 4, CV_8UC3, cv::Scalar(0, 0, 255))
    };
}

void test_sequence_increases()
{
    auto source =
        std::make_unique<
            sturdy_guide::camera::FakeFrameSource>(
                make_frames());

    sturdy_guide::camera::CameraSession session(
        std::move(source));

    session.start();

    std::vector<std::uint64_t> sequences;

    bool received = wait_until(
        [&]()
        {
            auto frame = session.latest_frame();

            if (frame)
            {
                sequences.push_back(frame->sequence);
            }

            return sequences.size() >= 3;
        });

    session.stop();

    require(received, "Did not receive three frames");

    for (std::size_t index = 1;
         index < sequences.size();
         ++index)
    {
        require(
            sequences[index] > sequences[index - 1],
            "Frame sequence did not increase");
    }
}

void test_duplicate_start_throws()
{
    auto source =
        std::make_unique<
            sturdy_guide::camera::FakeFrameSource>(
                make_frames());

    sturdy_guide::camera::CameraSession session(
        std::move(source));

    session.start();

    bool threw = false;

    try
    {
        session.start();
    }
    catch (const std::logic_error&)
    {
        threw = true;
    }

    session.stop();

    require(threw, "Duplicate start did not throw");
}

void test_repeated_stop_is_safe()
{
    auto source =
        std::make_unique<
            sturdy_guide::camera::FakeFrameSource>(
                make_frames());

    sturdy_guide::camera::CameraSession session(
        std::move(source));

    session.start();
    session.stop();
    session.stop();

    require(
        session.state() ==
            sturdy_guide::camera::SessionState::stopped,
        "Session did not remain stopped");
}

void test_buffer_is_bounded()
{
    auto source =
        std::make_unique<
            sturdy_guide::camera::FakeFrameSource>(
                make_frames());

    sturdy_guide::camera::CameraSession session(
        std::move(source));

    session.start();
    std::this_thread::sleep_for(30ms);

    require(
        session.buffered_frame_count() <= 2,
        "Frame buffer exceeded two frames");

    require(
        session.dropped_frame_count() > 0,
        "Old frames were not dropped");

    session.stop();
}

void test_background_error_is_visible()
{
    auto source =
        std::make_unique<
            sturdy_guide::camera::FakeFrameSource>(
                make_frames(),
                2);

    sturdy_guide::camera::CameraSession session(
        std::move(source));

    session.start();

    bool failed = wait_until(
        [&]()
        {
            return session.state() ==
                sturdy_guide::camera::SessionState::failed;
        });

    require(failed, "Background failure was not recorded");

    bool threw = false;

    try
    {
        session.rethrow_background_error();
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    session.stop();

    require(threw, "Background error was not rethrown");
}

void test_empty_frame_is_not_published()
{
    std::vector<cv::Mat> frames;
    frames.emplace_back();

    auto source =
        std::make_unique<
            sturdy_guide::camera::FakeFrameSource>(
                std::move(frames));

    sturdy_guide::camera::CameraSession session(
        std::move(source));

    session.start();

    bool failed = wait_until(
        [&]()
        {
            return session.state() ==
                sturdy_guide::camera::SessionState::failed;
        });

    require(failed, "Empty frame did not cause failure");

    require(
        session.buffered_frame_count() == 0,
        "Empty frame was published");

    session.stop();
}

struct DestructionState
{
    std::atomic<bool> reading{false};
    std::atomic<bool> stop_requested{false};
    std::atomic<bool> read_finished{false};
    std::atomic<bool> destroyed_safely{false};
};

class TrackingSource final
    : public sturdy_guide::camera::FrameSource
{
public:
    explicit TrackingSource(
        std::shared_ptr<DestructionState> state)
        : state_(std::move(state))
    {
    }

    ~TrackingSource() override
    {
        state_->destroyed_safely.store(
            state_->read_finished.load());
    }

    cv::Mat read() override
    {
        state_->reading.store(true);

        while (!state_->stop_requested.load())
        {
            std::this_thread::sleep_for(1ms);
        }

        state_->read_finished.store(true);
        return {};
    }

    void request_stop() noexcept override
    {
        state_->stop_requested.store(true);
    }

private:
    std::shared_ptr<DestructionState> state_;
};

void test_destructor_joins_worker()
{
    auto state = std::make_shared<DestructionState>();

    {
        auto source =
            std::make_unique<TrackingSource>(state);

        sturdy_guide::camera::CameraSession session(
            std::move(source));

        session.start();

        require(
            wait_until(
                [&]()
                {
                    return state->reading.load();
                }),
            "Worker did not start reading");
    }

    require(
        state->destroyed_safely.load(),
        "Source was destroyed before worker finished");
}

void run_test(
    const std::string& name,
    const std::function<void()>& test,
    int& failures)
{
    try
    {
        test();
        std::cout << "[PASS] " << name << '\n';
    }
    catch (const std::exception& error)
    {
        ++failures;
        std::cerr
            << "[FAIL] " << name
            << ": " << error.what() << '\n';
    }
}

}

int main()
{
    int failures = 0;

    run_test(
        "sequence increases",
        test_sequence_increases,
        failures);

    run_test(
        "duplicate start throws",
        test_duplicate_start_throws,
        failures);

    run_test(
        "repeated stop is safe",
        test_repeated_stop_is_safe,
        failures);

    run_test(
        "buffer is bounded",
        test_buffer_is_bounded,
        failures);

    run_test(
        "background error is visible",
        test_background_error_is_visible,
        failures);

    run_test(
        "empty frame is not published",
        test_empty_frame_is_not_published,
        failures);

    run_test(
        "destructor joins worker",
        test_destructor_joins_worker,
        failures);

    return failures == 0 ? 0 : 1;
}
