#include <iostream>
#include "models/Account.h"
#include "models/Category.h"
#include "storage/AccountStorage.h"
#include "storage/CategoryStorage.h"
#include "services/TransactionService.h"
#include <windows.h>

using namespace std;

void printMenu() {
    cout << "\n===============================" << endl;
    cout << "       💰 MONEFY APP 💰      " << endl;
    cout << "===============================" << endl;
    cout << "1. Додати дохід" << endl;
    cout << "2. Додати витрату" << endl;
    cout << "3. Показати аналітику витрат" << endl;
    cout << "4. Показати історію транзакцій" << endl;
    cout << "0. Вийти" << endl;
    cout << "===============================" << endl;
    cout << "Оберіть дію: ";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // Ініціалізуємо наші сховища та сервіс
    AccountStorage accountStorage;
    CategoryStorage categoryStorage;
    TransactionService txService;

    // Створюємо базовий рахунок та категорії, якщо їх ще немає
    Account defaultAcc;
    if (!accountStorage.getAccountById(1, defaultAcc)) {
        defaultAcc = {1, "Main Card", 5000.0, 1}; // Початковий баланс 5000
        accountStorage.addAccount(defaultAcc);
    }

    Category incomeCat, expenseCat;
    if (!categoryStorage.getCategoryById(1, incomeCat)) {
        incomeCat = {1, "Salary", true};
        categoryStorage.addCategory(incomeCat);
    }
    if (!categoryStorage.getCategoryById(2, expenseCat)) {
        expenseCat = {2, "Food", false};
        categoryStorage.addCategory(expenseCat);
    }

    int choice;
    do {
        printMenu();
        cin >> choice;

        if (choice == 1 || choice == 2) {
            double amount;
            char date[20];
            bool isIncome = (choice == 1);
            int categoryId = isIncome ? 1 : 2; // Для простоти беремо ID 1 або 2
            int accountId = 1; // Завжди використовуємо рахунок ID 1

            cout << "Введіть суму: ";
            cin >> amount;
            cout << "Введіть дату (наприклад, 22-03-2026): ";
            cin >> date;

            // Викликаємо наш сервіс!
            if (txService.addTransaction(amount, isIncome, date, categoryId, accountId)) {
                cout << "[УСПІХ] Транзакцію збережено!" << endl;
            } else {
                cout << "[ПОМИЛКА] Не вдалося зберегти (можливо, недостатньо коштів на рахунку)." << endl;
            }
        } 
        else if (choice == 3) {
            CategoryTotal totals[10];
            int count = 0;
            cout << "\n--- Аналітика витрат ---" << endl;
            txService.getAnalytics(false, totals, count, 10);
            
            if (count == 0) cout << "Витрат ще немає." << endl;
            for (int i = 0; i < count; i++) {
                cout << "Категорія ID " << totals[i].categoryId << " | Всього витрачено: " << totals[i].totalAmount << " UAH" << endl;
            }
        }
        else if (choice == 4) {
            Transaction history[100];
            int count = 0;
            cout << "\n--- Історія транзакцій ---" << endl;
            txService.getTransactionHistory(history, count, 100);
            
            if (count == 0) cout << "Історія порожня." << endl;
            for (int i = 0; i < count; i++) {
                cout << "ID: " << history[i].id << " | Дата: " << history[i].date << " | Сума: " 
                     << (history[i].isIncome ? "+" : "-") << history[i].amount << " UAH" << endl;
            }
        }
    } while (choice != 0);

    cout << "Програма завершена. Гарного дня!" << endl;
    return 0;
}