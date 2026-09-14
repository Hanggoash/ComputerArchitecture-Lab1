#include "benchmark.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace bench {
namespace {

void multiply_rows(const std::vector<double>& a,
                   const std::vector<double>& b,
                   std::vector<double>& c,
                   const std::size_t dimension,
                   const std::size_t row_begin,
                   const std::size_t row_end) {
    // Deliberately retain the fixed i-j-k basic algorithm required by the
    // experiment specification. No blocking, BLAS, intrinsics, or platform
    // specialization is used.
    for (std::size_t i = row_begin; i < row_end; ++i) {
        for (std::size_t j = 0; j < dimension; ++j) {
            double sum = 0.0;
            for (std::size_t k = 0; k < dimension; ++k) {
                sum += a[i * dimension + k] * b[k * dimension + j];
            }
            c[i * dimension + j] = sum;
        }
    }
}

double reference_cell(const std::vector<double>& a,
                      const std::vector<double>& b,
                      const std::size_t dimension,
                      const std::size_t row,
                      const std::size_t column) {
    double sum = 0.0;
    for (std::size_t k = 0; k < dimension; ++k) {
        sum += a[row * dimension + k] * b[k * dimension + column];
    }
    return sum;
}

std::uint64_t matrix_checksum(const std::vector<double>& matrix) {
    std::uint64_t total = 0;
    for (const double value : matrix) {
        if (!std::isfinite(value) || value < 0.0) {
            throw std::runtime_error("Matrix multiplication produced an invalid value.");
        }
        total += static_cast<std::uint64_t>(value);
    }
    return total;
}

}  // namespace

Measurement run_matrix(const std::size_t dimension, const unsigned int threads, const bool parallel) {
    if (dimension == 0 || threads == 0) {
        throw std::invalid_argument("Matrix dimension and thread count must be greater than zero.");
    }
    if (dimension > static_cast<std::size_t>(-1) / dimension) {
        throw std::invalid_argument("Matrix dimension is too large.");
    }

    const std::size_t element_count = dimension * dimension;
    std::vector<double> a(element_count);
    std::vector<double> b(element_count);
    std::vector<double> c(element_count, 0.0);
    for (std::size_t index = 0; index < element_count; ++index) {
        // Small integer-valued doubles keep the checksum exact and stable.
        a[index] = static_cast<double>((index * 17U + 3U) % 13U);
        b[index] = static_cast<double>((index * 29U + 7U) % 11U);
    }

    const auto start = std::chrono::steady_clock::now();
    if (!parallel || threads == 1) {
        multiply_rows(a, b, c, dimension, 0, dimension);
    } else {
        const unsigned int active_threads = static_cast<unsigned int>(std::min<std::size_t>(threads, dimension));
        std::vector<std::thread> workers;
        workers.reserve(active_threads);
        for (unsigned int thread_index = 0; thread_index < active_threads; ++thread_index) {
            const std::size_t row_begin = dimension * thread_index / active_threads;
            const std::size_t row_end = dimension * (thread_index + 1) / active_threads;
            workers.emplace_back(multiply_rows,
                                 std::cref(a),
                                 std::cref(b),
                                 std::ref(c),
                                 dimension,
                                 row_begin,
                                 row_end);
        }
        for (std::thread& worker : workers) {
            worker.join();
        }
    }
    const auto finish = std::chrono::steady_clock::now();

    const std::size_t final_row = dimension - 1;
    const std::size_t final_column = dimension - 1;
    if (c[0] != reference_cell(a, b, dimension, 0, 0) ||
        c[final_row * dimension + final_column] != reference_cell(a, b, dimension, final_row, final_column)) {
        throw std::runtime_error("Matrix multiplication correctness validation failed.");
    }
    const std::uint64_t checksum = matrix_checksum(c);

    const double seconds = std::chrono::duration<double>(finish - start).count();
    Measurement result;
    result.benchmark = "matrix";
    result.variant = parallel && threads > 1 ? "parallel" : "serial";
    result.size = dimension;
    result.threads = parallel && threads > 1
                         ? static_cast<unsigned int>(std::min<std::size_t>(threads, dimension))
                         : 1;
    result.time_ms = seconds * 1'000.0;
    result.metric = (2.0 * static_cast<double>(dimension) * static_cast<double>(dimension) * static_cast<double>(dimension)) /
                    seconds / 1'000'000'000.0;
    result.metric_name = "gflops";
    result.checksum = checksum;
    return result;
}

}  // namespace bench
