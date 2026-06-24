#include "ControlParser.h"

#include <cstdlib>

namespace {

float parseFrequencyField(std::string_view field) {
    // Second wire field only — e.g. "880.0" — copy to a small null-terminated buffer.
    char buffer[32];
    if (field.size() >= sizeof(buffer)) {
        return 0.0f;
    }
    for (std::size_t i = 0; i < field.size(); ++i) {
        buffer[i] = field[i];
    }
    buffer[field.size()] = '\0';
    return std::strtof(buffer, nullptr);
}

}  // namespace

bool parseControlMessage(std::string_view wire, ControlMessage& messageOut) {
    const std::size_t comma = wire.find(',');
    if (comma == std::string_view::npos) {
        return false;
    }

    const std::string_view state = wire.substr(0, comma);
    const std::string_view frequencyField = wire.substr(comma + 1);

    if (state == "NOTE_OFF") {
        messageOut.type = ControlType::NoteOff;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "NOTE_ON") {
        messageOut.type = ControlType::NoteOn;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "CUTOFF") {
        messageOut.type = ControlType::Cutoff;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "MODE") {
        messageOut.type = ControlType::FilterMode;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "DELAY") {
        messageOut.type = ControlType::DelayTime;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "FEEDBACK") {
        messageOut.type = ControlType::DelayFeedback;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "WET") {
        messageOut.type = ControlType::DelayWet;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "DRIVE") {
        messageOut.type = ControlType::Drive;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "WAVEFORM") {
        messageOut.type = ControlType::Waveform;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "GAIN") {
        messageOut.type = ControlType::MasterGain;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    if (state == "SIMD") {
        messageOut.type = ControlType::SimdMode;
        messageOut.frequencyHz = parseFrequencyField(frequencyField);
        return true;
    }

    return false;
}
