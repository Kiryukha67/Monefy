#pragma once
#include <cstring>

struct User {
    int id;
    char username[50];
    char password[50];
    char token[64];

    // Безпечна ініціалізація нулями
    User() : id(0) {
        std::memset(username, 0, sizeof(username));
        std::memset(password, 0, sizeof(password));
        std::memset(token, 0, sizeof(token));
    }
};