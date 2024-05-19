#include "contract.hpp"
#include "config.hpp"
#include "credit.hpp"
#include "fmt/core.h"
#include "order.hpp"
#include "order_tick.hpp"
#include "orderbook.hpp"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <vector>

std::vector<std::pair<size_t, size_t>> split_vector(size_t total_size, size_t K) {
    std::vector<std::pair<size_t, size_t>> result;
    size_t num_full_chunks = total_size / K;
    size_t remaining = total_size % K;
    for (size_t i = 0; i < num_full_chunks; ++i) {
        result.emplace_back(i * K, (i + 1) * K - 1);
    }
    if (remaining > 0) {
        result.emplace_back(num_full_chunks * K, total_size - 1);
    }
    return result;
}


Contract::Contract(std::shared_ptr<BiCredit> bi_credit)
        : info_queue_(Config::instance().contract_queue_size_), bi_credit_(bi_credit) {
    org_size_ = Config::instance().org_counts_;
    int partition_size = Config::instance().org_partition_size_;
    org_partitions_ = split_vector(org_size_, partition_size);
    //所有org切分为n个group并行处理
    int n = org_partitions_.size();
    auto contract_queue_size = Config::instance().contract_queue_size_;
    for (int i = 0; i < n; i++) {
        auto partition_queue = std::make_shared<OrderTickQueue>(contract_queue_size);
        partition_queues_.push_back(std::move(partition_queue));
    }
    //创建private和public的orderbook
    auto order_pool_size = Config::instance().order_pool_size_;
    auto level_pool_size = Config::instance().level_pool_size_;
    private_orderbooks_.reserve(org_size_);
    for (auto i = 0; i < org_size_; i++) {
        private_orderbooks_.emplace_back(
                std::make_shared<OrderBook>(order_pool_size, level_pool_size)
        );
    }
    //sync
    sync_event_handler = [this]() {
        //callback函数，打印当前合约下，所有机构在买卖方向的最优报价
        std::string s = "";
        for (int i = 0; i < org_size_; i++) {
            auto order_book_sp = private_orderbooks_[i];
            LinkedOrder &buy_best = order_book_sp->best_order<'B'>();
            LinkedOrder &sell_best = order_book_sp->best_order<'S'>();
            s += fmt::format("Org {} best price: B-{}, S-{}", i, buy_best.price_, sell_best.price_);
            s += '\n';
        }
    };
    //同步标志位计为0
    sync_counter = 0;
    //callback执行成功后的放行标志位
    ready = false;
}

bool Contract::filter_by_credit(const OrderTick &tick, OrgNo_t org_index) {
    char side = tick.b_s;
    OrgNo_t orgNo = tick.orgNo;
    auto get_credit = [&](char side, OrgNo_t org1, OrgNo_t org2) -> Credit_t {
        if (side == 'B') {
            return bi_credit_->get<'B'>(org1, org2);
        } else {
            return bi_credit_->get<'S'>(org1, org2);
        }
    };
    Credit_t c1 = get_credit(side, orgNo, org_index);
    Credit_t c2 = get_credit((side == 'B') ? 'S' : 'B', org_index, orgNo);
    return (c1 > 0 && c2 > 0);
}

bool Contract::push(const OrderTick &tick) {
    if (info_queue_.isFull()) {
        throw std::runtime_error("the tick queue ");
    }
    return info_queue_.write(tick);
}

bool Contract::proxy_tick_for_all(const OrderTick &tick) {
    for (auto i = 0; i < partition_queues_.size(); i++) {
        if (!partition_queues_[i]->write(tick)) {
            return false;
        }
    }
    return true;
}

void Contract::run() {
    OrderTick tick;
    while (true) {
        if (info_queue_.read(tick)) {
            if (!proxy_tick_for_all(tick)) {
                LOG(INFO) << "proxy failed";
            }
        }
    }
}

