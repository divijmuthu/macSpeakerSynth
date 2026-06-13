#include "DSP_utilities/Envelope.h"

#include <algorithm>
#include <cmath>

void Envelope::configure(float attackSec,
                         float decaySec,
                         float sustainLevel,
                         float releaseSec,
                         float sampleRate) {
    attackSec_ = attackSec;
    decaySec_ = decaySec;
    sustainLevel_ = sustainLevel;
    releaseSec_ = releaseSec;
    sampleRate_ = sampleRate;
    // TODO (Lab 02, Q1): Precompute per-sample deltas for Attack and Decay only.
    //
    //   attackDelta_ = 1.0f / (attackSec_ * sampleRate_);
    //   decayDelta_  = (1.0f - sustainLevel_) / (decaySec_ * sampleRate_);
    //
    // releaseDelta_ is computed in noteOff() from the current level (Q2b).
    //
    attackDelta_ = 1.0f/(attackSec_ * sampleRate_);  // 1/attackSamples to evenly spread amplitude gain over attack time
    decayDelta_ = (1.0f - sustainLevel_)/(decaySec_ * sampleRate_);   // same idea for decay
}

void Envelope::noteOn() {
    // TODO (Lab 02, Q2a): Start a new note from Attack.
    // - Set stage_ to Attack
    // - Reset level_ to 0.0f (clean retrigger; avoids clicks from leftover level)
    this->stage_ = EnvelopeStage::Attack;
    this->level_ = 0.0f;
}

void Envelope::noteOff() {
    // TODO (Lab 02, Q2b): Begin the release phase.
    // - If we are Idle, nothing to do (return early).
    // - Otherwise set stage_ to Release.
    //   (Do NOT snap level to zero — Release ramps from the current level.)
    //
    this->stage_ = EnvelopeStage::Release;
    this->releaseDelta_ = this->level_ / (releaseSec_ * sampleRate_); // compute release based on curr level which could vary
}

float Envelope::process() {
    // TODO (Lab 02, Q3): Implement the ADSR state machine for ONE sample.
    //
    // Attack:   level_ += attackDelta_;  clamp to 1.0; when >= 1.0 → Decay
    // Decay:    level_ -= decayDelta_;   clamp to sustainLevel_; when <= sustain → Sustain
    // Sustain:  hold level_ (no change)
    // Release:  level_ -= releaseDelta_; clamp to 0; when <= 0 → Idle
    // Idle:     level_ = 0
    //
    // Return level_ at the end.
    // ig the breaks are just to speed things up once we finish a case? isnt this implied?
    switch (this->stage_) {
        // currently attacking --> bump level, if level exceeds 1.0, transition to decay
        case EnvelopeStage::Attack: 
            this->level_ += this->attackDelta_;
            if (this->level_ >= 1.0f) {
                this->stage_ = EnvelopeStage::Decay;
                this->level_ = 1.0f;
            }
            break;
        // currently decaying --> drop level, if level reaches sustain, transition to sustain
        case EnvelopeStage::Decay:
            this->level_ -= this->decayDelta_;
            if (this->level_ <= this->sustainLevel_) {
                this->stage_ = EnvelopeStage::Sustain;
            }
            break;
        case EnvelopeStage::Sustain:
            break; // dont do anything lol  
        case EnvelopeStage::Release:
            this->level_ -= this->releaseDelta_;
            if (this->level_ <= 0.0f) {
                // done with the note, go to idle
                this->stage_ = EnvelopeStage::Idle;
                this->level_ = 0.0f;
            }
            break;
        case EnvelopeStage::Idle:
            this->level_ = 0.0f;
            break;
    }
    return this->level_;
}
