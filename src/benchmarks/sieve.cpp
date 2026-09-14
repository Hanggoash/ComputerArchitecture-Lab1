#include "benchmark.h"

#include <chrono>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace bench {

Measurement run_sieve(const std::size_t limit) {
    if (limit < 2) {
        throw std::invalid_argument("Sieve size must be at least 2.");
    }

    // Allocation and initialization intentionally happen before timing.
    std::vector<unsigned char> is_prime(limit + 1, 1);
    is_prime[0] = 0;
    is_prime[1] = 0;

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t candidate = 2; candidate <= limit / candidate; ++candidate) {
        if (is_prime[candidate] == 0) {
            continue;
        }
        for (std::size_t multiple = candidate * candidate; multiple <= limit; multiple += candidate) {
            is_prime[multiple] = 0;
        }
    }
    const auto finish = std::chrono::steady_clock::now();

    std::uint64_t prime_count = 0;
    for (const unsigned char value : is_prime) {
        prime_count += value;
    }

    const bool basic_validation = is_prime[2] != 0 && is_prime[3] != 0 && is_prime[4] == 0 && is_prime[limit] <= 1;
    const std::unordered_map<std::size_t, std::uint64_t> expected_counts{
        {1'000'000, 78'498},
        {5'000'000, 348'513},
        {10'000'000, 664'579},
        {50'000'000, 3'001'134},
    };
    const auto expected = expected_counts.find(limit);
    if (!basic_validation || (expected != expected_counts.end() && expected->second != prime_count)) {
        throw std::runtime_error("Sieve correctness validation failed.");
    }

    Measurement result;
    result.benchmark = "sieve";
    result.variant = "eratosthenes";
    result.size = limit;
    result.threads = 1;
    result.time_ms = std::chrono::duration<double, std::milli>(finish - start).count();
    result.metric = 0.0;
    result.metric_name = "none";
    result.checksum = prime_count;
    return result;
}

}  // namespace bench
