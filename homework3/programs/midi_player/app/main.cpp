#include "midi/console_midi_output.hpp"
#include "midi/demo_song.hpp"
#include "midi/midi_player.hpp"

#if defined(_WIN32)
#include "midi/win_mm_midi_output.hpp"
#elif defined(__linux__)
#include "midi/raw_midi_output.hpp"
#endif

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

struct Options {
  std::string device;
  bool show_help = false;
  bool list_devices = false;
};

Options parse_options(const int argc, const char* const argv[]) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
    } else if (argument == "--list-devices") {
      options.list_devices = true;
    } else if (argument == "--device") {
      if (index + 1 >= argc) {
        throw std::invalid_argument("--device requires a value");
      }
      options.device = argv[++index];
    } else {
      throw std::invalid_argument("unknown option: " + std::string{argument});
    }
  }
  return options;
}

void print_usage(const char* const program) {
  std::cout << "Usage: " << program
            << " [--list-devices] [--device DEVICE]\n\n"
            << "Without --device, MIDI messages are printed to the terminal.\n"
#if defined(_WIN32)
            << "On Windows, DEVICE is the numeric ID shown by "
               "--list-devices.\n";
#elif defined(__linux__)
            << "On Linux, DEVICE is a raw MIDI path such as "
               "/dev/snd/midiC1D0.\n";
#else
            << "This platform currently supports terminal output only.\n";
#endif
}

#if defined(_WIN32)

std::uint32_t parse_device_id(const std::string_view value) {
  std::uint32_t id = 0;
  const auto parse_result =
      std::from_chars(value.data(), value.data() + value.size(), id);
  if (parse_result.ec != std::errc{} ||
      parse_result.ptr != value.data() + value.size()) {
    throw std::invalid_argument("Windows MIDI device must be a numeric ID");
  }
  return id;
}

void print_devices() {
  const auto devices = sturdy_guide::midi::WinMmMidiOutput::devices();
  if (devices.empty()) {
    std::cout << "No Windows MIDI output devices found.\n";
    return;
  }

  for (std::size_t id = 0; id < devices.size(); ++id) {
    std::cout << id << ": " << devices[id] << '\n';
  }
}

#elif defined(__linux__)

std::vector<std::filesystem::path> linux_midi_devices() {
  std::vector<std::filesystem::path> devices;
  std::error_code error;
  const std::filesystem::path sound_directory{"/dev/snd"};

  for (std::filesystem::directory_iterator current{sound_directory, error}, end;
       !error && current != end; current.increment(error)) {
    const std::string name = current->path().filename().string();
    if (name.rfind("midiC", 0) == 0) {
      devices.push_back(current->path());
    }
  }

  std::sort(devices.begin(), devices.end());
  return devices;
}

void print_devices() {
  const auto devices = linux_midi_devices();
  if (devices.empty()) {
    std::cout << "No Linux raw MIDI output devices found under /dev/snd.\n";
    return;
  }

  for (const auto& device : devices) {
    std::cout << device.string() << '\n';
  }
}

#else

void print_devices() {
  std::cout << "Device output is supported on Windows and Linux only.\n";
}

#endif

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    const Options options = parse_options(argc, argv);
    if (options.show_help) {
      print_usage(argv[0]);
      return 0;
    }
    if (options.list_devices) {
      print_devices();
      return 0;
    }

    // 多态后端由应用层选择，播放器只依赖 MidiOutput 契约。
    std::unique_ptr<sturdy_guide::midi::MidiOutput> output;
    if (options.device.empty()) {
      output =
          std::make_unique<sturdy_guide::midi::ConsoleMidiOutput>(std::cout);
      std::cout << "Console mode: events are displayed but no sound is "
                   "synthesized.\n";
    } else {
#if defined(_WIN32)
      const std::uint32_t device_id = parse_device_id(options.device);
      output =
          std::make_unique<sturdy_guide::midi::WinMmMidiOutput>(device_id);
      std::cout << "Sending the demo scale to Windows MIDI device "
                << device_id << "\n";
#elif defined(__linux__)
      output =
          std::make_unique<sturdy_guide::midi::RawMidiOutput>(options.device);
      std::cout << "Sending the demo scale to " << options.device << "\n";
#else
      throw std::invalid_argument(
          "device output is supported on Windows and Linux only");
#endif
    }

    // output 声明在 player 之前，因此析构顺序保证 output 活得更久。
    sturdy_guide::midi::MidiPlayer player{*output};
    player.play(sturdy_guide::midi::make_demo_scale());
    player.wait();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "midi player: " << error.what() << '\n';
    return 1;
  }
}
