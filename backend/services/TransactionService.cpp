#include "TransactionService.h"
#include <cstring> // Для безпечного копіювання рядків (strncpy)

// Допоміжна функція: шукає найбільший ID у файлі і повертає наступний
int TransactionService::generateNewTransactionId() {
    Transaction transactions[100]; // Тимчасовий масив (без vector)
    int count = 0;
    transactionStorage.getAllTransactions(transactions, count, 100);
    
    int maxId = 0;
    for (int i = 0; i < count; ++i) {
        if (transactions[i].id > maxId) {
            maxId = transactions[i].id;
        }
    }
    return maxId + 1; // Наступний вільний ID
}

// Головна бізнес-логіка додавання транзакції
bool TransactionService::addTransaction(double amount, bool isIncome, const char* date, int categoryId, int accountId) {
    // 1. Валідація: сума має бути більшою за нуль
    if (amount <= 0) return false;

    // 2. Перевіряємо, чи існує такий рахунок
    Account account;
    if (!accountStorage.getAccountById(accountId, account)) {
        return false; // Рахунок не знайдено
    }

    // 3. Перевіряємо, чи існує така категорія
    Category category;
    if (!categoryStorage.getCategoryById(categoryId, category)) {
        return false; // Категорія не знайдена
    }

    // 4. Бізнес-логіка: якщо це витрата, перевіряємо чи є гроші на рахунку
    if (!isIncome && account.balance < amount) {
        return false; // Недостатньо коштів!
    }

    // 5. Оновлюємо баланс рахунку
    if (isIncome) {
        account.balance += amount;
    } else {
        account.balance -= amount;
    }

    // Зберігаємо оновлений баланс у файл
    if (!accountStorage.updateAccount(account)) {
        return false; // Помилка запису рахунку
    }

    // 6. Формуємо саму транзакцію
    Transaction tx;
    tx.id = generateNewTransactionId();
    tx.amount = amount;
    tx.isIncome = isIncome;
    tx.categoryId = categoryId;
    tx.accountId = accountId;
    
    // Безпечно копіюємо дату в масив char[]
    std::strncpy(tx.date, date, sizeof(tx.date) - 1);
    tx.date[sizeof(tx.date) - 1] = '\0'; // Гарантуємо кінець рядка

    // 7. Зберігаємо транзакцію у файл
    transactionStorage.addTransaction(tx);

    return true; // Успіх!
}

// Збирає суми по категоріях для діаграми
void TransactionService::getAnalytics(bool isIncome, CategoryTotal* outTotals, int& count, int maxCount) {
    count = 0;
    Transaction transactions[100];
    int txCount = 0;
    
    transactionStorage.getAllTransactions(transactions, txCount, 100);

    for (int i = 0; i < txCount; ++i) {
        if (transactions[i].isIncome == isIncome) {
            
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
}

// Повертає список усіх транзакцій
void TransactionService::getTransactionHistory(Transaction* outList, int& count, int maxCount) {
    transactionStorage.getAllTransactions(outList, count, maxCount);
}