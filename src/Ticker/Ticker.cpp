#include "Ticker.hpp"

// Unnamed namespace for local utilities
namespace {
std::chrono::milliseconds GetCurrentTimeStamp() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
}
}  // namespace

constexpr int tick_delay = 100;

std::mutex Ticker::global_mt;
std::thread Ticker::th;
std::vector<std::function<void(void)>> Ticker::callbacks;
bool Ticker::run = true;
std::chrono::milliseconds Ticker::delay = std::chrono::milliseconds(tick_delay);

void Ticker::Worker() {
  auto sleep_delay = delay;
  while (run) {
    std::this_thread::sleep_for(sleep_delay);
    std::lock_guard<std::mutex> lck(global_mt);
    auto init_time = GetCurrentTimeStamp();
    SPDLOG_TRACE("Ticker::worker : Starting loop for {} callbacks.",
                 callbacks.size());
    for (const auto &cb : callbacks) {
      cb();
    }

    // Make the callback intervals as consistent as possible via tracking the
    // time spent on invoking them
    auto current_time = GetCurrentTimeStamp();
    auto time_diff = current_time - init_time;
    auto sleep_delay = delay - time_diff;
    SPDLOG_TRACE("Ticker::worker : Loop took {}ms. Sleeping for {}ms.",
                 time_diff.count(), sleep_delay.count());
  }
}

void Ticker::Start() {
  std::lock_guard<std::mutex> lck(global_mt);

  // Initialize the thread
  if (!th.joinable()) {
    th = std::thread(&Ticker::Worker);
  }
}

void Ticker::Stop() {
  std::lock_guard<std::mutex> lck(global_mt);

  // Stop the thread and join
  run = false;
  th.join();
}

void Ticker::RegisterCallback(std::function<void(void)> cb) {
  std::lock_guard<std::mutex> lck(global_mt);
  callbacks.push_back(std::move(cb));
}
