#include "dbat/sysdep.h"
std::shared_ptr<spdlog::logger> logger;

std::random_device randomDevice;
std::default_random_engine randomEngine(randomDevice());

