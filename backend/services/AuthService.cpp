#include "AuthService.h"
#include <iostream> // For debugging, can be removed
#include <cstring>  // For std::strcmp

// Helper function: finds the largest ID in the file and returns the next one
int AuthService::generateNewUserId() {
    User users[100]; // Temporary array (assuming max 100 users for simplicity)
    int count = 0;
    userStorage.getAllUsers(users, count, 100);

    int maxId = 0;
    for (int i = 0; i < count; ++i) {
        if (users[i].id > maxId) {
            maxId = users[i].id;
        }
    }
    return maxId + 1; // Next available ID
}

bool AuthService::login(const char* username, const char* password, User& outUser) {
    User users[100];
    int count = 0;
    userStorage.getAllUsers(users, count, 100);

    for (int i = 0; i < count; ++i) {
        if (std::strcmp(users[i].username, username) == 0) {
            // Found user, now check password
            if (std::strcmp(users[i].passwordHash, password) == 0) {
                outUser = users[i];
                return true; // Login successful
            } else {
                return false; // Incorrect password
            }
        }
    }
    return false; // User not found
}

bool AuthService::registerUser(const char* username, const char* password, User& outUser) {
    // Check if username already exists
    User users[100];
    int count = 0;
    userStorage.getAllUsers(users, count, 100);

    for (int i = 0; i < count; ++i) {
        if (std::strcmp(users[i].username, username) == 0) {
            return false; // Username already taken
        }
    }

    // Create new user
    User newUser;
    newUser.id = generateNewUserId();
    newUser.setUsername(username);
    newUser.setPasswordHash(password); 

    userStorage.addUser(newUser);
    outUser = newUser;
    return true; // Registration successful
}