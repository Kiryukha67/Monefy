#include "crow.h"
#include "storage/AccountStorage.h"
#include "services/TransactionService.h"

int main() {
    crow::SimpleApp app;
    AccountStorage accountStorage;

    // Створюємо базовий рахунок
    Account defaultAcc;
    if (!accountStorage.getAccountById(1, defaultAcc)) {
        defaultAcc = {1, "Main Card", 5000.0, 1};
        accountStorage.addAccount(defaultAcc);
    }

    CROW_ROUTE(app, "/")([](){
        return "Monefy Web Server is running! 🚀";
    });

    CROW_ROUTE(app, "/api/balance")([&accountStorage](){
        Account acc;
        if (accountStorage.getAccountById(1, acc)) {
            crow::json::wvalue json_resp;
            json_resp["id"] = acc.id;
            json_resp["name"] = acc.name;
            json_resp["balance"] = acc.balance;
            
            // Формируем ответ и добавляем "ПРОПУСК" для браузера (CORS)
            auto res = crow::response(json_resp);
            res.add_header("Access-Control-Allow-Origin", "*"); 
            return res;
        }
        
        auto res = crow::response(404, "Account not found");
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    app.port(8080).multithreaded().run();
    return 0;
}