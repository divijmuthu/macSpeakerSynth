#pragma once

#include "ControlMessage.h"

#include <array>
#include <atomic>
#include <cstddef>

// Lab 04: Single-Producer Single-Consumer (SPSC) lock-free ring buffer.
//
//   Producer (ZMQ std::thread)  ──push──►  [ ring ]  ──pop──►  Consumer (audio callback)
//
// One thread calls push(), one thread calls pop() — never both on the same side.
// See learning/lab04/README.md.

template <std::size_t Capacity>
class LockFreeQueue {
    static_assert(Capacity >= 2, "SPSC ring needs at least 2 slots");

public:
    // Producer only (ZMQ thread). Returns false if queue is full (message dropped).
    bool push(const ControlMessage& message) {
        // TODO (Lab 04, Q1): SPSC push — producer side.
        //
        // 1. Load write_idx_ relaxed.
        // 2. next = (write + 1) % Capacity.
        // 3. If next == read_idx_.load(acquire) → full, return false.
        // 4. buffer_[write] = message;
        // 5. write_idx_.store(next, release); return true.
        //
        std::size_t curr_idx = write_idx_.load(std::memory_order_relaxed);
        std::size_t next_idx = (curr_idx + 1) % Capacity;
        // Full when advancing write would collide with read (one slot reserved).
        if (next_idx == read_idx_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[curr_idx] = message;
        write_idx_.store(next_idx, std::memory_order_release);
        return true;
    }

    // Consumer only (audio thread). Returns false if queue is empty.
    bool pop(ControlMessage& messageOut) {
        // TODO (Lab 04, Q2): SPSC pop — consumer side.
        //
        // 1. Load read_idx_ relaxed.
        // 2. If read == write_idx_.load(acquire) → empty, return false.
        // 3. messageOut = buffer_[read];
        // 4. read_idx_.store((read + 1) % Capacity, release); return true.
        //
        std::size_t curr_idx = read_idx_.load(std::memory_order_relaxed);
        // failure case is an empty queue i.e. same as write index 
        if (curr_idx == write_idx_.load(std::memory_order_acquire)) {
            // cannot read, queue is empty
            return false;
        }
        messageOut = buffer_[curr_idx];
        read_idx_.store((curr_idx+1) % Capacity, std::memory_order_release);
        return true;
    }

    std::size_t capacity() const { return Capacity; }

private:
    std::array<ControlMessage, Capacity> buffer_{};
    std::atomic<std::size_t> write_idx_{0};
    std::atomic<std::size_t> read_idx_{0};
};
