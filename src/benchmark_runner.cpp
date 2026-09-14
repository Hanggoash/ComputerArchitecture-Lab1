#include "benchmark_runner.h"

#include "benchmark.h"
#include "build_config.h"
#include "platform.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bench {
namespace {

class CsvWriter {
public:
    CsvWriter(const PlatformInfo& platform, const std::filesystem::path& root)
        : platform_(platform) {
        const std::filesystem::path raw_directory = root / "results" / platform.result_key / "raw";
        std::filesystem::create_directories(raw_directory);

        const std::string stem = unique_stem(raw_directory, "run_" + filename_timestamp());
        csv_path_ = raw_directory / (stem + ".csv");
        metadata_path_ = raw_directory / ("metadata_" + stem.substr(4) + ".json");
        stream_.open(csv_path_);
        if (!stream_) {
            throw std::runtime_error("Unable to create raw CSV: " + csv_path_.string());
        }
        stream_ << "timestamp,platform,arch,benchmark,variant,size,threads,run,time_ms,metric,metric_name,checksum\n";
        write_metadata();
    }

    void write(const Measurement& measurement, const unsigned int run_number) {
        stream_ << csv_escape(iso_timestamp()) << ','
                << csv_escape(platform_.os) << ','
                << csv_escape(platform_.architecture) << ','
                << csv_escape(measurement.benchmark) << ','
                << csv_escape(measurement.variant) << ','
                << measurement.size << ','
                << measurement.threads << ','
                << run_number << ','
                << std::fixed << std::setprecision(6) << measurement.time_ms << ','
                << std::setprecision(10) << measurement.metric << ','
                << csv_escape(measurement.metric_name) << ','
                << measurement.checksum << '\n';
        stream_.flush();
    }

    const std::filesystem::path& csv_path() const {
        return csv_path_;
    }

private:
    static std::string unique_stem(const std::filesystem::path& directory, const std::string& proposed) {
        std::string candidate = proposed;
        unsigned int suffix = 1;
        while (std::filesystem::exists(directory / (candidate + ".csv"))) {
            candidate = proposed + "_" + std::to_string(suffix++);
        }
        return candidate;
    }

    static std::string json_escape(const std::string& value) {
        std::string escaped;
        for (const char character : value) {
            switch (character) {
                case '\\': escaped += "\\\\"; break;
                case '\"': escaped += "\\\""; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += character; break;
            }
        }
        return escaped;
    }

    void write_metadata() const {
        std::ofstream metadata(metadata_path_);
        if (!metadata) {
            throw std::runtime_error("Unable to create metadata JSON: " + metadata_path_.string());
        }
        metadata << "{\n"
                 << "  \"timestamp\": \"" << json_escape(iso_timestamp()) << "\",\n"
                 << "  \"raw_data_file\": \"" << json_escape(csv_path_.filename().string()) << "\",\n"
                 << "  \"os\": \"" << json_escape(platform_.os) << "\",\n"
                 << "  \"architecture\": \"" << json_escape(platform_.architecture) << "\",\n"
                 << "  \"result_directory\": \"" << json_escape(platform_.result_key) << "\",\n"
                 << "  \"cpu_model\": \"" << json_escape(platform_.cpu_model) << "\",\n"
                 << "  \"hardware_concurrency\": " << platform_.hardware_threads << ",\n"
                 << "  \"compiler\": \"" << json_escape(BENCHMARK_COMPILER) << "\",\n"
                 << "  \"compiler_version\": \"" << json_escape(BENCHMARK_COMPILER_VERSION) << "\",\n"
                 << "  \"cxx_standard\": \"" << json_escape(BENCHMARK_CXX_STANDARD) << "\",\n"
                 << "  \"build_type\": \"" << json_escape(BENCHMARK_BUILD_TYPE) << "\",\n"
                 << "  \"compiler_flags\": \"" << json_escape(BENCHMARK_COMPILER_FLAGS) << "\",\n"
                 << "  \"git_commit\": \"" << json_escape(BENCHMARK_GIT_COMMIT) << "\",\n"
                 << "  \"benchmark_version\": \"" << json_escape(BENCHMARK_VERSION) << "\"\n"
                 << "}\n";
    }

