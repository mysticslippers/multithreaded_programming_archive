#pragma once

#include <vector>

int get_tid();

int run_threads(const std::vector<int>& values, int consumer_count,
                int sleep_limit_ms, bool debug);