#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP
#include "order.hpp"
#include "order_tick.hpp"
#include "price_leader.hpp"
#include <absl/container/btree_map.h>
#include <absl/container/flat_hash_map.h>
#include <absl/log/absl_log.h>
#include <absl/log/log.h>
#include <cstdint>
#include <folly/IntrusiveList.h>
#include <folly/ProducerConsumerQueue.h>//SPSC queue
#include <memory>
#include <stdexcept>
#include <fmt/format.h>

class OrderBook {
private:
    template<char Side>
    void remove_level(typename PriceLevel<Side>::iterator level_iter);

    template<char Side>
    void delete_order(int64_t price, OrderNo_t orderNo);

    template<char Side>
    void delete_order(typename PriceLevel<Side>::iterator level_iter, OrderNo_t orderNo);
private:
    //当前订单薄中持有的价格节点
    LevelList levels_;
    //预分配的订单池、价格节点池
    std::shared_ptr<LinkedOrderPool> order_pool_;
    std::shared_ptr<LevelPool> level_pool_;
    //买卖队列
    PriceLevel<'B'> buy_levels;
    PriceLevel<'S'> sell_levels;
public:
    OrderBook(size_t queue_size, size_t order_pool_size, size_t level_pool_size){
        order_pool_ = std::make_shared<LinkedOrderPool>(order_pool_size);
        level_pool_ = std::make_shared<LevelPool>(level_pool_size);
    }
    template<char Side>
    void process_order_mod(OrderTick *tick);

    template<char Side>
    void process_order_add(OrderTick *tick);

    template<char Side>
    void process_order_del(OrderTick *tick);
    void display_order_book(int level);
};

template<char Side>
void OrderBook::remove_level(typename PriceLevel<Side>::iterator level_iter) {
    Level &level = *level_iter->second;
    level_pool_->release_level(level);
    if constexpr (Side == 'B') {
        buy_levels.erase(level_iter);
    } else if constexpr (Side == 'S') {
        sell_levels.erase(level_iter);
    } else {
        static_assert(false);
    }
}


template<char Side>
void OrderBook::delete_order(int64_t price, OrderNo_t orderNo) {
    typename PriceLevel<Side>::iterator level_iter;
    if constexpr (Side == 'B') {
        level_iter = buy_levels.find(price);
        if (level_iter == buy_levels.end()) {
            throw std::runtime_error("failed to find the price in orderbook");
        }
    } else {
        level_iter = sell_levels.find(price);
        if (level_iter == sell_levels.end()) {
            throw std::runtime_error("failed to find the price in orderbook");
        }
    }
    level_iter->second->remove(orderNo);
    if (level_iter->second->order_list.empty()) {
        remove_level<Side>(level_iter);
    }
}
template<char Side>
void OrderBook::delete_order(typename PriceLevel<Side>::iterator level_iter, OrderNo_t orderNo) {
    level_iter->second->remove(orderNo);
    if (level_iter->second->order_list.empty()) {
        remove_level<Side>(level_iter);
    }
}

#endif