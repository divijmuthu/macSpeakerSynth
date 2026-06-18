#include "AudioCore.h"
#include "ControlParser.h"
#include "LockFreeQueue.h"

#include <zmq.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

// local endpoint for ZMQ subscriber to bind to
constexpr const char* kZmqEndpoint = "tcp://127.0.0.1:5555";
constexpr const char* kScopeEndpoint = "tcp://127.0.0.1:5556";
constexpr std::size_t kScopePayloadBytes = AudioCore::kScopeSize * sizeof(float) * 2;

// global queue for control messages from ZMQ
LockFreeQueue<AudioCore::kControlQueueCapacity> g_controlQueue;
// global running flag to signal the main thread to quit
std::atomic<bool> g_running{true};
AudioCore* g_engine = nullptr;

void scopePublisherThread() {
    void* ctx = zmq_ctx_new();
    void* pub = zmq_socket(ctx, ZMQ_PUB);
    zmq_bind(pub, kScopeEndpoint);

    float output[AudioCore::kScopeSize];
    float envelope[AudioCore::kScopeSize];
    std::vector<char> payload(4 + kScopePayloadBytes);

    while (g_running.load(std::memory_order_acquire)) {
        if (g_engine != nullptr) {
            g_engine->copyScopeSnapshot(output, envelope);
            std::memcpy(payload.data(), "SCOP", 4);
            std::memcpy(payload.data() + 4, output, AudioCore::kScopeSize * sizeof(float));
            std::memcpy(payload.data() + 4 + AudioCore::kScopeSize * sizeof(float),
                        envelope,
                        AudioCore::kScopeSize * sizeof(float));
            zmq_send(pub, payload.data(), payload.size(), 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    zmq_close(pub);
    zmq_ctx_destroy(ctx);
}

void zmqSubscriberThread() {
    // TODO (Lab 04, Q5): ZeroMQ subscriber loop (producer → g_controlQueue).
    // create a new context for ZMQ
    void* ctx = zmq_ctx_new();
    // create a new subscriber socket
    void* sub = zmq_socket(ctx, ZMQ_SUB);
    // bind the subscriber socket to the local endpoint
    zmq_bind(sub, kZmqEndpoint);
    // subscribe to all topics
    zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);
    // while the main thread is running, keep taking incoming messages + pushing to global queue based on successful parsing
    while (g_running.load(std::memory_order_acquire)) {
        // receive a message from the subscriber socket
        char buf[256];
        int n = zmq_recv(sub, buf, sizeof(buf) - 1, 0);
        if (n <= 0) continue;
        buf[n] = '\0';
        ControlMessage msg;
        if (parseControlMessage(buf, msg)) {
            g_controlQueue.push(msg);
        }
    }
    // close the subscriber socket
    zmq_close(sub);
    // destroy the context
    zmq_ctx_destroy(ctx);
}

}  // namespace

int main() {
    std::printf("RACE synth — Lab 09 (Ctrl+C to quit)\n");
    std::printf("Start Python UI: python py_interface/main.py\n");

    AudioCore engine(&g_controlQueue);
    g_engine = &engine;
    // Lab 04 status: wait for NOTE_ON from Python — do not hardcode noteOn() here. Listen for ZMQ messages and queue them.
    if (!engine.start()) {
        std::cerr << "Failed to start audio device.\n";
        return 1;
    }
    // thread for zmq listener/subscriber to get incoming commands and queue them
    std::thread zmqThread(zmqSubscriberThread);
    std::thread scopeThread(scopePublisherThread);
    std::printf("Audio running. Control %s  Scope %s\n", kZmqEndpoint, kScopeEndpoint);
    // main thread for audio engine to run and process commands from the queue
    while (g_running.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // join the zmq thread to the main thread to wait for it to finish
    if (zmqThread.joinable()) {
        zmqThread.join();
    }
    if (scopeThread.joinable()) {
        scopeThread.join();
    }
    g_engine = nullptr;
    // finished, quit
    engine.stop();
    return 0;
}