void Contract::ordertick_process(int group_index) {
    auto tick_queue = partition_queues_[group_index];
    while (true) {
        if (!tick_queue->isEmpty()) {
            OrderTick *tick = tick_queue->frontPtr();
            if (tick->type == OrderTick::Sync) {
                //若为同步消息，则lock
                std::unique_lock<std::mutex> lock(mu);
                sync_counter++;
                if (sync_counter == partition_queues_.size()) {
                    //每一个queue都处理到sync标志
                    if (sync_event_handler) {
                        LOG(INFO) << fmt::format("group {} exec sync handler!", group_index);
                        sync_event_handler();
                    }
                    ready.store(true);
                    sync_counter--;
                    while (true) {
                        if (sync_counter == 0 && !ready.load()) {
                            /**
                             * @brief 开lock放行的条件
                             * 最后一个到达sync的线程，成功执行callback，ready为true
                             */
                            lock.unlock();
                            break;
                        }
                        std::this_thread::yield();
                    }
                    LOG(INFO) << fmt::format("group {} passed sync event!", group_index);
                } else {
                    lock.unlock();
                    while (ready.load() == false) {
                        std::this_thread::yield();
                    }
                    sync_counter--;
                    LOG(INFO) << fmt::format("group {} passed sync event!", group_index);
                }
            } else {
                //非同步事件，若满足授信，则直接直接插入orderbook
                auto partition_range = org_partitions_[group_index];
                auto start_org = partition_range.first;
                auto end_org = partition_range.second;
                for (auto i = start_org; i < end_org; i++) {
                    //只有满足授信约束，才加入orderbook
                    if (filter_by_credit(*tick, i)) {
                        private_orderbooks_[i]->process_order(tick);
                    }
                }
                //完整的公有orderbook附带在最后一组处理
                if(group_index == org_partitions_.size() - 1){
                    public_orderbook_->process_order(tick);
                }
            }
            tick_queue->popFront();
        }
    }
}


void Contract::run_groups() {
    std::vector<std::thread> group_workers;
    // 获取partition_queues_的大小，即工作线程的数量
    int num_workers = partition_queues_.size();

    // 创建并启动每个工作线程
    for (int i = 0; i < num_workers; ++i) {
        group_workers.emplace_back(&Contract::ordertick_process, this, i);
    }

    // 开启所有工作线程
    for (auto &t: group_workers) {
        t.detach();
    }
}


ContractManager::ContractManager() : running_flag_(true) {
    symbol_to_index_ = std::make_unique<absl::flat_hash_map<std::string, int>>();
    contract_symbols_ = Config::instance().subject_matters_;
    int n = contract_symbols_.size();
    for (auto i = 0; i < n; i++) {
        auto p = std::pair<std::string, int>(contract_symbols_[i], i);
        symbol_to_index_->insert(std::move(p));
    }
    /**
     * @brief Manager应该持有以下跨合约数据
     * 1、额度数据
     * 2、最优价格数据的写入接口，最好封装在某个object
     * 3、所有参与交易的合约
     * 4、GPU回传订单处理
     */
    bi_credit_ = std::make_shared<BiCredit>();
    //此处设置为全授信
    bi_credit_->set_full_credit();
    for (auto i = 0; i < n; i++) {
        auto contract_ptr = std::make_unique<Contract>(bi_credit_);
        contracts_.push_back(std::move(contract_ptr));
    }
}

void ContractManager::run() {
    std::vector<std::thread> contract_takers;
    std::vector<std::thread> contract_processers;
    for (auto i = 0; i < contract_symbols_.size(); i++) {
        contract_takers.emplace_back(
                [this, i]() {
                    contracts_[i]->run();
                });
        contract_processers.emplace_back(
                [this, i]() {
                    contracts_[i]->run_groups();
                });
    }
    for (auto i = 0; i < contract_symbols_.size(); i++) {
        contract_takers[i].detach();
        contract_processers[i].detach();
        LOG(INFO) << fmt::format("start contract {}",contract_symbols_[i]);
    }
    while (running_flag_) {
        //keep running
    }
}