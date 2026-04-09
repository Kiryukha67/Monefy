#include "CategoryStorage.h"
#include <fstream>
#include <cstdio>

void CategoryStorage::addCategory(const Category& category) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&category), sizeof(Category));
        file.close();
    }
}

bool CategoryStorage::getCategoryById(int id, Category& outCategory) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    Category temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Category))) {
        if (temp.id == id) {
            outCategory = temp;
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

void CategoryStorage::getAllCategories(Category* outArray, int& count, int maxCount) {
    count = 0;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
    Category temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Category))) {
        if (count < maxCount) { outArray[count++] = temp; } 
        else break;
    }
    file.close();
}

bool CategoryStorage::updateCategory(const Category& updatedCategory) {
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return false;
    Category temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Category))) {
        if (temp.id == updatedCategory.id) {
            file.seekp(-static_cast<int>(sizeof(Category)), std::ios::cur);
            file.write(reinterpret_cast<const char*>(&updatedCategory), sizeof(Category));
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

bool CategoryStorage::deleteCategory(int id) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) return false;
    const char* tempFilename = "temp_categories.dat";
    std::ofstream outFile(tempFilename, std::ios::binary);
    if (!outFile.is_open()) { inFile.close(); return false; }

    bool found = false;
    Category temp;
    while (inFile.read(reinterpret_cast<char*>(&temp), sizeof(Category))) {
        if (temp.id != id) {
            outFile.write(reinterpret_cast<const char*>(&temp), sizeof(Category));
        } else { found = true; }
    }
    inFile.close(); outFile.close();
    if (found) { std::remove(filename); std::rename(tempFilename, filename); } 
    else { std::remove(tempFilename); }
    return found;
}