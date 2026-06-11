#include "crow.h"
#include "crow/middlewares/cors.h"
#include "storage/AccountStorage.h"
#include "storage/CategoryStorage.h"
#include "storage/UserStorage.h"
#include "services/TransactionService.h"
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#undef DELETE
#endif

int getAuthenticatedUserId(const crow::request& req, UserStorage& userStorage) {
    std::string header = req.get_header_value("Authorization");
    const std::string bearerPrefix = "Bearer ";
    const std::string tokenPrefix = "token-";

    if (header.rfind(bearerPrefix, 0) == 0) {
        header = header.substr(bearerPrefix.size());
    }

    if (header.rfind(tokenPrefix, 0) != 0) return 0;

    try {
        int userId = std::stoi(header.substr(tokenPrefix.size()));
        User user;
        return userStorage.getUserById(userId, user) ? userId : 0;
    } catch (...) {
        return 0;
    }
}

crow::response unauthorizedResponse() {
    return crow::response(401, "Unauthorized");
}

void safeCopy(char* dest, const std::string& src, size_t maxLen) {
    std::strncpy(dest, src.c_str(), maxLen - 1);
    dest[maxLen - 1] = '\0';
}

int nextUserId(UserStorage& userStorage) {
    User users[200];
    int count = 0;
    userStorage.getAllUsers(users, count, 200);

    int maxId = 0;
    for (int i = 0; i < count; ++i) {
        if (users[i].id > maxId) maxId = users[i].id;
    }
    return maxId + 1;
}

int nextAccountId(AccountStorage& accountStorage) {
    Account accounts[200];
    int count = 0;
    accountStorage.getAllAccounts(accounts, count, 200);

    int maxId = 0;
    for (int i = 0; i < count; ++i) {
        if (accounts[i].id > maxId) maxId = accounts[i].id;
    }
    return maxId + 1;
}

int nextCategoryId(CategoryStorage& categoryStorage) {
    Category categories[200];
    int count = 0;
    categoryStorage.getAllCategories(categories, count, 200);

    int maxId = 0;
    for (int i = 0; i < count; ++i) {
        if (categories[i].id > maxId) maxId = categories[i].id;
    }
    return maxId + 1;
}

