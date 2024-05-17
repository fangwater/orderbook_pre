#include "contract.hpp"
#include "config.hpp"
#include "credit.hpp"
#include "order_tick.hpp"
#include <stdexcept>

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


Contract::Contract() : info_queue_(Config::instance().contrace_queue_size_) {
    org_size_ = Config::instance().org_counts_;
    int partition_size = Config::instance().org_partition_size_;
    org_partitions_ = split_vector(org_size_,partition_size);
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

bool Contract::send_sync_for_all() {
    OrderTick tick;
    tick.type = OrderTick::Sync;
    for (auto i = 0; i < partition_queues_.size(); i++) {
        if (!private_orderbooks_[i]->order_tick_queue_.write(tick)) {
            return false;
        }
    }
    return public_orderbook_->order_tick_queue_.write(tick);
}

bool Contract::send_tick_for_all(const OrderTick &tick) {
    for (auto i = 0; i < private_orderbooks_.size(); i++) {
        if (filter_by_credit(tick, i)) {
            if (!private_orderbooks_[i]->order_tick_queue_.write(tick)) {
                return false;
            }
        }
    }
    return public_orderbook_->order_tick_queue_.write(tick);
}

void Contract::run() {
    OrderTick tick;
    while (true) {
        if (info_queue_.read(tick)) {
            if (tick.type == OrderTick::Sync) {
                //给所有orderbook发送
                send_sync_for_all();
            } else {
                //只给具有授信的对手发送
                for(auto i = 0; i < private_orderbooks_.size())
            }
        }
    }
}


ContractManager::ContractManager() {
    symbol_to_index_ = std::make_unique<absl::flat_hash_map<std::string, int>>();
    auto &subject_matters = Config::instance().subject_matters_;
    int n = subject_matters.size();
    for (auto i = 0; i < n; i++) {
        auto p = std::pair<std::string, int>(subject_matters[i], i);
        symbol_to_index_->insert(std::move(p));
    }
    //为每个contract创建
}