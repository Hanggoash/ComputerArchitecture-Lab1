#include "benchmark_runner.h"

#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable << " [--benchmark all|sieve|quicksort|matrix|memory]"
              << " [--warmup N] [--repeat N] [--size N] [--threads N]\n\n"
              << "Defaults: --benchmark all --warmup 1 --repeat 5\n"
              << "--size is the benchmark's native unit: elements for sieve/quicksort,\n"
              << "matrix dimension for matrix, and bytes for memory.\n";
}

template <typename Number>
Number parse_positive(const std::string& text, const std::string& option) {
    std::size_t consumed = 0;
    unsigned long long value = 0;
    try {
        value = std::stoull(text, &consumed, 10);
    } catch (const std::exception&) {
        throw std::invalid_argument(option + " requires a positive integer.");
    }
    if (consumed != text.size() || value == 0 || value > std::numeric_limits<Number>::max()) {
        throw std::invalid_argument(option + " requires a positive integer in range.");
    }
    return static_cast<Number>(value);
}

template <typename Number>
Number parse_nonnegative(const std::string& text, const std::string& option) {
    std::size_t consumed = 0;
    unsigned long long value = 0;
    try {
        value = std::stoull(text, &consumed, 10);
    } catch (const std::exception&) {
        throw std::invalid_argument(option + " requires a non-negative integer.");
    }
    if (consumed != text.size() || value > std::numeric_limits<Number>::max()) {
        throw std::invalid_argument(option + " requires a non-negative integer in range.");
    }
    return static_cast<Number>(value);
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        bench::RunOptions options;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--help" || argument == "-h") {
                print_usage(argv[0]);
                return 0;
            }
            if (index + 1 >= argc) {
                throw std::invalid_argument("Missing value for " + argument + ".");
            }
            const std::string value = argv[++index];
            if (argument == "--benchmark") {
                options.benchmark = value;
            } else if (argument == "--warmup") {
                options.warmup = parse_nonnegative<unsigned int>(value, argument);
            } else if (argument == "--repeat") {
                options.repeat = parse_positive<unsigned int>(value, argument);
            } else if (argument == "--size") {
                options.size = parse_positive<std::size_t>(value, argument);
            } else if (argument == "--threads") {
                options.threads = parse_positive<unsigned int>(value, argument);
            } else {
                throw std::invalid_argument("Unknown option: " + argument);
            }
        }

        if (options.benchmark != "all" && options.benchmark != "sieve" && options.benchmark != "quicksort" &&
            options.benchmark != "matrix" && options.benchmark != "memory") {
            throw std::invalid_argument("--benchmark must be all, sieve, quicksort, matrix, or memory.");
        }

        bench::BenchmarkRunner(std::move(options)).run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
