#include "camera/opencv_camera.hpp"
#include "camera/preview_application.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{

int parse_integer(
    int argc,
    char* argv[],
    int& index,
    const std::string& option)
{
    if (index + 1 >= argc)
    {
        throw std::invalid_argument(
            "Missing value for " + option);
    }

    try
    {
        return std::stoi(argv[++index]);
    }
    catch (const std::exception&)
    {
        throw std::invalid_argument(
            "Invalid value for " + option);
    }
}

}

int main(int argc, char* argv[])
{
    int device = 0;
    int width = 1280;
    int height = 720;

    try
    {
        for (int index = 1; index < argc; ++index)
        {
            std::string argument = argv[index];

            if (argument == "--device")
            {
                device = parse_integer(
                    argc, argv, index, argument);
            }
            else if (argument == "--width")
            {
                width = parse_integer(
                    argc, argv, index, argument);
            }
            else if (argument == "--height")
            {
                height = parse_integer(
                    argc, argv, index, argument);
            }
            else if (argument == "--help" ||
                     argument == "-h")
            {
                std::cout
                    << "Usage: sturdy-guide-camera "
                    << "[--device INDEX] "
                    << "[--width PIXELS] "
                    << "[--height PIXELS]\n";
                return 0;
            }
            else
            {
                throw std::invalid_argument(
                    "Unknown option: " + argument);
            }
        }

        if (device < 0 || width <= 0 || height <= 0)
        {
            throw std::invalid_argument(
                "Device, width or height is invalid");
        }

        auto source =
            std::make_unique<
                sturdy_guide::camera::OpenCvCamera>(
                device,
                width,
                height);

        sturdy_guide::camera::PreviewApplication application(
            std::move(source));

        return application.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}