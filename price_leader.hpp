#ifndef PRICE_LEADER
#define PRICE_LEADER
#include <fmt/format.h>
#include <optional>
#include <absl/container/btree_map.h>
#include "order.hpp"
#include "order_tick.hpp"
/**
 * @brief 订单薄上的价格节点
 * 多个价格节点构成订单薄
 */
class PriceLeader {
public:
    PriceLeader();
    void remove(LinkedOrder &order);
    bool remove(OrderNo_t orderNo);
    void push_back(LinkedOrder &order);
    std::optional<LinkedOrder *> find_order(OrderNo_t orderNo);

public:
    //节点价格
    Price_t price_;
    //当前节点上的所有订单
    LinkedOrderOrderList order_list_;
    /**
     * @brief 订单池
     * 所有order都从这个orderpool获取
     * 为保证不出现线程争用，每个orderbook都有独立的order_pool
     * 同一个orderbook的所有price_leader共享这个订单薄
     */
    std::shared_ptr<LinkedOrderPool> order_pool_;
};


/**
 * @brief 价格级，简写level
 * 
 */
class Level : public PriceLeader {
public:
    folly::IntrusiveListHook hook;
};

using LevelList = folly::IntrusiveList<Level, &Level::hook>;


class LevelPool {
public:
    LevelPool(size_t size);
    //分配新的价格层级
    Level &get_level(int64_t price, std::shared_ptr<LinkedOrderPool> order_pool);
    //释放旧的价格层级
    void release_level(Level &level);
public:
    friend Level;
    std::vector<Level> level_pool_; //预分配节点
    LevelList free_list_;// 空闲价格leader节点
};

template<char Side>
struct Compare {
    bool operator()(const int64_t &a, const int64_t &b) const {
        if constexpr (Side == 'S') {
            //对于sell方向，top价格是sell的最低价，因此应该是升序排列
            return a < b;// 升序排序
        } else {
            //对于buy方向，top价格是buy的最高价，因此是降序排列
            return a > b;// 降序排序
        }
    }
};

template<char Side>
using PriceLevel = absl::btree_map<Price_t, LevelList::iterator, Compare<Side>>;


#endif