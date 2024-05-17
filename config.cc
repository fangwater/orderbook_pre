#include "config.hpp"
#include <fstream>

json Config::open_json_file(std::string path_to_json) {
    std::ifstream config_file(path_to_json);
    if (!config_file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file: : {}", path_to_json));
    }
    json config_json;
    config_file >> config_json;
    return config_json;
}

Config &Config::instance() {
    static Config config;
    return config;
}

Config::Config() {
    json params = open_json_file("./config.json");
    org_counts_ = params["org_size"];
    org_partition_size_ = params["org_partition_size"];
    subject_matters_ = params["subject_matters"].get<std::vector<std::string>>();
    contrace_queue_size_ = params["contrace_queue_size"];
}

