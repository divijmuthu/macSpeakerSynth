// Lab 08 tests — polyphony, voice allocation, mixing.
// Build: cmake --build build && ./build/test_lab08

#include "AudioCore.h"
#include "ControlParser.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

constexpr float kSampleRate = 48000.0f;

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

float renderPeak(AudioCore& core, int blocks = 32) {
    std::vector<float> buffer(512 * 2, 0.0f);
    for (int b = 0; b < blocks; ++b) {
        core.renderBlock(buffer.data(), 512);
    }
    float peak = 0.0f;
    for (float s : buffer) {
        peak = std::fmax(peak, std::fabs(s));
    }
    return peak;
}

void test_two_notes_use_separate_slots() {
    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore core(&queue);

    queue.push({ControlType::NoteOn, 440.0f});
    queue.push({ControlType::NoteOn, 554.37f});
    core.drainControlQueue();

    check(core.activeVoiceCount() >= 2, "two NOTE_ON → at least 2 active voices");
    check(core.voiceSlot(0).voice.isActive(), "slot 0 active");
    check(core.voiceSlot(1).voice.isActive(), "slot 1 active");
    check(std::fabs(core.voiceSlot(0).frequencyHz - 440.0f) < 1.0f ||
              std::fabs(core.voiceSlot(1).frequencyHz - 440.0f) < 1.0f,
          "440 Hz assigned to a slot");
}

void test_chord_headroom_is_stable() {
    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore core(&queue);

    queue.push({ControlType::NoteOn, 261.63f});
    core.drainControlQueue();
    const float singlePeak = renderPeak(core);

    queue.push({ControlType::NoteOn, 329.63f});
    queue.push({ControlType::NoteOn, 392.0f});
    core.drainControlQueue();
    const float chordPeak = renderPeak(core);

    check(chordPeak > singlePeak * 0.6f, "C major chord remains audible with headroom");
    check(chordPeak < 0.99f, "C major chord stays below hard clip");
}

void test_note_off_by_frequency() {
    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore core(&queue);

    queue.push({ControlType::NoteOn, 440.0f});
    queue.push({ControlType::NoteOn, 554.37f});
    core.drainControlQueue();

    queue.push({ControlType::NoteOff, 440.0f});
    core.drainControlQueue();

    check(core.activeVoiceCount() >= 1, "one voice still active after partial NOTE_OFF");
    const float peak = renderPeak(core, 16);
    check(peak > 0.05f, "remaining voice still audible");
}

void test_parse_note_off_with_frequency() {
    ControlMessage msg{};
    check(parseControlMessage("NOTE_OFF,440.000000", msg), "parse NOTE_OFF with freq");
    check(msg.type == ControlType::NoteOff && std::fabs(msg.frequencyHz - 440.0f) < 1e-3f,
          "NOTE_OFF carries frequency");
}

}  // namespace

int main() {
    std::printf("Lab 08 — Polyphony tests\n");
    std::printf("========================\n");

    test_two_notes_use_separate_slots();
    test_chord_headroom_is_stable();
    test_note_off_by_frequency();
    test_parse_note_off_with_frequency();

    std::printf("========================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs in AudioCore.cpp — see learning/lab08/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 08 done. Hold multiple piano keys for chords.\n");
    return EXIT_SUCCESS;
}
