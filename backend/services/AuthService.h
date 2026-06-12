#pragma once
#include "../storage/UserStorage.h"
#include "../models/User.h"
#include <string>

class AuthService {
private:
    UserStorage userStorage;
    int generateNewUserId();
public:
    bool login(const char* username, const char* password, User& outUser);
    bool registerUser(const char* username, const char* password, User& outUser);
};