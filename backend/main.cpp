#include "crow.h"
#include "crow/middlewares/cors.h"
#include <exception>
#include "storage/AccountStorage.h"
#include "storage/CategoryStorage.h"
#include "services/TransactionService.h"
#include <ctime> // Для std::time (генерація унікальних ID)

// Виправлення конфлікту макросів Windows API
#ifdef _WIN32
#undef DELETE
#endif

int main() {
    // Використовуємо вбудований CORS Middleware від Crow
    crow::App<crow::CORSHandler> app;
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization", "Accept")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET, crow::HTTPMethod::PUT, crow::HTTPMethod::DELETE, crow::HTTPMethod::OPTIONS)
        .origin("*");

    AccountStorage accountStorage;
    CategoryStorage categoryStorage;
    TransactionService transactionService;

    // Створюємо базовий рахунок
    Account defaultAcc = {}; // Очищаємо пам'ять (Zero-initialization)
    if (!accountStorage.getAccountById(1, defaultAcc)) {
        defaultAcc.id = 1;
        std::strncpy(defaultAcc.name, "Main Card", sizeof(defaultAcc.name) - 1);
        defaultAcc.balance = 5000.0;
        std::strncpy(defaultAcc.icon, "💳", sizeof(defaultAcc.icon) - 1);
        accountStorage.addAccount(defaultAcc);
    }

    // Створюємо базові категорії (щоб фронтенд міг їх успішно використовувати)
    const char* defaultCategories[] = {"Їжа", "Транспорт", "Супермаркети", "Розваги", "Зарплата"};
    const char* defaultIcons[] = {"🍔", "🚗", "🛒", "🎮", "💰"};
    const char* defaultColors[] = {"#ef4444", "#3b82f6", "#10b981", "#8b5cf6", "#f59e0b"};
    for (int i = 1; i <= 5; ++i) {
        Category cat = {}; // Очищаємо пам'ять, щоб уникнути сміття у файлі
        if (!categoryStorage.getCategoryById(i, cat)) {
            cat.id = i;
            std::strncpy(cat.name, defaultCategories[i-1], sizeof(cat.name) - 1);
            cat.name[sizeof(cat.name) - 1] = '\0';
            std::strncpy(cat.icon, defaultIcons[i-1], sizeof(cat.icon) - 1);
            cat.icon[sizeof(cat.icon) - 1] = '\0'; // Захист від ієрогліфів
            std::strncpy(cat.color, defaultColors[i-1], sizeof(cat.color) - 1);
            cat.color[sizeof(cat.color) - 1] = '\0'; // Захист від ієрогліфів
            cat.isIncome = (i == 5); // 5-та категорія (Зарплата) - це дохід
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
        if (!data.has("amount") || !data.has("isIncome") || !data.has("categoryId") || !data.has("accountId") || !data.has("date")) {
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
        
        if (data["accountId"].t() != crow::json::type::Number) {
            auto res = crow::response(400, "Invalid type for accountId");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        int accountId = static_cast<int>(data["accountId"].d());

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

    // ==========================================================
    // --- НОВИЙ МАРШРУТ: Переказ між рахунками (POST /api/transfer) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/transfer").methods(crow::HTTPMethod::POST)
    ([&accountStorage, &transactionService](const crow::request& req) {
        try {
            auto data = crow::json::load(req.body);
            if (!data) return crow::response(400, "Invalid JSON");

            if (!data.has("fromAccountId") || !data.has("toAccountId") || !data.has("amount") || !data.has("date")) {
                auto res = crow::response(400, "Missing required fields");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            int fromAccountId = static_cast<int>(data["fromAccountId"].d());
            int toAccountId = static_cast<int>(data["toAccountId"].d());
            double amount = data["amount"].d();
            std::string date = data["date"].s();

            if (fromAccountId == toAccountId || amount <= 0) {
                auto res = crow::response(400, "Invalid transfer parameters");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            Account fromAcc, toAcc;
            if (!accountStorage.getAccountById(fromAccountId, fromAcc) || 
                !accountStorage.getAccountById(toAccountId, toAcc)) {
                auto res = crow::response(404, "One or both accounts not found");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            if (fromAcc.balance < amount) {
                auto res = crow::response(400, "Insufficient funds");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            // 1. Оновлюємо баланси рахунків
            fromAcc.balance -= amount;
            toAcc.balance += amount;
            accountStorage.updateAccount(fromAcc);
            accountStorage.updateAccount(toAcc);

            // 2. Створюємо ДВІ нові транзакції (категорія 0 = системна/Інше)
            // Примітка: поле note/comment можна додати до структури Transaction у майбутньому
            transactionService.addTransaction(amount, false, date.c_str(), 0, fromAccountId); // Витрата з першого
            transactionService.addTransaction(amount, true, date.c_str(), 0, toAccountId);    // Дохід на другий

            auto res = crow::response(200, "Transfer successful");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            auto res = crow::response(500, std::string("Server Error: ") + e.what());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });

    // МАРШРУТ: Історія транзакцій
    CROW_ROUTE(app, "/api/transactions")([&transactionService, &categoryStorage, &accountStorage](){
        Transaction txs[100];
        int count = 0;
        transactionService.getTransactionHistory(txs, count, 100);
        
        std::vector<crow::json::wvalue> json_txs;
        for (int i = count - 1; i >= 0; --i) { // Нові транзакції зверху
            crow::json::wvalue tx;
            tx["id"] = txs[i].id;
            tx["amount"] = txs[i].amount;
            tx["type"] = txs[i].isIncome ? "income" : "expense";
            tx["categoryId"] = txs[i].categoryId;
            tx["accountId"] = txs[i].accountId;
            
            Category cat;
            if (categoryStorage.getCategoryById(txs[i].categoryId, cat)) {
                tx["categoryName"] = std::string(cat.name);
                tx["icon"] = std::string(cat.icon);
                tx["color"] = std::string(cat.color);
            } else {
                tx["categoryName"] = "Інше";
                tx["icon"] = "💰";
                tx["color"] = "#cbd5e1";
            }

            Account acc;
            if (accountStorage.getAccountById(txs[i].accountId, acc)) {
                tx["accountName"] = std::string(acc.name);
            } else {
                tx["accountName"] = "Видалений рахунок";
            }
            
            txs[i].date[19] = '\0'; // Гарантуємо відсутність крашу пам'яті при читанні дати
            tx["date"] = std::string(txs[i].date);
            json_txs.push_back(tx);
        }
        
        auto res = crow::response(crow::json::wvalue(json_txs));
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // МАРШРУТ: Аналітика (Кругова діаграма)
    CROW_ROUTE(app, "/api/analytics")([&transactionService, &categoryStorage](const crow::request& req){
        // Читаємо параметр type (якщо "income", то це доходи)
        auto typeParam = req.url_params.get("type");
        bool isIncome = (typeParam && std::string(typeParam) == "income");

        CategoryTotal totals[10]; // Максимум 10 категорій
        int count = 0;
        
        // Отримуємо статистику згідно з типом (isIncome)
        transactionService.getAnalytics(isIncome, totals, count, 10);
        
        std::vector<std::string> labels;
        std::vector<double> dataArray;
        std::vector<std::string> colorsArray;
        
        for (int i = 0; i < count; ++i) {
            Category cat;
            // Отримуємо реальну назву категорії з бази даних
            if (categoryStorage.getCategoryById(totals[i].categoryId, cat)) {
                labels.push_back(std::string(cat.name));
                colorsArray.push_back(std::string(cat.color));
            } else {
                labels.push_back("Інше"); // Fallback, якщо категорію видалено
                colorsArray.push_back("#cbd5e1"); // Дефолтний колір
            }
            dataArray.push_back(totals[i].totalAmount);
        }
        
        crow::json::wvalue res_json;
        res_json["labels"] = labels;
        res_json["data"] = dataArray;
        res_json["colors"] = colorsArray;
        
        auto res = crow::response(res_json);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // ==========================================================
    // --- 1. Отримання всіх рахунків (GET /api/accounts) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/accounts")([&accountStorage](){
        try {
            Account accounts[100];
            int count = 0;
            accountStorage.getAllAccounts(accounts, count, 100);

            std::vector<crow::json::wvalue> json_accounts;
            for (int i = 0; i < count; ++i) {
                crow::json::wvalue acc;
                acc["id"] = accounts[i].id;
                acc["name"] = std::string(accounts[i].name);
                acc["balance"] = accounts[i].balance;
                acc["icon"] = std::string(accounts[i].icon); // Гарантовано конвертується з char[]
                json_accounts.push_back(acc);
            }

            auto res = crow::response(crow::json::wvalue(json_accounts));
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            auto res = crow::response(500, std::string("Server Error: ") + e.what());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });

    // ==========================================================
    // --- 2. Створення нового рахунку (POST /api/account) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/account").methods(crow::HTTPMethod::POST)
    ([&accountStorage](const crow::request& req) {
        try {
            auto data = crow::json::load(req.body);
            if (!data) {
                auto res = crow::response(400, "Invalid JSON");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            // Перевірка наявності полів
            if (!data.has("name") || !data.has("balance") || !data.has("icon")) {
                auto res = crow::response(400, "Missing required fields");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            // Сувора перевірка типів JSON
            if (data["name"].t() != crow::json::type::String || 
                data["balance"].t() != crow::json::type::Number ||
                data["icon"].t() != crow::json::type::String) {
                auto res = crow::response(400, "Invalid field types");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            Account newAcc = {};
            newAcc.id = static_cast<int>(std::time(nullptr)); // Генеруємо унікальний ID через Timestamp
            newAcc.balance = data["balance"].d();
            
            // Безпечне копіювання рядків
            std::string nameStr = data["name"].s();
            std::strncpy(newAcc.name, nameStr.c_str(), sizeof(newAcc.name) - 1);
            newAcc.name[sizeof(newAcc.name) - 1] = '\0';

            std::string iconStr = data["icon"].s();
            std::strncpy(newAcc.icon, iconStr.c_str(), sizeof(newAcc.icon) - 1);
            newAcc.icon[sizeof(newAcc.icon) - 1] = '\0';

            accountStorage.addAccount(newAcc);

            auto res = crow::response(200, "Account created successfully");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            auto res = crow::response(500, std::string("Server Error: ") + e.what());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });

    // ==========================================================
    // --- 2.1. Редагування рахунку (PUT /api/account) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/account").methods(crow::HTTPMethod::PUT)
    ([&accountStorage](const crow::request& req) {
        try {
            auto data = crow::json::load(req.body);
            if (!data) return crow::response(400, "Invalid JSON");

            if (!data.has("id") || !data.has("name") || !data.has("balance") || !data.has("icon")) {
                return crow::response(400, "Missing required fields");
            }

            Account acc;
            int accId = data["id"].i();
            if (!accountStorage.getAccountById(accId, acc)) {
                auto res = crow::response(404, "Account not found");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            acc.balance = data["balance"].d();

            auto safeCopy = [](char* dest, const std::string& src, size_t maxLen) {
                std::strncpy(dest, src.c_str(), maxLen - 1);
                dest[maxLen - 1] = '\0';
            };

            safeCopy(acc.name, data["name"].s(), sizeof(acc.name));
            safeCopy(acc.icon, data["icon"].s(), sizeof(acc.icon));

            accountStorage.updateAccount(acc);

            auto res = crow::response(200, "Account updated successfully");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Server Error: ") + e.what());
        }
    });

    // ==========================================================
    // --- 2.2. Видалення рахунку (DELETE /api/account/<id>) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/account/<int>").methods(crow::HTTPMethod::DELETE)
    ([&accountStorage](int id) {
        try {
            if (accountStorage.deleteAccount(id)) {
                auto res = crow::response(200, "Account deleted successfully");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            auto res = crow::response(404, "Account not found");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Server Error: ") + e.what());
        }
    });

    // ==========================================================
    // --- 3. Отримання всіх категорій (GET /api/categories) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/categories")([&categoryStorage](){
        try {
            Category categories[100];
            int count = 0;
            categoryStorage.getAllCategories(categories, count, 100);

            std::vector<crow::json::wvalue> json_cats;
            for (int i = 0; i < count; ++i) {
                crow::json::wvalue cat;
                cat["id"] = categories[i].id;
                cat["name"] = std::string(categories[i].name);
                cat["isIncome"] = categories[i].isIncome;
                cat["icon"] = std::string(categories[i].icon);
                cat["color"] = std::string(categories[i].color);
                json_cats.push_back(cat);
            }

            auto res = crow::response(crow::json::wvalue(json_cats));
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            auto res = crow::response(500, std::string("Server Error: ") + e.what());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });

    // ==========================================================
    // --- 4. Створення нової категорії (POST /api/category) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/category").methods(crow::HTTPMethod::POST)
    ([&categoryStorage](const crow::request& req) {
        try {
            auto data = crow::json::load(req.body);
            if (!data) return crow::response(400, "Invalid JSON");

            if (!data.has("name") || !data.has("isIncome") || !data.has("icon") || !data.has("color")) {
                return crow::response(400, "Missing required fields");
            }

            Category newCat = {};
            newCat.id = static_cast<int>(std::time(nullptr)); 
            newCat.isIncome = data["isIncome"].b();

            // Максимально безпечне копіювання з допомогою лямбди
            auto safeCopy = [](char* dest, const std::string& src, size_t maxLen) {
                std::strncpy(dest, src.c_str(), maxLen - 1);
                dest[maxLen - 1] = '\0';
            };

            safeCopy(newCat.name, data["name"].s(), sizeof(newCat.name));
            safeCopy(newCat.icon, data["icon"].s(), sizeof(newCat.icon));
            safeCopy(newCat.color, data["color"].s(), sizeof(newCat.color));

            categoryStorage.addCategory(newCat);

            auto res = crow::response(200, "Category created successfully");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Server Error: ") + e.what());
        }
    });

    // ==========================================================
    // --- 5. Редагування категорії (PUT /api/category) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/category").methods(crow::HTTPMethod::PUT)
    ([&categoryStorage](const crow::request& req) {
        try {
            auto data = crow::json::load(req.body);
            if (!data) return crow::response(400, "Invalid JSON");

            if (!data.has("id") || !data.has("name") || !data.has("isIncome") || !data.has("icon") || !data.has("color")) {
                return crow::response(400, "Missing required fields");
            }

            Category cat;
            int catId = data["id"].i();
            if (!categoryStorage.getCategoryById(catId, cat)) {
                auto res = crow::response(404, "Category not found");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            cat.isIncome = data["isIncome"].b();
            
            // cat.limit = ... (Якщо в майбутньому повернеш поле limit у структуру Category, додай сюди парсинг)

            auto safeCopy = [](char* dest, const std::string& src, size_t maxLen) {
                std::strncpy(dest, src.c_str(), maxLen - 1);
                dest[maxLen - 1] = '\0';
            };

            safeCopy(cat.name, data["name"].s(), sizeof(cat.name));
            safeCopy(cat.icon, data["icon"].s(), sizeof(cat.icon));
            safeCopy(cat.color, data["color"].s(), sizeof(cat.color));

            categoryStorage.updateCategory(cat);

            auto res = crow::response(200, "Category updated successfully");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Server Error: ") + e.what());
        }
    });

    // ==========================================================
    // --- 6. Видалення категорії (DELETE /api/category/<id>) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/category/<int>").methods(crow::HTTPMethod::DELETE)
    ([&categoryStorage](int id) {
        try {
            if (categoryStorage.deleteCategory(id)) {
                auto res = crow::response(200, "Category deleted successfully");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            auto res = crow::response(404, "Category not found");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            auto res = crow::response(500, std::string("Server Error: ") + e.what());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });

    // ==========================================================
    // --- 7. Видалення транзакції (DELETE /api/transaction/<id>) ---
    // ==========================================================
    CROW_ROUTE(app, "/api/transaction/<int>").methods(crow::HTTPMethod::DELETE)
    ([&transactionService](int id) {
        try {
            if (transactionService.deleteTransaction(id)) {
                auto res = crow::response(200, "Transaction deleted successfully");
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }
            auto res = crow::response(404, "Transaction not found");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        } catch (const std::exception& e) {
            auto res = crow::response(500, std::string("Server Error: ") + e.what());
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }
    });

    app.port(8085).multithreaded().run();
    return 0;
}