#include "benchmark.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace bench {

Measurement run_memory_access(const std::size_t working_set_bytes, const bool random_access) {
    if (working_set_bytes < sizeof(std::uint32_t)) {
        throw std::invalid_argument("Memory working set is too small.");
    }
    const std::size_t element_count = working_set_bytes / sizeof(std::uint32_t);
    if (element_count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Memory working set is too large for reproducible indices.");
    }

    // The data and, for random access, the entire random index sequence are
    // created before timing. mt19937 plus modulo yields identical inputs on
    // the two standard-library implementations.
    std::mt19937 rng(12'345);
    std::vector<std::uint32_t> values(element_count);
    for (std::uint32_t& value : values) {
        value = rng();
    }

    constexpr std::size_t kMinimumAccesses = 4U * 1024U * 1024U;
    const std::size_t access_count = std::max(element_count, kMinimumAccesses);
    std::vector<std::uint32_t> indices;
    if (random_access) {
        indices.resize(access_count);
        for (std::uint32_t& index : indices) {
            index = rng() % static_cast<std::uint32_t>(element_count);
        }
    }

    std::uint64_t checksum = 0;
    const auto start = std::chrono::steady_clock::now();
    if (random_access) {
        for (const std::uint32_t index : indices) {
            checksum += values[index];
        }
    } else {
        std::size_t remaining = access_count;
        while (remaining > 0) {
            const std::size_t pass_length = std::min(remaining, element_count);
            for (std::size_t index = 0; index < pass_length; ++index) {
                checksum += values[index];
            }
            remaining -= pass_length;
        }
    }
    const auto finish = std::chrono::steady_clock::now();

    const double seconds = std::chrono::duration<double>(finish - start).count();
    Measurement result;
    result.benchmark = "memory";
    result.variant = random_access ? "random" : "sequential";
    result.size = working_set_bytes;
    result.threads = 1;
    result.time_ms = seconds * 1'000.0;
    result.metric = static_cast<double>(access_count) / seconds / 1'000'000.0;
    result.metric_name = "million_accesses_per_s";
    result.checksum = checksum;
    return result;
}

}  // namespace bench
