#include "camera/preview_application.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace sturdy_guide::camera
{

PreviewApplication::PreviewApplication(
    std::unique_ptr<FrameSource> source)
    : session_(std::move(source))
{
}

int PreviewApplication::run()
{
    bool window_created = false;
    int result = 0;

    try
    {
        session_.start();

        while (true)
        {
            session_.rethrow_background_error();

            auto frame = session_.latest_frame();

            if (!frame)
            {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1));
                continue;
            }

            update_fps(*frame);
            draw_overlay(frame->image, *frame);

            cv::imshow(window_name_, frame->image);
            window_created = true;

            int key = cv::waitKey(1);

            if (key == 'q' || key == 'Q')
            {
                break;
            }

            if (key == 's' || key == 'S')
            {
                save_screenshot(frame->image);
            }

            double visible = cv::getWindowProperty(
                window_name_,
                cv::WND_PROP_VISIBLE);

            if (visible >= 0.0 && visible < 1.0)
            {
                break;
            }
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }

    session_.stop();

    if (window_created)
    {
        cv::destroyWindow(window_name_);
    }

    return result;
}

void PreviewApplication::update_fps(const Frame& frame)
{
    if (previous_frame_time_ !=
        std::chrono::steady_clock::time_point{})
    {
        double seconds =
            std::chrono::duration<double>(
                frame.captured_at - previous_frame_time_)
                .count();

        if (seconds > 0.0)
        {
            double current_fps = 1.0 / seconds;

            if (fps_ == 0.0)
            {
                fps_ = current_fps;
            }
            else
            {
                fps_ = fps_ * 0.9 + current_fps * 0.1;
            }
        }
    }

    previous_frame_time_ = frame.captured_at;
}

void PreviewApplication::draw_overlay(
    cv::Mat& image,
    const Frame& frame) const
{
    std::ostringstream text;
    text << "frame " << frame.sequence
         << "  FPS " << std::fixed
         << std::setprecision(1) << fps_;

    cv::putText(
        image,
        text.str(),
        cv::Point(20, 40),
        cv::FONT_HERSHEY_SIMPLEX,
        1.0,
        cv::Scalar(0, 255, 0),
        2);
}

void PreviewApplication::save_screenshot(const cv::Mat& image)
{
    std::filesystem::create_directories("captures");

    std::filesystem::path path;

    do
    {
        path = std::filesystem::path("captures") /
            ("capture-" +
             std::to_string(screenshot_number_++) +
             ".png");
    }
    while (std::filesystem::exists(path));

    if (!cv::imwrite(path.string(), image))
    {
        throw std::runtime_error("Failed to save screenshot");
    }

    std::cout << "Saved " << path.string() << '\n';
}

}