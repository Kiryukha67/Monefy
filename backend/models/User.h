#pragma once
#include <cstring>

struct User {
    int id;
    char name[50];
    char email[100];
    char passwordHash[128];

    // Безпечна ініціалізація нулями
    User() : id(0) {
        std::memset(name, 0, sizeof(name));
        std::memset(email, 0, sizeof(email));
        std::memset(passwordHash, 0, sizeof(passwordHash));
    }
};