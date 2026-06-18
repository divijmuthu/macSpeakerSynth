// Lab 04 tests — lock-free queue, parser, AudioCore drain (no ZMQ, no speakers).
// Build: cmake --build build && ./build/test_lab04

#include "AudioCore.h"
#include "ControlParser.h"
#include "LockFreeQueue.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

namespace {

constexpr float kSampleRate = AudioCore::kSampleRate;

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

bool near(float a, float b, float eps = 1e-3f) {
    return std::fabs(a - b) < eps;
}

void test_queue_push_pop() {
    LockFreeQueue<8> queue;
    ControlMessage in{ControlType::NoteOn, 523.25f};
    ControlMessage out{};

    check(queue.push(in), "push returns true when not full");
    check(queue.pop(out), "pop returns true when not empty");
    check(out.type == ControlType::NoteOn && near(out.frequencyHz, 523.25f),
          "pop returns same message");
}

void test_queue_empty_pop() {
    LockFreeQueue<8> queue;
    ControlMessage out{};
    check(!queue.pop(out), "pop on empty queue returns false");
}

void test_parse_note_off() {
    ControlMessage msg{};
    check(parseControlMessage("NOTE_OFF,0", msg), "parse NOTE_OFF");
    check(msg.type == ControlType::NoteOff, "NOTE_OFF type");
}

void test_parse_note_on_frequency() {
    ControlMessage msg{};
    check(parseControlMessage("NOTE_ON,880.0", msg), "parse NOTE_ON");
    check(msg.type == ControlType::NoteOn, "NOTE_ON type");
    check(near(msg.frequencyHz, 880.0f), "NOTE_ON frequency extracted");
}

void test_drain_note_on_starts_voice() {
    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore core(&queue);

    ControlMessage noteOn{ControlType::NoteOn, 440.0f};
    queue.push(noteOn);
    core.drainControlQueue();

    std::vector<float> buffer(512, 0.0f);
    for (int block = 0; block < 30; ++block) {
        core.renderBlock(buffer.data(), 256);
    }

    float peak = 0.0f;
    for (float s : buffer) {
        peak = std::fmax(peak, std::fabs(s));
    }
    check(peak > 0.2f, "drainControlQueue NOTE_ON → audible renderBlock output");
}

void test_drain_frequency_change() {
    LockFreeQueue<AudioCore::kControlQueueCapacity> queue;
    AudioCore core(&queue);

    queue.push({ControlType::NoteOn, 220.0f});
    core.drainControlQueue();

    std::vector<float> low(2048, 0.0f);
    for (int i = 0; i < 8; ++i) {
        core.renderBlock(low.data(), 256);
    }

    queue.push({ControlType::NoteOn, 880.0f});
    core.drainControlQueue();

    std::vector<float> high(2048, 0.0f);
    for (int i = 0; i < 8; ++i) {
        core.renderBlock(high.data(), 256);
    }

    auto zeroCrossings = [](const std::vector<float>& samples) {
        int count = 0;
        for (std::size_t i = 1; i < samples.size(); ++i) {
            if ((samples[i - 1] >= 0.0f && samples[i] < 0.0f) ||
                (samples[i - 1] < 0.0f && samples[i] >= 0.0f)) {
                ++count;
            }
        }
        return count;
    };

    const int lowCross = zeroCrossings(low);
    const int highCross = zeroCrossings(high);
    check(highCross > lowCross, "higher NOTE_ON frequency → more zero crossings in buffer");
}

void test_spsc_producer_consumer() {
    LockFreeQueue<64> queue;
    ControlMessage probe{ControlType::NoteOn, 440.0f};
    if (!queue.push(probe)) {
        std::printf("ok     SPSC threaded test (skipped until Q1/Q2 implemented)\n");
        ++g_passed;
        return;
    }
    ControlMessage discard{};
    (void)queue.pop(discard);

    constexpr int kMessages = 100;
    std::atomic<int> consumed{0};

    std::thread producer([&] {
        for (int i = 0; i < kMessages; ++i) {
            ControlMessage msg{ControlType::NoteOn, 440.0f + static_cast<float>(i)};
            while (!queue.push(msg)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&] {
        ControlMessage msg{};
        while (consumed.load() < kMessages) {
            if (queue.pop(msg)) {
                ++consumed;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();
    check(consumed.load() == kMessages, "SPSC producer/consumer delivered all messages");
}

}  // namespace

int main() {
    std::printf("Lab 04 — Queue + parser + drain tests\n");
    std::printf("======================================\n");

    test_queue_push_pop();
    test_queue_empty_pop();
    test_parse_note_off();
    test_parse_note_on_frequency();
    test_drain_note_on_starts_voice();
    test_drain_frequency_change();
    test_spsc_producer_consumer();

    std::printf("======================================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs — see learning/lab04/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 04 tests done. Run ./build/race_synth + python py_interface/main.py\n");
    return EXIT_SUCCESS;
}
