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
    bool deleteTransaction(int id);

    // Головна функція додавання транзакції
    bool addTransaction(double amount, bool isIncome, const char* date, int categoryId, int accountId);
    
    // Логіка переказу коштів між рахунками
    bool transferFunds(int fromAccountId, int toAccountId, double amount, const char* date);
    
    // Отримання аналітики (суми, згруповані по категоріях)
    void getAnalytics(bool isIncome, CategoryTotal* outTotals, int& count, int maxCount);
    
    // Отримання історії всіх транзакцій
    void getTransactionHistory(Transaction* outList, int& count, int maxCount);
};