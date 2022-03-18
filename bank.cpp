#include "bank.h"
#include <stdexcept>
namespace bank {

user::user(std::string const &name_) : username(name_) {
    user_balance_xts = 100;
    std::string message = "Initial deposit for " + name_;
    transaction temp(nullptr, 100, message);
    transtactions.push_back(temp);
}

user_transactions_iterator user::monitor() const {
    return user_transactions_iterator(this);
}
user_transactions_iterator::user_transactions_iterator(const user *user_ptr)
    : user_ptr(user_ptr), index(user_ptr->transtactions.size()) {
}

transaction user_transactions_iterator::wait_next_transaction() {
    std::unique_lock l(user_ptr->m_m);
    while (index >= user_ptr->transtactions.size()) {
        user_ptr->cond.wait(l);
    }
    return user_ptr->transtactions[index++];
}
void user::transfer(user &counterparty,
                    int amount_xts,
                    const std::string &comment) {
    std::scoped_lock l(m_m, counterparty.m_m);
    if (amount_xts > user_balance_xts) {
        throw not_enough_funds_error();
    } else {
        user *temp = &counterparty;
        transaction output_transaction(temp, -amount_xts, comment);
        transaction input_transaction(this, amount_xts, comment);
        transtactions.push_back(output_transaction);
        counterparty.transtactions.push_back(input_transaction);
        counterparty.balance_change(true, amount_xts);
        balance_change(false, amount_xts);
        cond.notify_all();
        counterparty.cond.notify_all();
    }
}
void user::balance_change(bool mode, int amount_xts) {
    if (mode) {
        user_balance_xts += amount_xts;
    } else {
        user_balance_xts -= amount_xts;
    }
}
user_transactions_iterator user::snapshot_transactions(
    std::function<void(const std::vector<transaction> &transactions1,
                       int balance)> const &input) const {
    std::unique_lock l(m_m);
    input(transtactions, user_balance_xts);
    return user_transactions_iterator(this);
}
}  // namespace bank