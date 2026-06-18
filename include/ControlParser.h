#pragma once

#include "ControlMessage.h"

#include <string_view>

// Lab 04 wire protocol (Python PUB → C++ SUB):
//
//   NOTE_ON,<frequency>   e.g. NOTE_ON,440.0
//   NOTE_OFF,<hz|0>       0 = all off; else match pitch (Lab 08)
//   CUTOFF,<hz>           e.g. CUTOFF,1200.0   (Lab 05)
//   MODE,<0|1|2>          0=LP, 1=HP, 2=BP     (Lab 06)
//   DELAY,<seconds>       FEEDBACK,<0-0.95>  WET,<0-1>  DRIVE,<gain>  (Lab 07)
//   WAVEFORM,<0|1|2|3>     0=sine 1=saw 2=square 3=triangle  (Lab 09)
//
// Two fields separated by one comma — state first, number second.
// No JSON; C++ splits on ',' and reads the second field as float.

bool parseControlMessage(std::string_view wire, ControlMessage& messageOut);
