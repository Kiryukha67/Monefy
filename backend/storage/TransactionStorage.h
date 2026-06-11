#pragma once
#include "../models/Transaction.h"

class TransactionStorage {
private:
    const char* filename = "transactions_v2.dat";
public:
    void addTransaction(const Transaction& transaction);
    bool getTransactionById(int id, Transaction& outTransaction);
    void getAllTransactions(Transaction* outArray, int& count, int maxCount);
    bool updateTransaction(const Transaction& updatedTransaction);
    bool deleteTransaction(int id);
};
