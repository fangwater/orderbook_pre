#ifndef CONTRACT_HPP
#define CONTRACT_HPP
/**
 * @brief 交易标的管理,即合约
 * 每个合约维护对应标的买和卖双边
 */
#include "absl/container/flat_hash_map.h"
#include "credit.hpp"
#include "order_tick.hpp"
#include "orderbook.hpp"
#include <cstddef>
#include <memory>
#include <vector>
std::vector<std::pair<size_t, size_t>> split_vector(size_t total_size, size_t K);

using OrderBookSp = std::shared_ptr<OrderBook>;

class Contract {
public:
    Contract();
    bool push(const OrderTick &tick);
    bool send_tick_for_all(const OrderTick& tick);
    bool send_sync_for_all();
    //处理授信变更的情况
    bool handle_credit_update();
    void run();
private:
    //查询order对于目标机构
    bool filter_by_credit(const OrderTick &tick, OrgNo_t org_index);

private:
    //机构数
    int org_size_; 
    /**
     * @brief 接收当前合约的所有交易TXN，以及行情计算的同步指令
     * 不涉及多线程，只作为缓存使用
     */
    OrderTickQueue info_queue_;
    /**
     * @brief 执行线程拆分机制
     * 对于每个机构，需要维护自己的副本orderbook，但如果每个orderbook占据一个线程
     * 则需要的线程数等于(机构数 \times 合约数)，并不合理
     * 因此设计一种拆分的执行方法，即对于每个合约内部，将所有机构拆分为K个block
     * 每个block管理一部分orderbook，同步的单位为block
     */
    std::vector<std::pair<size_t, size_t>> org_partitions_;
    //每个partition的缓冲池
    std::vector<std::shared_ptr<OrderTickQueue>> partition_queues_;
public:
    //双边额度授信关系
    std::shared_ptr<BiCredit> bi_credit_;
    //每个机构保留一个独立的orderbook, 只保存自己授信可见的订单
    std::vector<std::shared_ptr<OrderBook>> private_orderbooks_;
    //保留一个公有订单薄，处理授信变更的情况
    std::shared_ptr<OrderBook> public_orderbook_;
};


/**
 * @brief 集中管理所有合约
 * 接收Message推送的消息，处理后分发到每个合约
 */
class ContractManager {
public:
    ContractManager();
    void run();

private:
public:
    std::unique_ptr<absl::flat_hash_map<std::string, int>> symbol_to_index_;
    std::vector<std::unique_ptr<Contract>> contracts_;
};

#endif
