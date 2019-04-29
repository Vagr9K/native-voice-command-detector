#pragma once

#include <spdlog/spdlog.h>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// A static class that invokes synchronization callbacks at fixed intervals
class Ticker {
 private:
  // Lock
  static std::mutex global_mt;
  // Thread handle
  static std::thread th;
  // List of callbacks
  static std::vector<std::function<void(void)>> callbacks;
  // Start/stop toggle
  static bool run;
  // Next delay between the invokations
  static std::chrono::milliseconds delay;

  // The thread worker responsible for invoking the callbaks
  static void Worker();

 public:
  // Start the callback invokation
  static void Start();
  // Stop the callback invokation
  static void Stop();
  // Register a sync callback
  static void RegisterCallback(std::function<void(void)> cb);
};
