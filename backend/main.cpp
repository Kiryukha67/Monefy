#include "crow.h"
#include "storage/AccountStorage.h"
#include "storage/CategoryStorage.h"
#include "services/TransactionService.h"

int main() {
    crow::SimpleApp app;
    AccountStorage accountStorage;
    CategoryStorage categoryStorage;
    TransactionService transactionService;

    // Створюємо базовий рахунок
    Account defaultAcc;
    if (!accountStorage.getAccountById(1, defaultAcc)) {
        defaultAcc = {1, "Main Card", 5000.0, 1};
        accountStorage.addAccount(defaultAcc);
    }

    // Створюємо базову категорію (оскільки TransactionService перевіряє її наявність)
    Category defaultCat;
    if (!categoryStorage.getCategoryById(1, defaultCat)) {
        defaultCat.id = 1;
        // Припускаємо, що у Category є поле name. Якщо інше - адаптуй під свою структуру.
        std::strncpy(defaultCat.name, "General", sizeof(defaultCat.name) - 1);
        categoryStorage.addCategory(defaultCat);
    }

    // Віддача балансу (це в нас вже було)
    CROW_ROUTE(app, "/api/balance")([&accountStorage](){
        Account acc;
        if (accountStorage.getAccountById(1, acc)) {
            crow::json::wvalue json_resp;
            json_resp["id"] = acc.id;
            json_resp["name"] = acc.name;
            json_resp["balance"] = acc.balance;
            
            auto res = crow::response(json_resp);
            res.add_header("Access-Control-Allow-Origin", "*"); 
            return res;
        }
        return crow::response(404);
    });

    // НОВИЙ МАРШРУТ: Приймаємо транзакції (доходи і витрати)
    CROW_ROUTE(app, "/api/transaction").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)
    ([&transactionService](const crow::request& req) {
        // Обробка CORS для POST-запитів
        if (req.method == crow::HTTPMethod::OPTIONS) {
            auto res = crow::response(200);
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            res.add_header("Access-Control-Allow-Headers", "Content-Type");
            return res;
        }

        // Читаємо JSON, який прислав браузер
        auto data = crow::json::load(req.body);
        if (!data) return crow::response(400, "Invalid JSON");

        // Валідація полів JSON
        if (!data.has("amount") || !data.has("isIncome") || !data.has("categoryId") || !data.has("date")) {
            auto res = crow::response(400, "Missing required fields");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }

        double amount = data["amount"].d();
        bool isIncome = data["isIncome"].b();
        int categoryId = data["categoryId"].i();
        std::string date = data["date"].s();
        int accountId = 1; // Поки працюємо з одним захардкодженим рахунком

        // Викликаємо нашу бізнес-логіку!
        if (transactionService.addTransaction(amount, isIncome, date.c_str(), categoryId, accountId)) {
            auto res = crow::response(200, "Success");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        
        auto res = crow::response(400, "Transaction failed (Insufficient funds or missing category/account)");
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

app.port(8085).multithreaded().run();
    return 0;
}