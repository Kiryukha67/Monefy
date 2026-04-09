#include "crow.h"
#include "crow/middlewares/cors.h"
#include <exception>
#include "storage/AccountStorage.h"
#include "storage/CategoryStorage.h"
#include "services/TransactionService.h"

int main() {
    // Використовуємо вбудований CORS Middleware від Crow
    crow::App<crow::CORSHandler> app;
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)
        .origin("*");

    AccountStorage accountStorage;
    CategoryStorage categoryStorage;
    TransactionService transactionService;

    // Створюємо базовий рахунок
    Account defaultAcc = {}; // Очищаємо пам'ять (Zero-initialization)
    if (!accountStorage.getAccountById(1, defaultAcc)) {
        defaultAcc = {1, "Main Card", 5000.0, 1};
        accountStorage.addAccount(defaultAcc);
    }

    // Створюємо базові категорії (щоб фронтенд міг їх успішно використовувати)
    const char* defaultCategories[] = {"Їжа", "Транспорт", "Супермаркети", "Розваги", "Зарплата"};
    for (int i = 1; i <= 5; ++i) {
        Category cat = {}; // Очищаємо пам'ять, щоб уникнути сміття у файлі
        if (!categoryStorage.getCategoryById(i, cat)) {
            cat.id = i;
            std::strncpy(cat.name, defaultCategories[i-1], sizeof(cat.name) - 1);
            cat.name[sizeof(cat.name) - 1] = '\0';
            categoryStorage.addCategory(cat);
        }
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
        auto res = crow::response(404, "Account not found");
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // НОВИЙ МАРШРУТ: Приймаємо транзакції (доходи і витрати)
    CROW_ROUTE(app, "/api/transaction").methods(crow::HTTPMethod::POST)
    ([&transactionService](const crow::request& req) {
        try {
        // Читаємо JSON, який прислав браузер
        auto data = crow::json::load(req.body);
        if (!data) {
            auto res = crow::response(400, "Invalid JSON");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }

        // Валідація полів JSON
        if (!data.has("amount") || !data.has("isIncome") || !data.has("categoryId") || !data.has("date")) {
            auto res = crow::response(400, "Missing required fields");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }

        // АБСОЛЮТНО БЕЗПЕЧНЕ ЧИТАННЯ БЕЗ КРАШІВ
        double amount = 0.0;
        if (data["amount"].t() == crow::json::type::Number) {
            amount = data["amount"].d();
        } else {
            auto res = crow::response(400, "Invalid type for amount");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }

        if (data["isIncome"].t() != crow::json::type::True && data["isIncome"].t() != crow::json::type::False) {
            auto res = crow::response(400, "Invalid type for isIncome");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        bool isIncome = data["isIncome"].b();

        if (data["categoryId"].t() != crow::json::type::Number) {
            auto res = crow::response(400, "Invalid type for categoryId");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        int categoryId = static_cast<int>(data["categoryId"].d()); // Безпечне читання числа

        if (data["date"].t() != crow::json::type::String) {
            auto res = crow::response(400, "Invalid type for date");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        std::string date = data["date"].s();
        
        int accountId = 1; 

        // Викликаємо нашу бізнес-логіку!
        if (transactionService.addTransaction(amount, isIncome, date.c_str(), categoryId, accountId)) {
            auto res = crow::response(200, "Success");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        
        auto res = crow::response(400, "Transaction failed (Insufficient funds)");
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
        } catch (const std::exception& e) {
            auto res = crow::response(500, std::string("Server Exception: ") + e.what());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });

    // МАРШРУТ: Історія транзакцій
    CROW_ROUTE(app, "/api/transactions")([&transactionService](){
        Transaction txs[100];
        int count = 0;
        transactionService.getTransactionHistory(txs, count, 100);
        
        std::vector<crow::json::wvalue> json_txs;
        for (int i = count - 1; i >= 0; --i) { // Нові транзакції зверху
            crow::json::wvalue tx;
            tx["id"] = txs[i].id;
            tx["amount"] = txs[i].amount;
            tx["type"] = txs[i].isIncome ? "income" : "expense";
            
            // Базовий мапінг імен
            const char* names[] = {"Інше", "Їжа", "Транспорт", "Супермаркети", "Розваги", "Зарплата"};
            const char* icons[] = {"💰", "🍔", "🚗", "🛒", "🎮", "💰"};
            
            int catIdx = (txs[i].categoryId >= 1 && txs[i].categoryId <= 5) ? txs[i].categoryId : 0;
            tx["categoryName"] = names[catIdx];
            tx["icon"] = icons[catIdx];
            txs[i].date[19] = '\0'; // Гарантуємо відсутність крашу пам'яті при читанні дати
            tx["date"] = std::string(txs[i].date);
            json_txs.push_back(tx);
        }
        
        auto res = crow::response(crow::json::wvalue(json_txs));
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // МАРШРУТ: Аналітика (Кругова діаграма)
    CROW_ROUTE(app, "/api/analytics")([&transactionService](){
        CategoryTotal totals[10]; // Максимум 10 категорій
        int count = 0;
        
        // Отримуємо статистику витрат (isIncome = false)
        transactionService.getAnalytics(false, totals, count, 10);
        
        std::vector<std::string> labels;
        std::vector<double> dataArray;
        
        const char* names[] = {"Інше", "Їжа", "Транспорт", "Супермаркети", "Розваги", "Зарплата"};
        
        for (int i = 0; i < count; ++i) {
            int catIdx = (totals[i].categoryId >= 1 && totals[i].categoryId <= 5) ? totals[i].categoryId : 0;
            labels.push_back(names[catIdx]);
            dataArray.push_back(totals[i].totalAmount);
        }
        
        crow::json::wvalue res_json;
        res_json["labels"] = labels;
        res_json["data"] = dataArray;
        
        auto res = crow::response(res_json);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

app.port(8085).multithreaded().run();
    return 0;
}