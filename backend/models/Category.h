#pragma once
#include <cstring>

struct Category {
    int id;
    int userId;
    char name[50];
    bool isIncome; 
    char color[10];
    char icon[16]; 
    double limit; 

    Category() : id(0), userId(0), isIncome(false), limit(0.0) {
        std::memset(name, 0, sizeof(name));
        std::memset(color, 0, sizeof(color));
        std::memset(icon, 0, sizeof(icon));
    }

    void setName(const char* newName) {
        std::strncpy(name, newName, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }
    void setColor(const char* newColor) {
        std::strncpy(color, newColor, sizeof(color) - 1);
        color[sizeof(color) - 1] = '\0';
    }
    void setIcon(const char* newIcon) {
        std::strncpy(icon, newIcon, sizeof(icon) - 1);
        icon[sizeof(icon) - 1] = '\0';
    }
};
