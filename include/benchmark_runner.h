#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace bench {

struct RunOptions {
    std::string benchmark = "all";
    unsigned int warmup = 1;
    unsigned int repeat = 5;
    std::optional<std::size_t> size;
    std::optional<unsigned int> threads;
};

class BenchmarkRunner {
public:
    explicit BenchmarkRunner(RunOptions options);
    void run();

private:
    RunOptions options_;
};

}  // namespace bench
