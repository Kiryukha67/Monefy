#include "TransactionService.h"
#include <cstring>

int TransactionService::generateNewTransactionId() {
    Transaction transactions[100];
    int count = 0;
    transactionStorage.getAllTransactions(transactions, count, 100);

    int maxId = 0;
    for (int i = 0; i < count; ++i) {
        if (transactions[i].id > maxId) {
            maxId = transactions[i].id;
        }
    }
    return maxId + 1;
}

bool TransactionService::deleteTransaction(int id, int userId) {
    Transaction transactions[100];
    int count = 0;
    transactionStorage.getAllTransactions(transactions, count, 100);

    int targetIndex = -1;
    for (int i = 0; i < count; ++i) {
        if (transactions[i].id == id && transactions[i].userId == userId) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex == -1) return false;

    Account acc;
    if (accountStorage.getAccountById(transactions[targetIndex].accountId, acc) && acc.userId == userId) {
        if (transactions[targetIndex].isIncome) {
            acc.balance -= transactions[targetIndex].amount;
        } else {
            acc.balance += transactions[targetIndex].amount;
        }
        accountStorage.updateAccount(acc);
    }

    return transactionStorage.deleteTransaction(id);
}

bool TransactionService::transferFunds(int fromAccountId, int toAccountId, double amount, const char* date, int userId) {
    if (amount <= 0 || fromAccountId == toAccountId) return false;

    Account fromAcc, toAcc;
    if (!accountStorage.getAccountById(fromAccountId, fromAcc) ||
        !accountStorage.getAccountById(toAccountId, toAcc)) {
        return false;
    }
    if (fromAcc.userId != userId || toAcc.userId != userId) return false;
    if (fromAcc.balance < amount) return false;

    fromAcc.balance -= amount;
    toAcc.balance += amount;

    if (!accountStorage.updateAccount(fromAcc) || !accountStorage.updateAccount(toAcc)) {
        return false;
    }

    Transaction txOut = {};
    txOut.id = generateNewTransactionId();
    txOut.userId = userId;
    txOut.amount = amount;
    txOut.isIncome = false;
    txOut.categoryId = 0;
    txOut.accountId = fromAccountId;
    std::strncpy(txOut.date, date, sizeof(txOut.date) - 1);
    txOut.date[sizeof(txOut.date) - 1] = '\0';
    transactionStorage.addTransaction(txOut);

    Transaction txIn = {};
    txIn.id = generateNewTransactionId();
    txIn.userId = userId;
    txIn.amount = amount;
    txIn.isIncome = true;
    txIn.categoryId = 0;
    txIn.accountId = toAccountId;
    std::strncpy(txIn.date, date, sizeof(txIn.date) - 1);
    txIn.date[sizeof(txIn.date) - 1] = '\0';
    transactionStorage.addTransaction(txIn);

    return true;
}

bool TransactionService::addTransaction(double amount, bool isIncome, const char* date, int categoryId, int accountId, int userId) {
    if (amount <= 0) return false;

    Account account;
    if (!accountStorage.getAccountById(accountId, account)) return false;
    if (account.userId != userId) return false;

    Category category;
    if (!categoryStorage.getCategoryById(categoryId, category)) return false;
    if (category.userId != userId) return false;

    if (!isIncome && account.balance < amount) return false;

    if (isIncome) {
        account.balance += amount;
    } else {
        account.balance -= amount;
    }

    if (!accountStorage.updateAccount(account)) return false;

    Transaction tx = {};
    tx.id = generateNewTransactionId();
    tx.userId = userId;
    tx.amount = amount;
    tx.isIncome = isIncome;
    tx.categoryId = categoryId;
    tx.accountId = accountId;
    std::strncpy(tx.date, date, sizeof(tx.date) - 1);
    tx.date[sizeof(tx.date) - 1] = '\0';

    transactionStorage.addTransaction(tx);
    return true;
}

void TransactionService::getAnalytics(bool isIncome, int userId, CategoryTotal* outTotals, int& count, int maxCount) {
    count = 0;
    Transaction transactions[100];
    int txCount = 0;

    transactionStorage.getAllTransactions(transactions, txCount, 100);

    for (int i = 0; i < txCount; ++i) {
        if (transactions[i].userId != userId || transactions[i].isIncome != isIncome) continue;

        bool found = false;
        for (int j = 0; j < count; ++j) {
            if (outTotals[j].categoryId == transactions[i].categoryId) {
                outTotals[j].totalAmount += transactions[i].amount;
                found = true;
                break;
            }
        }

        if (!found && count < maxCount) {
            outTotals[count].categoryId = transactions[i].categoryId;
            outTotals[count].totalAmount = transactions[i].amount;
            count++;
        }
    }
}

void TransactionService::getTransactionHistory(int userId, Transaction* outList, int& count, int maxCount) {
    Transaction transactions[100];
    int txCount = 0;
    transactionStorage.getAllTransactions(transactions, txCount, 100);

    count = 0;
    for (int i = 0; i < txCount && count < maxCount; ++i) {
        if (transactions[i].userId == userId) {
            outList[count++] = transactions[i];
        }
    }
}
