#ifndef CREDIT_HPP
#define CREDIT_HPP

/**
 * @brief 存储对手方和本方在买卖两个方向的额度 类似于区分拆入和拆出
 * 
 * 需要提供额度的更新接口和额度的访问接口，
 */
#include <cstdint>
#include <memory>
#include <vector>
using Credit_t = int64_t;

class BiCredit {
public:
    //额度考虑双边授信
    template<char Side>
    Credit_t get(int from, int to);

    template<char Side>
    void set(int from, int to, Credit_t credit);

    // 设置为全授信
    void set_full_credit();

    // 设置为零授信
    void set_zero_credit();

    //构造函数
    BiCredit();
public:
    int orgs_;                                               // 机构数
    std::unique_ptr<std::vector<Credit_t>> buy_credit_data_; //主动买入额度矩阵
    std::unique_ptr<std::vector<Credit_t>> sell_credit_data_;//主动卖出额度矩阵
};


template<char Side>
Credit_t BiCredit::get(int from, int to) {
    int64_t index = from * orgs_ + to;
    if constexpr (Side == 'B') {
        //查询 from 向 to 的 买入额度
        return (*buy_credit_data_)[index];
    } else if constexpr (Side == 'S') {
        //查询 from 向 to 的 卖出额度
        return (*sell_credit_data_)[index];
    } else {
        static_assert(false);
    }
}

template<char Side>
void BiCredit::set(int from, int to, Credit_t credit) {
    int64_t index = from * orgs_ + to;
    if constexpr (Side == 'B') {
        //设置 from 向 to 的 买入额度
        (*buy_credit_data_)[index] = credit;
    } else if constexpr (Side == 'S') {
        //设置 from 向 to 的 卖出额度
        (*sell_credit_data_)[index] = credit;
    } else {
        static_assert(false);
    }
}

#endif