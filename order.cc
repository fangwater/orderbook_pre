#include "order.hpp"
#include <stdexcept>
// 构造函数定义
LinkedOrderPool::LinkedOrderPool(size_t size) {
    orders_pool_.resize(size);
    for (auto &order: orders_pool_) {
        free_list_.push_back(order);
    }
}

// 获取订单函数定义
LinkedOrder &LinkedOrderPool::get_order() {
    if (!free_list_.empty()) {
        LinkedOrder &order = free_list_.front();
        free_list_.pop_front();
        return order;
    } else {
        throw std::runtime_error("memory exhausted");
    }
}

// 回收订单函数定义
void LinkedOrderPool::recycle_order(LinkedOrder &order) {
    free_list_.push_back(order);
}