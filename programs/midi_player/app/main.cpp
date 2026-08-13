#include "midi/console_midi_output.hpp"
#include "midi/demo_song.hpp"
#include "midi/midi_player.hpp"
#include "midi/raw_midi_output.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Options {
  std::string device;
  bool show_help = false;
};

Options parse_options(const int argc, const char* const argv[]) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
    } else if (argument == "--device") {
      if (index + 1 >= argc) {
        throw std::invalid_argument("--device requires a path");
      }
      options.device = argv[++index];
    } else {
      throw std::invalid_argument("unknown option: " + std::string{argument});
    }
  }
  return options;
}

void print_usage(const char* const program) {
  std::cout << "Usage: " << program << " [--device PATH]\n\n"
            << "Without --device, MIDI messages are printed to the terminal.\n"
            << "On Linux, PATH can be a raw MIDI device such as "
               "/dev/snd/midiC1D0.\n";
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    const Options options = parse_options(argc, argv);
    if (options.show_help) {
      print_usage(argv[0]);
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
      output =
          std::make_unique<sturdy_guide::midi::RawMidiOutput>(options.device);
      std::cout << "Sending the demo scale to " << options.device << "\n";
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
