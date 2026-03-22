#include "UserStorage.h"
#include <fstream>
#include <cstdio> // для функцій remove та rename

void UserStorage::addUser(const User& user) {
    // Відкриваємо файл для додавання (app) у бінарному режимі (binary)
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&user), sizeof(User));
        file.close();
    }
}

bool UserStorage::getUserById(int id, User& outUser) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    User temp;
    // Читаємо файл блок за блоком (розміром з нашу структуру)
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(User))) {
        if (temp.id == id) {
            outUser = temp; // Знайшли! Передаємо дані через параметр
            file.close();
            return true;
        }
    }
    file.close();
    return false; // Користувача з таким ID не знайдено
}

void UserStorage::getAllUsers(User* outArray, int& count, int maxCount) {
    count = 0;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;

    User temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(User))) {
        if (count < maxCount) {
            outArray[count] = temp;
            count++;
        } else {
            break; // Захист від переповнення масиву
        }
    }
    file.close();
}

bool UserStorage::updateUser(const User& updatedUser) {
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return false;

    User temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(User))) {
        if (temp.id == updatedUser.id) {
            // Повертаємо курсор файлу назад на розмір однієї структури
            file.seekp(-static_cast<int>(sizeof(User)), std::ios::cur);
            // Перезаписуємо новими даними
            file.write(reinterpret_cast<const char*>(&updatedUser), sizeof(User));
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

bool UserStorage::deleteUser(int id) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) return false;

    const char* tempFilename = "temp_users.dat";
    std::ofstream outFile(tempFilename, std::ios::binary);
    if (!outFile.is_open()) {
        inFile.close();
        return false;
    }

    bool found = false;
    User temp;
    while (inFile.read(reinterpret_cast<char*>(&temp), sizeof(User))) {
        if (temp.id != id) {
            // Записуємо у тимчасовий файл всіх, КРІМ того, кого треба видалити
            outFile.write(reinterpret_cast<const char*>(&temp), sizeof(User));
        } else {
            found = true;
        }
    }

    inFile.close();
    outFile.close();

    // Замінюємо старий файл новим
    if (found) {
        std::remove(filename);
        std::rename(tempFilename, filename);
    } else {
        std::remove(tempFilename); // Якщо не знайшли, просто видаляємо тимчасовий файл
    }

    return found;
}