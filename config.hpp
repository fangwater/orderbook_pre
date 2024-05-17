#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <fmt/format.h>
/**
 * @brief 存储全局的配置信息
 * 方便其他位置进行读取
 */
using json = nlohmann::json;
class Config {
public:
    static Config &instance();

private:
    static json open_json_file(std::string path_to_json);
    Config();

public:
    int org_counts_;//市场机构数
    int org_partition_size_;  //机构分区单元
    int contrace_queue_size_;//合约内主缓存队列大小
    std::vector<std::string> subject_matters_;// 所有可供交易的标的名称
};





#endif