#include "LogSetup.hpp"

void SetupLogger() {
  // Use the console as the default logger
  auto console = spdlog::stdout_color_mt("console");
  spdlog::set_default_logger(console);
}
