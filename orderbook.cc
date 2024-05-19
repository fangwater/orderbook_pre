#include "orderbook.hpp"
#include <fmt/format.h>
#include "order.hpp"
#include "order_tick.hpp"
#include "price_leader.hpp"
#include <cassert>
#include <stack>
#include <stdexcept>
#include <string>
void OrderBook::display_order_book(int level) {
    auto biter = buy_levels.begin();
    int count = 0;
    std::string ob_str;
    ob_str += "-----------------\n|buy\n-----------------\n";
    std::stack<std::string> buy_level_lines;
    while (biter != buy_levels.end() && count < level) {
        std::string line = fmt::format("{:<8}", biter->first);
        for (auto iter = biter->second->order_list_.begin(); iter != biter->second->order_list_.end(); iter++) {
            //打印订单内容(机构-量)
            line += fmt::format("({},{})-->", iter->orgNo_, iter->volume_);
        }
        ++biter;//递增迭代器
        ++count;
        buy_level_lines.push(std::move(line));
    }
    while (!buy_levels.empty()) {
        ob_str += (buy_level_lines.top() + "\n");
        buy_level_lines.pop();
    }
    // 添加分隔线
    ob_str += "-----------------\n";

    ob_str += "-----------------\n|sell\n-----------------\n";
    // sell_level的处理
    auto siter = sell_levels.begin();
    count = 0;
    while (siter != sell_levels.end() && count < level) {
        std::string line = fmt::format("{:<8}", siter->first);
        for (auto iter = siter->second->order_list_.begin(); iter != siter->second->order_list_.end(); iter++) {
            //打印订单内容(机构-量)
            line += fmt::format("({},{})-->", iter->orgNo_, iter->volume_);
        }
        ob_str += (line + "\n");
        ++siter;// 递增迭代器
        ++count;
    }
    ob_str += "-----------------\n\n";
    // 最终输出到控制台
    fmt::print("{}", ob_str);
}

template<char Side>
void OrderBook::process_order_del(OrderTick *tick) {
    if constexpr (Side == 'B') {
        delete_order<'B'>(tick->price, tick->orderNo);
    } else if constexpr (Side == 'S'){
        delete_order<'S'>(tick->price, tick->orderNo);
    } else {
        static_assert(false);
    }
}

template<char Side>
void OrderBook::process_order_mod(OrderTick *tick) {
    //修改某个订单，差不多的做法
    typename PriceLevel<Side>::iterator level_iter;
    if constexpr (Side == 'B') {
        level_iter = buy_levels.find(tick->price);
        if (level_iter == buy_levels.end()) {
            throw std::runtime_error("failed to find the price in orderbook");
        }
    } else if constexpr (Side == 'S'){
        level_iter = sell_levels.find(tick->price);
        if (level_iter == sell_levels.end()) {
            throw std::runtime_error("failed to find the price in orderbook");
        }
    } else {
        LOG(FATAL) << "tick bs wrong";
    }
    OrderNo_t orderNo = tick->orderNo;
    LinkedOrder *target_order = [&level_iter, &orderNo]() {
        auto res = level_iter->second->find_order(orderNo);
        if (res.has_value()) {
            return res.value();
        } else {
            throw std::runtime_error(fmt::format("can not find target_order: {}", orderNo));
        }
    }();

    Vol_t processed_vol = target_order->volume_ + tick->vol;
    if (processed_vol == 0) {
        //delete
        delete_order<Side>(level_iter, orderNo);
    } else if (processed_vol) {
        //update vol
        target_order->volume_ = processed_vol;
    } else {
        //processed_vol < 0, error
        throw std::runtime_error("trying to decrease order < 0");
    }
}


template <char Side>
void OrderBook::process_order_add(OrderTick *tick) {
    LinkedOrder &order = order_pool_->get_order();
    order.fill_by_order_add_tick(tick);
    if (order.b_s_ == 'B') {
        auto level_iter = buy_levels.find(order.price_);
        if (level_iter == buy_levels.end()) {
            //当前价格不存在orderbook中，需要新增一个价格节点
            Level &level = level_pool_->get_level(order.price_, order_pool_);
            //此笔order作为节点的第一个订单
            level.push_back(order);
            //将这个价格对应的level，放置到levels_中
            levels_.push_back(level);
            //btree中存储迭代器，而非level节点本身，因为btree_map不支持侵入式
            auto last_elem_iter = --levels_.end();
            std::pair<Price_t, LevelList::iterator> p(level.price_, last_elem_iter);
            DLOG(INFO) << fmt::format("OrderNo: {}",order.orderNo_);
            buy_levels.emplace(std::move(p));
        } else {
            //当前价格已经存在orderbook中，直接追加订单即可
            auto &level = level_iter->second;
            level->push_back(order);
        }
    } else {
        auto level_iter = sell_levels.find(order.price_);
        if (level_iter == sell_levels.end()) {
            //当前价格不存在orderbook中，需要新增一个价格节点
            Level &level = level_pool_->get_level(order.price_, order_pool_);
            //此笔order作为节点的第一个订单
            level.push_back(order);
            //将这个价格对应的level，放置到levels_中
            levels_.push_back(level);
            //btree中存储迭代器，而非level节点本身，因为btree_map不支持侵入式
            auto last_elem_iter = --levels_.end();
            std::pair<Price_t, LevelList::iterator> p(level.price_, last_elem_iter);
            DLOG(INFO) << fmt::format("OrderNo: {}", order.orderNo_);
            sell_levels.emplace(std::move(p));
        } else {
            //当前价格已经存在orderbook中，直接追加订单即可
            auto &level = level_iter->second;
            level->push_back(order);
        }
    }
}

void OrderBook::process_order(OrderTick *tick) {
    if (tick->type == OrderTick::Sync) {
        throw std::runtime_error("error type SYNC, should not process here!");
    } else if (tick->type == OrderTick::OrderAdd) {
        if (tick->b_s == 'B') {
            process_order_add<'B'>(tick);
        } else if (tick->b_s == 'S') {
            process_order_add<'S'>(tick);
        } else {
            throw std::runtime_error(fmt::format("error side {}, should be B/S", tick->b_s));
        }
    } else if (tick->type == OrderTick::OrderCancel) {
        if (tick->b_s == 'B') {
            process_order_del<'B'>(tick);
        } else if (tick->b_s == 'S') {
            process_order_del<'S'>(tick);
        } else {
            throw std::runtime_error(fmt::format("error side {}, should be B/S", tick->b_s));
        }
    } else if (tick->type == OrderTick::OrderMod){
        if (tick->b_s == 'B') {
            process_order_mod<'B'>(tick);
        } else if (tick->b_s == 'S') {
            process_order_mod<'S'>(tick);
        } else {
            throw std::runtime_error(fmt::format("error side {}, should be B/S", tick->b_s));
        }
    } else {
        throw std::runtime_error(fmt::format("error type {}, should be A/D/M", tick->type));
    }
}