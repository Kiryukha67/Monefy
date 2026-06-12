#include "AuthService.h"
#include <iostream>
#include <cstring>

int AuthService::generateNewUserId() {
    User users[200];
    int count = 0;
    userStorage.getAllUsers(users, count, 200);
    int maxId = 0;
    for (int i = 0; i < count; ++i) {
        if (users[i].id > maxId) {
            maxId = users[i].id;
        }
    }
    return maxId + 1;
}

bool AuthService::login(const char* username, const char* password, User& outUser) {
    return userStorage.authenticateUser(username, password, outUser);
}

bool AuthService::registerUser(const char* username, const char* password, User& outUser) {
    if (userStorage.isEmailTaken(username)) return false;

    User newUser;
    newUser.id = generateNewUserId();
    std::snprintf(newUser.name, sizeof(newUser.name), "%s", username);
    std::snprintf(newUser.email, sizeof(newUser.email), "%s", username);
    std::snprintf(newUser.passwordHash, sizeof(newUser.passwordHash), "%s", password);
    
    userStorage.addUser(newUser);
    outUser = newUser;
    return true;
}