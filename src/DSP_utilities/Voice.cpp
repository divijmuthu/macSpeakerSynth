#include "DSP_utilities/Voice.h"

Voice::Voice() {
    setCutoff(12000.0f, 48000.0f);
}

void Voice::noteOn() {
    // Fresh note from silence: align oscillator + filter so the first audible
    // samples ramp smoothly (arbitrary phase / stale filter state → clicks).
    if (!env_.isActive()) {
        osc_.resetPhase();
        filter_.reset();
    }
    env_.noteOn();
}

float Voice::nextSample() {
    const bool wasActive = env_.isActive();
    const float gain = env_.process();
    const float osc = osc_.nextSample();

    if (wasActive && !env_.isActive()) {
        filter_.reset();
    }
    if (gain <= 0.0f) {
        return 0.0f;
    }

    // Filter the raw oscillator, then apply envelope (VCA). Multiplying last
    // guarantees output hits true zero when gain is zero — filter memory alone
    // cannot leak audio or pop on retrigger.
    return filter_.process(osc) * gain;
}
