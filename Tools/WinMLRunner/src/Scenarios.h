#pragma once

#include "common.h"

// load a model in a multi-threaded environment with num_threads number of
// threads Each thread will load a model once, with interval in milliseconds for
// each thread tasks
void ConcurrentLoadModel(const std::vector<std::wstring>& paths, unsigned num_threads, unsigned interval_milliseconds,
                         bool print_info);
