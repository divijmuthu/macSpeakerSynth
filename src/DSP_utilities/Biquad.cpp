#include "DSP_utilities/Biquad.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

void Biquad::setLowPass(float cutoffHz, float sampleRate, float q) {
    // Intermediate vars (Cookbook names → valid C++ locals, not members):
    //
    //   w0    = 2π·fc/fs   "digital frequency" of the cutoff (radians/sample)
    //   cosW0 = cos(w0)    used in pole placement on the z-plane
    //   alpha = sin(w0)/(2Q)  bandwidth / resonance knob
    //
    const float fc = std::clamp(cutoffHz, 20.0f, sampleRate * 0.49f);
    const float w0 = 2.0f * kPi * fc / sampleRate;
    const float cosW0 = std::cos(w0);
    const float alpha = std::sin(w0) / (2.0f * q);

    // Raw biquad coefficients (before normalizing a0 into them).
    const float b0 = (1.0f - cosW0) / 2.0f;
    const float b1 = 1.0f - cosW0;
    const float b2 = (1.0f - cosW0) / 2.0f;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cosW0;
    const float a2 = 1.0f - alpha;

    // Store normalized form so process() can use: y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2
    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
    a1_ = a1 / a0;
    a2_ = a2 / a0;
}

float Biquad::process(float input) {
    const float y = b0_ * input + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;

    x2_ = x1_;
    x1_ = input;
    y2_ = y1_;
    y1_ = y;

    return y;
}

void Biquad::reset() {
    x1_ = x2_ = y1_ = y2_ = 0.0f;
}
