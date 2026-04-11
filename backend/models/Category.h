#pragma once
#include <cstring>

struct Category {
    int id;
    char name[50];
    bool isIncome;  // <--- Додано тип (витрата/дохід)
    char color[10]; // <--- Додано колір
    char icon[16];  // <--- Додано поле для емодзі
    double limit;   // <--- Місячний ліміт витрат

    // Конструктор за замовчуванням: безпечна ініціалізація нулями
    Category() : id(0), isIncome(false), limit(0.0) {
        std::memset(name, 0, sizeof(name));
        std::memset(color, 0, sizeof(color));
        std::memset(icon, 0, sizeof(icon));
    }

    // Безпечні допоміжні функції для заповнення полів
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
