#include "price_leader.hpp"
#include "order.hpp"
void PriceLeader::remove(LinkedOrder &order) {
    order_list_.remove(order);
    if (order_pool_) {
        order_pool_->recycle_order(order);
    } else {
        throw std::runtime_error("order pool is not bind");
    }
}
std::optional<LinkedOrder *> PriceLeader::find_order(OrderNo_t orderNo) {
    for (auto iter = order_list_.begin(); iter != order_list_.end(); iter++) {
        if (iter->orderNo_ == orderNo) {
            return &*iter;
        } else {
            fmt::print("OrderNo is {}\n", iter->orderNo_);
        }
    }
    return std::nullopt;
}

bool PriceLeader::remove(OrderNo_t orderNo) {
    for (auto iter = order_list_.begin(); iter != order_list_.end(); iter++) {
        if (iter->orderNo_ == orderNo) {
            //find order
            remove(*iter);
            return true;
        }
    }
    return false;
}

void PriceLeader::push_back(LinkedOrder &order) {
    order_list_.push_back(order);
}

LevelPool::LevelPool(size_t size) : level_pool_(size) {
    for (auto &level: level_pool_) {
        free_list_.push_back(level);
    }
}

Level &LevelPool::get_level(int64_t price, std::shared_ptr<LinkedOrderPool> order_pool) {
    if (!free_list_.empty()) {
        Level &level = free_list_.front();
        free_list_.pop_front();
        level.price_ = price;
        level.order_pool_ = order_pool;
        return level;
    } else {
        throw std::runtime_error("memory exhausted");
    }
}

void LevelPool::release_level(Level &level) {
    level.order_list_.clear();
    free_list_.push_back(level);
}