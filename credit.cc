#include "credit.hpp"
#include "config.hpp"
#include <memory>
BiCredit::BiCredit() {
    orgs_ = Config::instance().org_counts_;
    buy_credit_data_ = std::make_unique<std::vector<Credit_t>>(orgs_ * orgs_);
    sell_credit_data_ = std::make_unique<std::vector<Credit_t>>(orgs_*orgs_);
}