    PlatformInfo platform_;
    std::filesystem::path csv_path_;
    std::filesystem::path metadata_path_;
    std::ofstream stream_;
};

std::vector<std::size_t> selected_sizes(const std::optional<std::size_t>& requested,
                                        const std::vector<std::size_t>& defaults) {
    return requested ? std::vector<std::size_t>{*requested} : defaults;
}

std::vector<unsigned int> default_thread_counts(const unsigned int hardware_threads) {
    std::vector<unsigned int> counts{1};
    for (unsigned int count = 2; count < hardware_threads; count *= 2) {
        counts.push_back(count);
        if (count > std::numeric_limits<unsigned int>::max() / 2) {
            break;
        }
    }
    if (hardware_threads > 1 && counts.back() != hardware_threads) {
        counts.push_back(hardware_threads);
    }
    return counts;
}

void execute_measurement(CsvWriter& writer,
                         const RunOptions& options,
                         const std::string& label,
                         const std::function<Measurement()>& work) {
    std::cout << "  " << label << std::flush;
    for (unsigned int warmup = 0; warmup < options.warmup; ++warmup) {
        static_cast<void>(work());
    }
    for (unsigned int run = 1; run <= options.repeat; ++run) {
        Measurement measurement = work();
        writer.write(measurement, run);
        std::cout << " [" << run << ": " << std::fixed << std::setprecision(2)
                  << measurement.time_ms << " ms]" << std::flush;
    }
    std::cout << '\n';
}

bool runs(const RunOptions& options, const std::string& name) {
    return options.benchmark == "all" || options.benchmark == name;
}

}  // namespace

BenchmarkRunner::BenchmarkRunner(RunOptions options) : options_(std::move(options)) {}

void BenchmarkRunner::run() {
    const PlatformInfo platform = detect_platform();
    std::cout << "Platform: " << platform.result_key << " | CPU: " << platform.cpu_model
              << " | hardware threads: " << platform.hardware_threads << '\n';
    std::cout << "Profile: baseline | compiler: " << BENCHMARK_COMPILER << ' ' << BENCHMARK_COMPILER_VERSION
              << " | flags: " << BENCHMARK_COMPILER_FLAGS << '\n';

    CsvWriter writer(platform, project_root_from_current_directory());
    std::cout << "Writing raw measurements to " << writer.csv_path().string() << '\n';

    if (runs(options_, "sieve")) {
        const std::vector<std::size_t> sizes = selected_sizes(
            options_.size, {1'000'000, 5'000'000, 10'000'000, 50'000'000});
        for (const std::size_t size : sizes) {
            execute_measurement(writer, options_, "sieve size=" + std::to_string(size), [size] {
                return run_sieve(size);
            });
        }
    }

    if (runs(options_, "quicksort")) {
        const std::vector<std::size_t> sizes = selected_sizes(
            options_.size, {100'000, 1'000'000, 5'000'000, 10'000'000});
        for (const std::size_t size : sizes) {
            execute_measurement(writer, options_, "quicksort size=" + std::to_string(size), [size] {
                return run_quicksort(size);
            });
        }
    }

    if (runs(options_, "matrix")) {
        const std::vector<std::size_t> sizes = selected_sizes(options_.size, {128, 256, 512, 1024});
        if (options_.threads) {
            const bool parallel = *options_.threads > 1;
            for (const std::size_t size : sizes) {
                execute_measurement(writer,
                                    options_,
                                    "matrix size=" + std::to_string(size) + " threads=" + std::to_string(*options_.threads),
                                    [size, parallel, threads = *options_.threads] {
                                        return run_matrix(size, threads, parallel);
                                    });
            }
        } else {
            for (const std::size_t size : sizes) {
                execute_measurement(writer, options_, "matrix serial size=" + std::to_string(size), [size] {
                    return run_matrix(size, 1, false);
                });
            }

            // The default multicore sweep uses one fixed, report-friendly
            // problem size; serial 512 is already in the measurements above.
            const std::size_t scaling_size = options_.size ? *options_.size : 512;
            for (const unsigned int threads : default_thread_counts(platform.hardware_threads)) {
                if (threads == 1) {
                    continue;
                }
                execute_measurement(writer,
                                    options_,
                                    "matrix parallel size=" + std::to_string(scaling_size) + " threads=" + std::to_string(threads),
                                    [scaling_size, threads] {
                                        return run_matrix(scaling_size, threads, true);
                                    });
            }
        }
    }

    if (runs(options_, "memory")) {
        const std::vector<std::size_t> sizes = selected_sizes(
            options_.size,
            {32U * 1024U,
             64U * 1024U,
             256U * 1024U,
             1U * 1024U * 1024U,
             4U * 1024U * 1024U,
             16U * 1024U * 1024U,
             64U * 1024U * 1024U,
             256U * 1024U * 1024U});
        for (const std::size_t size : sizes) {
            execute_measurement(writer, options_, "memory sequential bytes=" + std::to_string(size), [size] {
                return run_memory_access(size, false);
            });
            execute_measurement(writer, options_, "memory random bytes=" + std::to_string(size), [size] {
                return run_memory_access(size, true);
            });
        }
    }

    std::cout << "Benchmark run complete. Raw data was preserved in " << writer.csv_path().string() << '\n';
}

}  // namespace bench
