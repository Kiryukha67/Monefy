#pragma once
#include "../models/Account.h"

class AccountStorage {
private:
    const char* filename = "accounts.dat";
public:
    void addAccount(const Account& account);
    bool getAccountById(int id, Account& outAccount);
    void getAllAccounts(Account* outArray, int& count, int maxCount);
    bool updateAccount(const Account& updatedAccount);
    bool deleteAccount(int id);
};