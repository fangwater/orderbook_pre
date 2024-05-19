#ifndef ORDERTICK_HPP
#define ORDERTICK_HPP
#include <cstdint>
#include <folly/ProducerConsumerQueue.h>
using OrderNo_t = int64_t;
using OrgNo_t = int64_t;
using Price_t = int64_t;
using Vol_t = int64_t;
struct OrderTick {
    constexpr static char OrderAdd = 'A';
    constexpr static char OrderCancel = 'D';
    constexpr static char OrderMod = 'M';
    constexpr static char Sync = 'S';
    std::string contract_;
    char type; //Add Del Mod Sync
    char b_s; // B S
    int64_t timestamp; // 时间
    OrderNo_t orderNo;   // Order编号
    OrgNo_t orgNo;     // 报价机构编号
    Price_t price;   // 订单价格
    /**
     * @brief 订单的报价量
     * 1、OrderAdd的情况，表示订单的量
     * 2、OrderDel的情况，vol不会使用，没有意义
     * 3、OrderMod的情况，vol表示修改量，带符号，vol < 0 表示减量，vol > 0 表示增量
     */
    Vol_t vol;// 笔数
};

using OrderTickQueue = folly::ProducerConsumerQueue<OrderTick>;
#endif