#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace bench {

struct Measurement {
    std::string benchmark;
    std::string variant;
    std::size_t size = 0;
    unsigned int threads = 1;
    double time_ms = 0.0;
    double metric = 0.0;
    std::string metric_name;
    std::uint64_t checksum = 0;
};

Measurement run_sieve(std::size_t limit);
Measurement run_quicksort(std::size_t count);
Measurement run_matrix(std::size_t dimension, unsigned int threads, bool parallel);
Measurement run_memory_access(std::size_t working_set_bytes, bool random_access);

}  // namespace bench
