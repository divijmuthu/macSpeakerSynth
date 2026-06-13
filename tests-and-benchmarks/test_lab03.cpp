// Lab 03 tests — renderBlock without opening speakers.
// Build: cmake --build build && ./build/test_lab03

#include "AudioCore.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

void check(bool condition, const char* name) {
    if (condition) {
        std::printf("ok     %s\n", name);
        ++g_passed;
    } else {
        std::printf("not ok %s\n", name);
        ++g_failed;
    }
}

void test_render_block_silent_before_note_on() {
    AudioCore core;
    std::vector<float> buffer(512, 999.0f);
    core.renderBlock(buffer.data(), 256);

    float peak = 0.0f;
    for (float s : buffer) {
        if (s == 999.0f) {
            peak = 999.0f;  // renderBlock never wrote
            break;
        }
        peak = std::fmax(peak, std::fabs(s));
    }
    check(peak < 1e-5f, "renderBlock silent before noteOn");
}

void test_render_block_produces_audio_after_note_on() {
    AudioCore core;
    core.voice().noteOn();

    // Let envelope reach sustain
    std::vector<float> buffer(4096, 0.0f);
    for (int block = 0; block < 20; ++block) {
        core.renderBlock(buffer.data(), 512);
    }

    float peak = 0.0f;
    for (float s : buffer) {
        peak = std::fmax(peak, std::fabs(s));
    }
    check(peak > 0.2f, "renderBlock audible after noteOn + sustain");
}

void test_render_block_stereo_interleaved() {
    AudioCore core;
    core.voice().noteOn();

    std::vector<float> buffer(64, 0.0f);
    for (int block = 0; block < 30; ++block) {
        core.renderBlock(buffer.data(), 32);
    }

    float peak = 0.0f;
    bool lrMatch = true;
    for (std::size_t i = 0; i < 32; ++i) {
        peak = std::fmax(peak, std::fabs(buffer[2 * i + 0]));
        if (buffer[2 * i + 0] != buffer[2 * i + 1]) {
            lrMatch = false;
            break;
        }
    }
    check(peak > 0.2f && lrMatch, "stereo L/R match and output is audible");
}

void test_render_block_bounded_output() {
    AudioCore core;
    core.voice().noteOn();

    std::vector<float> buffer(2048, 0.0f);
    for (int block = 0; block < 10; ++block) {
        core.renderBlock(buffer.data(), 256);
    }

    bool bounded = true;
    float peak = 0.0f;
    for (float s : buffer) {
        peak = std::fmax(peak, std::fabs(s));
        if (s < -1.5f || s > 1.5f) {
            bounded = false;
            break;
        }
    }
    check(bounded && peak > 0.2f, "samples bounded and audible");
}

}  // namespace

int main() {
    std::printf("Lab 03 — AudioCore renderBlock tests\n");
    std::printf("=====================================\n");

    test_render_block_silent_before_note_on();
    test_render_block_produces_audio_after_note_on();
    test_render_block_stereo_interleaved();
    test_render_block_bounded_output();

    std::printf("=====================================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs in AudioCore.cpp — see learning/lab03/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 03 unit tests done. Run ./build/race_synth to hear speakers.\n");
    return EXIT_SUCCESS;
}
