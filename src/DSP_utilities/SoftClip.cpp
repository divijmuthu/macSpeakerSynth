#include "DSP_utilities/SoftClip.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

}  // namespace

void SoftClip::setDrive(float drive) {
    // 1.0 = unity bypass; values above 1 add saturation.
    drive_ = std::clamp(drive, 1.0f, 8.0f);
}

float SoftClip::process(float input) const {
    if (drive_ <= 1.0f + 1e-6f) {
        return input;
    }
    return std::tanh(drive_ * input) / std::tanh(drive_);
}
