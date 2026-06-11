#pragma once
#include "../models/Transaction.h"
#include "../storage/TransactionStorage.h"
#include "../storage/AccountStorage.h"
#include "../storage/CategoryStorage.h"

// Структура для передачі даних в аналітику (для побудови кругової діаграми)
struct CategoryTotal {
    int categoryId;
    double totalAmount;
};

class TransactionService {
private:
    TransactionStorage transactionStorage;
    AccountStorage accountStorage;
    CategoryStorage categoryStorage;

    // Допоміжна функція для генерації нового ID
    int generateNewTransactionId();

public:
    // Видалення транзакції та відкат балансу
    bool deleteTransaction(int id, int userId);

    // Головна функція додавання транзакції
    bool addTransaction(double amount, bool isIncome, const char* date, int categoryId, int accountId, int userId);
    
    // Логіка переказу коштів між рахунками
    bool transferFunds(int fromAccountId, int toAccountId, double amount, const char* date, int userId);
    
    // Отримання аналітики (суми, згруповані по категоріях)
    void getAnalytics(bool isIncome, int userId, CategoryTotal* outTotals, int& count, int maxCount);
    
    // Отримання історії всіх транзакцій
    void getTransactionHistory(int userId, Transaction* outList, int& count, int maxCount);
};
