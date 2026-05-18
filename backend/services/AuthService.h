#pragma once
#include "../storage/UserStorage.h"
#include "../models/User.h"
#include <string> // For std::string

class AuthService {
private:
    UserStorage userStorage;

    // Helper to generate a new unique user ID
    int generateNewUserId();

public:
    // Attempts to log in a user. Returns true on success and populates outUser.
    bool login(const char* username, const char* password, User& outUser);

    // Attempts to register a new user. Returns true on success and populates outUser.
    bool registerUser(const char* username, const char* password, User& outUser);
};