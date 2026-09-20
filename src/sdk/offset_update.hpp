#pragma once

// Runtime offset poller for https://www.cheatoffsets.com/api/games/cs2/current
// See src/sdk/offset_update/README.md

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class OffsetUpdate {
public:
    static OffsetUpdate& instance();

    void start();
    void stop();
    void tick();
    void request_poll();

    std::string status() const;
    bool live() const { return live_.load(std::memory_order_acquire); }
    ~OffsetUpdate();

private:
    OffsetUpdate() = default;
    OffsetUpdate(const OffsetUpdate&) = delete;
    OffsetUpdate& operator=(const OffsetUpdate&) = delete;

    void worker();
    void poll_once();
    void load_cache();
    std::string cache_path() const;
    void apply_flat(const std::unordered_map<std::string, std::ptrdiff_t>& flat,
                    const std::string& version, const std::string& updated);

    void* stop_event_ = nullptr;
    std::thread thread_;
    std::atomic<bool> running_{ false };
    std::atomic<bool> live_{ false };
    std::atomic<bool> force_poll_{ false };

    mutable std::mutex mu_;
    std::string etag_;
    std::string status_{ "idle · baked" };
    std::unordered_map<std::string, std::ptrdiff_t> pending_;
    std::string pending_version_;
    std::string pending_updated_;
    bool pending_ready_ = false;
};
