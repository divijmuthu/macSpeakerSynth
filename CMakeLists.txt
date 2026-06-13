cmake_minimum_required(VERSION 3.16)
project(race_synth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- miniaudio (vendored header — see third_party/miniaudio/) ---
add_library(miniaudio_iface INTERFACE)
target_include_directories(miniaudio_iface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
)

# --- DSP library ---
add_library(race_dsp
    src/DSP_utilities/Oscillator.cpp
    src/DSP_utilities/Envelope.cpp
    src/DSP_utilities/Voice.cpp
)
target_include_directories(race_dsp PUBLIC include)

# --- Lab tests ---
add_executable(test_lab01 tests-and-benchmarks/test_lab01.cpp)
target_link_libraries(test_lab01 PRIVATE race_dsp)

add_executable(test_lab02 tests-and-benchmarks/test_lab02.cpp)
target_link_libraries(test_lab02 PRIVATE race_dsp)

add_executable(test_lab03 tests-and-benchmarks/test_lab03.cpp)
target_link_libraries(test_lab03 PRIVATE race_dsp AudioCore)

# --- Lab 03: audio engine ---
add_library(AudioCore src/AudioCore.cpp)
target_include_directories(AudioCore PUBLIC include)
target_link_libraries(AudioCore PUBLIC race_dsp miniaudio_iface)

add_executable(race_synth src/main.cpp)
target_link_libraries(race_synth PRIVATE AudioCore)

# Lab 04+: ZMQ in main.cpp
# Lab 05+: Biquad.cpp in race_dsp
