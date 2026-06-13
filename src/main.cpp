#include "AudioCore.h"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

int main() {
    std::printf("RACE synth — Lab 03 (Ctrl+C to quit)\n");

    AudioCore engine;

    // TODO (Lab 03, Q4): Trigger the envelope before audio starts (smooth fade-in).
    engine.voice().noteOn(); // get engine's voice's envelope ready to go

    // TODO (Lab 03, Q5): Start playback. On failure, print an error and return 1.
    if (!engine.start()) {
        std::cerr << "Failed to start audio device." << std::endl;
        return 1;
    }

    // print w/fmt string to read off intended default freq
    std::printf("Playing %.0f Hz — you should hear a tone with a soft attack.\n",
                AudioCore::kDefaultFreqHz);

    // Keep process alive while miniaudio callback runs on another thread.
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
