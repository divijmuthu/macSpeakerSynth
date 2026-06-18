#pragma once

// Lab 07: Soft saturation / waveshaping (master bus).
// See learning/lab07/README.md.

class SoftClip {
public:
    void setDrive(float drive);

    float process(float input) const;
    float drive() const { return drive_; }

private:
    float drive_ = 1.0f;
};
