cmake_minimum_required(VERSION 3.16)
project(race_synth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(RACE_USE_COREAUDIO "Use macOS Core Audio HAL (Apple only)" ON)
option(RACE_ENABLE_NEON "Enable ARM NEON oscillator batch path" ON)

# --- ZeroMQ (Lab 04) ---
find_path(ZMQ_INCLUDE_DIR NAMES zmq.h
    HINTS /opt/homebrew/include /usr/local/include)
find_library(ZMQ_LIBRARY NAMES zmq
    HINTS /opt/homebrew/lib /usr/local/lib)
if(NOT ZMQ_INCLUDE_DIR OR NOT ZMQ_LIBRARY)
    message(FATAL_ERROR
        "ZeroMQ not found. Install with: brew install zeromq")
endif()

# --- miniaudio (vendored header — see third_party/miniaudio/) ---
add_library(miniaudio_iface INTERFACE)
target_include_directories(miniaudio_iface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
)

# --- DSP library ---
add_library(race_dsp
    src/DSP_utilities/Oscillator.cpp
    src/DSP_utilities/Waveform.cpp
    src/DSP_utilities/SimdOscillator.cpp
    src/DSP_utilities/Envelope.cpp
    src/DSP_utilities/Voice.cpp
    src/DSP_utilities/Biquad.cpp
    src/DSP_utilities/StateVariableFilter.cpp
    src/DSP_utilities/DelayLine.cpp
    src/DSP_utilities/SoftClip.cpp
)
target_include_directories(race_dsp PUBLIC include)

if(RACE_ENABLE_NEON AND (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|ARM64"))
    target_compile_definitions(race_dsp PUBLIC RACE_NEON=1)
    message(STATUS "RACE: ARM NEON batch oscillator enabled")
endif()

# --- Control parsing (Lab 04) ---
add_library(race_control src/ControlParser.cpp)
target_include_directories(race_control PUBLIC include)

# --- Audio engine (Lab 03 / 11) ---
add_library(AudioCore
    src/AudioCore.cpp
    src/platform/MiniaudioOutput.cpp
)
target_include_directories(AudioCore PUBLIC include)
target_link_libraries(AudioCore PUBLIC race_dsp miniaudio_iface)

if(APPLE AND RACE_USE_COREAUDIO)
    enable_language(OBJCXX)
    target_compile_definitions(AudioCore PRIVATE RACE_USE_COREAUDIO=1)
    target_sources(AudioCore PRIVATE src/platform/CoreAudioOutput.mm)
    target_link_libraries(AudioCore PUBLIC
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudio"
    )
    message(STATUS "RACE: macOS Core Audio backend enabled")
else()
    message(STATUS "RACE: miniaudio backend (default)")
endif()

# --- Lab tests ---
add_executable(test_lab01 tests-and-benchmarks/test_lab01.cpp)
target_link_libraries(test_lab01 PRIVATE race_dsp)

add_executable(test_lab02 tests-and-benchmarks/test_lab02.cpp)
target_link_libraries(test_lab02 PRIVATE race_dsp)

add_executable(test_lab03 tests-and-benchmarks/test_lab03.cpp)
target_link_libraries(test_lab03 PRIVATE race_dsp AudioCore)

add_executable(test_lab04 tests-and-benchmarks/test_lab04.cpp)
target_link_libraries(test_lab04 PRIVATE race_dsp race_control AudioCore)

add_executable(test_lab05 tests-and-benchmarks/test_lab05.cpp)
target_link_libraries(test_lab05 PRIVATE race_dsp race_control AudioCore)

add_executable(test_lab06 tests-and-benchmarks/test_lab06.cpp)
target_link_libraries(test_lab06 PRIVATE race_dsp race_control AudioCore)

add_executable(test_lab07 tests-and-benchmarks/test_lab07.cpp)
target_link_libraries(test_lab07 PRIVATE race_dsp race_control AudioCore)

add_executable(test_lab08 tests-and-benchmarks/test_lab08.cpp)
target_link_libraries(test_lab08 PRIVATE race_dsp race_control AudioCore)

add_executable(test_lab09 tests-and-benchmarks/test_lab09.cpp)
target_link_libraries(test_lab09 PRIVATE race_dsp race_control)

add_executable(benchmark_neon tests-and-benchmarks/benchmark_neon.cpp)
target_link_libraries(benchmark_neon PRIVATE race_dsp)

# --- Synth + ZMQ ---
add_executable(race_synth src/main.cpp)
target_include_directories(race_synth PRIVATE ${ZMQ_INCLUDE_DIR})
target_link_libraries(race_synth PRIVATE AudioCore race_control ${ZMQ_LIBRARY})
