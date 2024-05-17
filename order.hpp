#ifndef ORDER_HPP
#define ORDER_HPP

#include <cstdint>
#include <folly/IntrusiveList.h>
#include <stdexcept>
#include <vector>
#include "order_tick.hpp"

/**
 * @brief 基础Order类型
 */
struct Order {
    Price_t price_;
    Vol_t volume_;
    int64_t timestamp_;
    char b_s_;
    OrgNo_t orgNo_;
    OrderNo_t orderNo_;
    
    Order(Price_t p, Vol_t vol, int64_t tp, char b_s, OrgNo_t org_no, OrderNo_t order_no)
        : price_(p), volume_(vol), timestamp_(tp), b_s_(b_s), orgNo_(org_no), orderNo_(order_no) {}

    Order() = default;
};


//用于侵入式链表封装的Order
struct LinkedOrder : public Order {
    folly::IntrusiveListHook hook;
    using Order::Order;

    // 为 find 提供函数
    bool operator==(const LinkedOrder &other) const {
        return orderNo_ == other.orderNo_;// 比较两个订单的 id 是否相同
    }

    //
    void fill_by_order_add_tick(OrderTick *tick) {
        if (tick->type == OrderTick::OrderAdd) {
            price_ = tick->price;
            volume_ = tick->vol;
            timestamp_ = tick->timestamp;
            b_s_ = tick->b_s;
            orgNo_ = tick->orgNo;
            orderNo_ = tick->orderNo;
        } else {
            throw std::runtime_error("tick is not order add type");
        }
    }
};

using LinkedOrderOrderList = folly::IntrusiveList<LinkedOrder, &LinkedOrder::hook>;

class LinkedOrderPool {
public:
    LinkedOrderPool(size_t size);
    LinkedOrder &get_order();
    void recycle_order(LinkedOrder &order);
private:
    std::vector<LinkedOrder> orders_pool_;
    LinkedOrderOrderList free_list_;// 空闲订单列表
};

#endif