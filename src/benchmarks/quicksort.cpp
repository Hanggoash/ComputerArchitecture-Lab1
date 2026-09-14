#include "benchmark.h"

#include <chrono>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bench {
namespace {

void quicksort_impl(std::vector<std::int32_t>& values, std::ptrdiff_t low, std::ptrdiff_t high) {
    // Recurse into the smaller partition first, so a pathological input does
    // not consume linear stack space. The partition algorithm itself remains
    // the same textbook quicksort on every platform.
    while (low < high) {
        const std::int32_t pivot = values[low + (high - low) / 2];
        std::ptrdiff_t left = low;
        std::ptrdiff_t right = high;
        while (left <= right) {
            while (values[left] < pivot) {
                ++left;
            }
            while (values[right] > pivot) {
                --right;
            }
            if (left <= right) {
                std::swap(values[left], values[right]);
                ++left;
                --right;
            }
        }

        if (right - low < high - left) {
            if (low < right) {
                quicksort_impl(values, low, right);
            }
            low = left;
        } else {
            if (left < high) {
                quicksort_impl(values, left, high);
            }
            high = right;
        }
    }
}

bool is_sorted_non_decreasing(const std::vector<std::int32_t>& values) {
    for (std::size_t index = 1; index < values.size(); ++index) {
        if (values[index - 1] > values[index]) {
            return false;
        }
    }
    return true;
}

std::uint64_t checksum(const std::vector<std::int32_t>& values) {
    std::uint64_t total = 0;
    for (const std::int32_t value : values) {
        total += static_cast<std::uint32_t>(value);
    }
    return total;
}

}  // namespace

Measurement run_quicksort(const std::size_t count) {
    if (count == 0) {
        throw std::invalid_argument("Quicksort size must be greater than zero.");
    }

    std::mt19937 rng(12'345);
    std::vector<std::int32_t> original(count);
    for (std::int32_t& value : original) {
        // mt19937 has a specified sequence; this conversion avoids relying on
        // implementation-specific uniform_int_distribution behavior.
        value = static_cast<std::int32_t>(rng());
    }
    const std::uint64_t original_checksum = checksum(original);

    // The required input copy is deliberately outside the timed interval.
    std::vector<std::int32_t> values = original;
    const auto start = std::chrono::steady_clock::now();
    quicksort_impl(values, 0, static_cast<std::ptrdiff_t>(values.size() - 1));
    const auto finish = std::chrono::steady_clock::now();

    const std::uint64_t sorted_checksum = checksum(values);
    if (!is_sorted_non_decreasing(values) || sorted_checksum != original_checksum) {
        throw std::runtime_error("Quicksort correctness validation failed.");
    }

    Measurement result;
    result.benchmark = "quicksort";
    result.variant = "hoare";
    result.size = count;
    result.threads = 1;
    result.time_ms = std::chrono::duration<double, std::milli>(finish - start).count();
    result.metric = 0.0;
    result.metric_name = "none";
    result.checksum = sorted_checksum;
    return result;
}

}  // namespace bench