int main() {
    crow::App<crow::CORSHandler> app;
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization", "Accept")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET, crow::HTTPMethod::PUT, crow::HTTPMethod::DELETE, crow::HTTPMethod::OPTIONS)
        .origin("*");

    AccountStorage accountStorage;
    CategoryStorage categoryStorage;
    UserStorage userStorage;
    TransactionService transactionService;

    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([&userStorage](const crow::request& req) {
        try {
            auto data = crow::json::load(req.body);
            if (!data || !data.has("username") || !data.has("password")) {
                return crow::response(400, "Missing required fields");
            }

            std::string username = data["username"].s();
            std::string password = data["password"].s();

            if (username.length() < 3 || password.length() < 3) {
                return crow::response(400, "Login and password must be at least 3 characters");
            }
            if (userStorage.isEmailTaken(username.c_str())) {
                return crow::response(400, "User already exists");
            }

            User newUser = {};
            newUser.id = nextUserId(userStorage);
            safeCopy(newUser.name, username, sizeof(newUser.name));
            safeCopy(newUser.email, username, sizeof(newUser.email));
            safeCopy(newUser.passwordHash, password, sizeof(newUser.passwordHash));
            userStorage.addUser(newUser);

            crow::json::wvalue res;
            res["token"] = "token-" + std::to_string(newUser.id);
            res["userId"] = newUser.id;
            res["username"] = std::string(newUser.name);
            return crow::response(200, res);
        } catch (const std::exception& e) {
            return crow::response(500, std::string("Registration Error: ") + e.what());
        }
    });

    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([&userStorage](const crow::request& req) {
        try {
            auto data = crow::json::load(req.body);
            if (!data || !data.has("username") || !data.has("password")) {
                return crow::response(400, "Missing required fields");
            }

            User user;
            std::string username = data["username"].s();
            std::string password = data["password"].s();
            if (!userStorage.authenticateUser(username.c_str(), password.c_str(), user)) {
                return crow::response(401, "Invalid login or password");
            }

            crow::json::wvalue res;
            res["token"] = "token-" + std::to_string(user.id);
            res["userId"] = user.id;
            res["username"] = std::string(user.name);
            return crow::response(200, res);
        } catch (...) {
            return crow::response(500, "Server Error");
        }
    });

    CROW_ROUTE(app, "/api/accounts")([&accountStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        Account accounts[200];
        int count = 0;
        accountStorage.getAllAccounts(accounts, count, 200);

        std::vector<crow::json::wvalue> jsonAccounts;
        for (int i = 0; i < count; ++i) {
            if (accounts[i].userId != userId) continue;
            crow::json::wvalue acc;
            acc["id"] = accounts[i].id;
            acc["userId"] = accounts[i].userId;
            acc["name"] = std::string(accounts[i].name);
            acc["balance"] = accounts[i].balance;
            acc["icon"] = std::string(accounts[i].icon);
            jsonAccounts.push_back(acc);
        }
        return crow::response(crow::json::wvalue(jsonAccounts));
    });

    CROW_ROUTE(app, "/api/balance")([&accountStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        Account accounts[200];
        int count = 0;
        accountStorage.getAllAccounts(accounts, count, 200);

        double total = 0.0;
        for (int i = 0; i < count; ++i) {
            if (accounts[i].userId == userId) total += accounts[i].balance;
        }

        crow::json::wvalue res;
        res["balance"] = total;
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/account").methods(crow::HTTPMethod::POST)
    ([&accountStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        auto data = crow::json::load(req.body);
        if (!data || !data.has("name") || !data.has("balance") || !data.has("icon")) {
            return crow::response(400, "Missing required fields");
        }

        Account acc = {};
        acc.id = nextAccountId(accountStorage);
        acc.userId = userId;
        acc.balance = data["balance"].d();
        safeCopy(acc.name, data["name"].s(), sizeof(acc.name));
        safeCopy(acc.icon, data["icon"].s(), sizeof(acc.icon));
        accountStorage.addAccount(acc);
        return crow::response(200, "Account created successfully");
    });

    CROW_ROUTE(app, "/api/account").methods(crow::HTTPMethod::PUT)
    ([&accountStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        auto data = crow::json::load(req.body);
        if (!data || !data.has("id") || !data.has("name") || !data.has("balance") || !data.has("icon")) {
            return crow::response(400, "Missing required fields");
        }

        Account acc;
        int id = static_cast<int>(data["id"].d());
        if (!accountStorage.getAccountById(id, acc) || acc.userId != userId) {
            return crow::response(404, "Account not found");
        }

        acc.balance = data["balance"].d();
        safeCopy(acc.name, data["name"].s(), sizeof(acc.name));
        safeCopy(acc.icon, data["icon"].s(), sizeof(acc.icon));
        accountStorage.updateAccount(acc);
        return crow::response(200, "Account updated successfully");
    });

    CROW_ROUTE(app, "/api/account/<int>").methods(crow::HTTPMethod::DELETE)
    ([&accountStorage, &userStorage](const crow::request& req, int id) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        Account acc;
        if (!accountStorage.getAccountById(id, acc) || acc.userId != userId) {
            return crow::response(404, "Account not found");
        }
        return accountStorage.deleteAccount(id)
            ? crow::response(200, "Account deleted successfully")
            : crow::response(404, "Account not found");
    });

    CROW_ROUTE(app, "/api/categories")([&categoryStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        Category categories[200];
        int count = 0;
        categoryStorage.getAllCategories(categories, count, 200);

        std::vector<crow::json::wvalue> jsonCategories;
        for (int i = 0; i < count; ++i) {
            if (categories[i].userId != userId) continue;
            crow::json::wvalue cat;
            cat["id"] = categories[i].id;
            cat["userId"] = categories[i].userId;
            cat["name"] = std::string(categories[i].name);
            cat["isIncome"] = categories[i].isIncome;
            cat["icon"] = std::string(categories[i].icon);
            cat["color"] = std::string(categories[i].color);
            cat["limit"] = categories[i].limit;
            jsonCategories.push_back(cat);
        }
        return crow::response(crow::json::wvalue(jsonCategories));
    });

    CROW_ROUTE(app, "/api/category").methods(crow::HTTPMethod::POST)
    ([&categoryStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        auto data = crow::json::load(req.body);
        if (!data || !data.has("name") || !data.has("isIncome") || !data.has("icon") || !data.has("color")) {
            return crow::response(400, "Missing required fields");
        }

        Category cat = {};
        cat.id = nextCategoryId(categoryStorage);
        cat.userId = userId;
        cat.isIncome = data["isIncome"].b();
        cat.limit = data.has("limit") ? data["limit"].d() : 0.0;
        safeCopy(cat.name, data["name"].s(), sizeof(cat.name));
        safeCopy(cat.icon, data["icon"].s(), sizeof(cat.icon));
        safeCopy(cat.color, data["color"].s(), sizeof(cat.color));
        categoryStorage.addCategory(cat);
        return crow::response(200, "Category created successfully");
    });

    CROW_ROUTE(app, "/api/category").methods(crow::HTTPMethod::PUT)
    ([&categoryStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        auto data = crow::json::load(req.body);
        if (!data || !data.has("id") || !data.has("name") || !data.has("isIncome") || !data.has("icon") || !data.has("color")) {
            return crow::response(400, "Missing required fields");
        }

        Category cat;
        int id = static_cast<int>(data["id"].d());
        if (!categoryStorage.getCategoryById(id, cat) || cat.userId != userId) {
            return crow::response(404, "Category not found");
        }

        cat.isIncome = data["isIncome"].b();
        cat.limit = data.has("limit") ? data["limit"].d() : cat.limit;
        safeCopy(cat.name, data["name"].s(), sizeof(cat.name));
        safeCopy(cat.icon, data["icon"].s(), sizeof(cat.icon));
        safeCopy(cat.color, data["color"].s(), sizeof(cat.color));
        categoryStorage.updateCategory(cat);
        return crow::response(200, "Category updated successfully");
    });

    CROW_ROUTE(app, "/api/category/<int>").methods(crow::HTTPMethod::DELETE)
    ([&categoryStorage, &userStorage](const crow::request& req, int id) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        Category cat;
        if (!categoryStorage.getCategoryById(id, cat) || cat.userId != userId) {
            return crow::response(404, "Category not found");
        }
        return categoryStorage.deleteCategory(id)
            ? crow::response(200, "Category deleted successfully")
            : crow::response(404, "Category not found");
    });

    CROW_ROUTE(app, "/api/transaction").methods(crow::HTTPMethod::POST)
    ([&transactionService, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        auto data = crow::json::load(req.body);
        if (!data || !data.has("amount") || !data.has("isIncome") || !data.has("categoryId") || !data.has("accountId") || !data.has("date")) {
            return crow::response(400, "Missing required fields");
        }

        double amount = data["amount"].d();
        bool isIncome = data["isIncome"].b();
        int categoryId = static_cast<int>(data["categoryId"].d());
        int accountId = static_cast<int>(data["accountId"].d());
        std::string date = data["date"].s();

        if (transactionService.addTransaction(amount, isIncome, date.c_str(), categoryId, accountId, userId)) {
            return crow::response(200, "Success");
        }
        return crow::response(400, "Transaction failed");
    });

    CROW_ROUTE(app, "/api/transfer").methods(crow::HTTPMethod::POST)
    ([&transactionService, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        auto data = crow::json::load(req.body);
        if (!data || !data.has("fromAccountId") || !data.has("toAccountId") || !data.has("amount") || !data.has("date")) {
            return crow::response(400, "Missing required fields");
        }

        int fromAccountId = static_cast<int>(data["fromAccountId"].d());
        int toAccountId = static_cast<int>(data["toAccountId"].d());
        double amount = data["amount"].d();
        std::string date = data["date"].s();

        return transactionService.transferFunds(fromAccountId, toAccountId, amount, date.c_str(), userId)
            ? crow::response(200, "Transfer successful")
            : crow::response(400, "Transfer failed");
    });

    CROW_ROUTE(app, "/api/transactions")([&transactionService, &categoryStorage, &accountStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        Transaction txs[200];
        int count = 0;
        transactionService.getTransactionHistory(userId, txs, count, 200);

        std::vector<crow::json::wvalue> jsonTxs;
        for (int i = count - 1; i >= 0; --i) {
            crow::json::wvalue tx;
            tx["id"] = txs[i].id;
            tx["userId"] = txs[i].userId;
            tx["amount"] = txs[i].amount;
            tx["type"] = txs[i].isIncome ? "income" : "expense";
            tx["categoryId"] = txs[i].categoryId;
            tx["accountId"] = txs[i].accountId;
            tx["date"] = std::string(txs[i].date);

            Category cat;
            if (txs[i].categoryId != 0 && categoryStorage.getCategoryById(txs[i].categoryId, cat) && cat.userId == userId) {
                tx["categoryName"] = std::string(cat.name);
                tx["icon"] = std::string(cat.icon);
                tx["color"] = std::string(cat.color);
            } else {
                tx["categoryName"] = "Other";
                tx["icon"] = "";
                tx["color"] = "#cbd5e1";
            }

            Account acc;
            if (accountStorage.getAccountById(txs[i].accountId, acc) && acc.userId == userId) {
                tx["accountName"] = std::string(acc.name);
            } else {
                tx["accountName"] = "Deleted account";
            }

            jsonTxs.push_back(tx);
        }
        return crow::response(crow::json::wvalue(jsonTxs));
    });

    CROW_ROUTE(app, "/api/analytics")([&transactionService, &categoryStorage, &userStorage](const crow::request& req) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        auto typeParam = req.url_params.get("type");
        bool isIncome = (typeParam && std::string(typeParam) == "income");

        CategoryTotal totals[50];
        int count = 0;
        transactionService.getAnalytics(isIncome, userId, totals, count, 50);

        std::vector<std::string> labels;
        std::vector<double> data;
        std::vector<std::string> colors;

        for (int i = 0; i < count; ++i) {
            Category cat;
            if (categoryStorage.getCategoryById(totals[i].categoryId, cat) && cat.userId == userId) {
                labels.push_back(std::string(cat.name));
                colors.push_back(std::string(cat.color));
            } else {
                labels.push_back("Other");
                colors.push_back("#cbd5e1");
            }
            data.push_back(totals[i].totalAmount);
        }

        crow::json::wvalue res;
        res["labels"] = labels;
        res["data"] = data;
        res["colors"] = colors;
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/transaction/<int>").methods(crow::HTTPMethod::DELETE)
    ([&transactionService, &userStorage](const crow::request& req, int id) {
        int userId = getAuthenticatedUserId(req, userStorage);
        if (!userId) return unauthorizedResponse();

        return transactionService.deleteTransaction(id, userId)
            ? crow::response(200, "Transaction deleted successfully")
            : crow::response(404, "Transaction not found");
    });

    std::cout << "\n==================================================" << std::endl;
    std::cout << "Monefy Server is running on http://localhost:8085" << std::endl;
    std::cout << "==================================================\n" << std::endl;

    app.port(8085).multithreaded().run();
    return 0;
}
