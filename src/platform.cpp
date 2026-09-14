#include "platform.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>

namespace bench {
namespace {

std::string trim_line(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ')) {
        value.pop_back();
    }
    return value;
}

std::string command_output(const char* command) {
    std::array<char, 256> buffer{};
    std::string output;
#if defined(_WIN32)
    FILE* pipe = _popen(command, "r");
#else
    FILE* pipe = popen(command, "r");
#endif
    if (pipe == nullptr) {
        return {};
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return trim_line(output);
}

std::tm local_time(std::time_t value) {
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &value);
#else
    localtime_r(&value, &result);
#endif
    return result;
}

}  // namespace

PlatformInfo detect_platform() {
    PlatformInfo info;
#if defined(__APPLE__)
    info.os = "macos";
#elif defined(_WIN32)
    info.os = "windows";
#else
    info.os = "unknown";
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    info.architecture = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    info.architecture = "x86_64";
#else
    info.architecture = "unknown";
#endif

    info.result_key = info.os + "-" + info.architecture;
    info.hardware_threads = std::thread::hardware_concurrency();
    if (info.hardware_threads == 0) {
        info.hardware_threads = 1;
    }

#if defined(__APPLE__)
    info.cpu_model = command_output("sysctl -n machdep.cpu.brand_string 2>/dev/null");
    if (info.cpu_model.empty()) {
        info.cpu_model = command_output("sysctl -n hw.model 2>/dev/null");
    }
#elif defined(_WIN32)
    info.cpu_model = command_output("wmic cpu get name /value 2>NUL");
#endif
    if (info.cpu_model.empty()) {
        info.cpu_model = "unavailable";
    }
    return info;
}

std::string iso_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    const std::tm local = local_time(seconds);
    std::ostringstream result;
    result << std::put_time(&local, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << milliseconds;
    return result.str();
}

std::string filename_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    const std::tm local = local_time(seconds);
    std::ostringstream result;
    result << std::put_time(&local, "%Y%m%d_%H%M%S")
           << '_' << std::setfill('0') << std::setw(3) << milliseconds;
    return result.str();
}

std::string csv_escape(const std::string& value) {
    std::string escaped = "\"";
    for (const char character : value) {
        if (character == '\"') {
            escaped += "\"\"";
        } else {
            escaped += character;
        }
    }
    escaped += "\"";
    return escaped;
}

std::filesystem::path project_root_from_current_directory() {
    // The benchmark is intended to be run from the repository root. This
    // fallback also supports running it from the CMake build directory.
    const auto current = std::filesystem::current_path();
    if (std::filesystem::exists(current / "results")) {
        return current;
    }
    if (std::filesystem::exists(current.parent_path() / "results")) {
        return current.parent_path();
    }
    return current;
}

}  // namespace bench
