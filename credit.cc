#include "credit.hpp"
#include "config.hpp"
#include <limits>
#include <memory>
BiCredit::BiCredit() {
    orgs_ = Config::instance().org_counts_;
    buy_credit_data_ = std::make_unique<std::vector<Credit_t>>(orgs_ * orgs_);
    sell_credit_data_ = std::make_unique<std::vector<Credit_t>>(orgs_ * orgs_);
}

void BiCredit::set_full_credit() {
    std::fill(buy_credit_data_->begin(), buy_credit_data_->end(), std::numeric_limits<Credit_t>::max());
    std::fill(sell_credit_data_->begin(), sell_credit_data_->end(), std::numeric_limits<Credit_t>::max());
}

void BiCredit::set_zero_credit() {
    std::fill(buy_credit_data_->begin(), buy_credit_data_->end(), 0);
    std::fill(sell_credit_data_->begin(), sell_credit_data_->end(), 0);
}

