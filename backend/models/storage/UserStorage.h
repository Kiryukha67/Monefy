#pragma once
#include "../models/User.h"

class UserStorage {
private:
    const char* filename = "users.dat"; // Окремий файл зберігання для User

public:
    // Додавання запису
    void addUser(const User& user);
    
    // Пошук (читання одного запису за ID). Повертає true, якщо знайдено
    bool getUserById(int id, User& outUser);
    
    // Читання всіх записів. Передаємо масив, куди запишуться дані, і змінну count
    void getAllUsers(User* outArray, int& count, int maxCount);
    
    // Редагування запису
    bool updateUser(const User& updatedUser);
    
    // Видалення запису за ID
    bool deleteUser(int id);
};