#pragma once

#include <filesystem>
#include <string>

namespace bench {

struct PlatformInfo {
    std::string os;
    std::string architecture;
    std::string result_key;
    std::string cpu_model;
    unsigned int hardware_threads;
};

PlatformInfo detect_platform();
std::string iso_timestamp();
std::string filename_timestamp();
std::string csv_escape(const std::string& value);
std::filesystem::path project_root_from_current_directory();

}  // namespace bench
