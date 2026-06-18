// Lab 02 tests — Envelope FSM + Voice integration (Oscillator × Envelope).
// Build: cmake --build build && ./build/test_lab02

#include "DSP_utilities/Envelope.h"
#include "DSP_utilities/Voice.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kAttack = 0.01f;
constexpr float kDecay = 0.05f;
constexpr float kSustain = 0.6f;
constexpr float kRelease = 0.1f;
constexpr float kFreq = 440.0f;

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

Envelope makeConfiguredEnvelope() {
    Envelope env;
    env.configure(kAttack, kDecay, kSustain, kRelease, kSampleRate);
    return env;
}

int samplesFor(float seconds) {
    return static_cast<int>(seconds * kSampleRate);
}

void test_attack_delta() {
    Envelope env = makeConfiguredEnvelope();
    env.noteOn();
    const float expected = 1.0f / (kAttack * kSampleRate);
    const float afterOne = env.process();
    check(near(afterOne, expected, 1e-4f), "attack delta: one sample advances level correctly");
}

void test_decay_delta_behavior() {
    Envelope env = makeConfiguredEnvelope();
    env.noteOn();
    for (int i = 0; i < samplesFor(kAttack); ++i) {
        env.process();
    }
    const float atDecayStart = env.level();
    check(near(atDecayStart, 1.0f, 0.02f), "attack reaches ~1.0 before decay");

    const float beforeDecay = env.level();
    env.process();
    const float expectedDrop = (1.0f - kSustain) / (kDecay * kSampleRate);
    check(near(beforeDecay - env.level(), expectedDrop, 1e-4f),
          "decay delta: one sample drops level correctly");
}

void test_note_on_starts_attack() {
    Envelope env = makeConfiguredEnvelope();
    env.noteOn();
    check(env.stage() == EnvelopeStage::Attack && near(env.level(), 0.0f),
          "noteOn -> Attack stage, level 0");
}

void test_attack_reaches_unity() {
    Envelope env = makeConfiguredEnvelope();
    env.noteOn();
    for (int i = 0; i < samplesFor(kAttack) + 10; ++i) {
        env.process();
    }
    check(env.stage() == EnvelopeStage::Decay || env.stage() == EnvelopeStage::Sustain,
          "after attack time, left Attack stage");
    check(env.level() >= 0.95f, "level near 1.0 after attack");
}

void test_decay_reaches_sustain() {
    Envelope env = makeConfiguredEnvelope();
    env.noteOn();
    for (int i = 0; i < samplesFor(kAttack + kDecay) + 50; ++i) {
        env.process();
    }
    check(env.stage() == EnvelopeStage::Sustain, "after attack+decay, in Sustain");
    check(near(env.level(), kSustain, 0.05f), "level near sustain level");
}

void test_release_returns_to_idle() {
    Envelope env = makeConfiguredEnvelope();
    env.noteOn();
    for (int i = 0; i < samplesFor(kAttack + kDecay) + 100; ++i) {
        env.process();
    }
    const bool inSustain = env.stage() == EnvelopeStage::Sustain;
    check(inSustain, "reached Sustain before noteOff");
    if (!inSustain) {
        return;
    }

    env.noteOff();
    check(env.stage() == EnvelopeStage::Release, "noteOff enters Release");
    for (int i = 0; i < samplesFor(kRelease) + 200; ++i) {
        env.process();
    }
    check(env.stage() == EnvelopeStage::Idle, "after release, stage Idle");
    check(near(env.level(), 0.0f), "after release, level 0");
}

void test_voice_silent_before_note_on() {
    Voice voice;
    voice.setFrequency(kFreq, kSampleRate);
    voice.configureEnvelope(kAttack, kDecay, kSustain, kRelease, kSampleRate);

    float peak = 0.0f;
    for (int i = 0; i < 100; ++i) {
        peak = std::fmax(peak, std::fabs(voice.nextSample()));
    }
    check(peak < 1e-5f, "Voice silent when envelope idle");
}

void test_voice_audible_during_sustain() {
    Voice voice;
    voice.setFrequency(kFreq, kSampleRate);
    voice.configureEnvelope(kAttack, kDecay, kSustain, kRelease, kSampleRate);
    voice.noteOn();

    for (int i = 0; i < samplesFor(kAttack + kDecay) + 200; ++i) {
        (void)voice.nextSample();
    }

    float peak = 0.0f;
    for (int i = 0; i < 500; ++i) {
        peak = std::fmax(peak, std::fabs(voice.nextSample()));
    }
    check(peak > 0.3f, "Voice audible during sustain (osc × envelope)");
}

void test_no_click_retrigger() {
    Envelope env = makeConfiguredEnvelope();
    env.noteOn();
    for (int i = 0; i < samplesFor(kAttack + kDecay) + 50; ++i) {
        env.process();
    }
    const float midNote = env.level();
    env.noteOn();
    check(env.stage() == EnvelopeStage::Attack && near(env.level(), midNote),
          "retrigger noteOn keeps level (no amplitude snap)");
}

}  // namespace

int main() {
    std::printf("Lab 02 — Envelope + Voice tests\n");
    std::printf("================================\n");

    test_attack_delta();
    test_decay_delta_behavior();
    test_note_on_starts_attack();
    test_attack_reaches_unity();
    test_decay_reaches_sustain();
    test_release_returns_to_idle();
    test_voice_silent_before_note_on();
    test_voice_audible_during_sustain();
    test_no_click_retrigger();

    std::printf("================================\n");
    std::printf("%d passed, %d failed\n", g_passed, g_failed);
    if (g_failed > 0) {
        std::printf("\nFix TODOs in Envelope.cpp and Voice.cpp — see learning/lab02/README.md\n");
        return EXIT_FAILURE;
    }
    std::printf("\nLab 02 complete. Next: learning/lab03/ (AudioCore + speakers)\n");
    return EXIT_SUCCESS;
}
