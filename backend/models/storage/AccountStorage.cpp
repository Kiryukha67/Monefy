#include "AccountStorage.h"
#include <fstream>
#include <cstdio>

void AccountStorage::addAccount(const Account& account) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&account), sizeof(Account));
        file.close();
    }
}

bool AccountStorage::getAccountById(int id, Account& outAccount) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    Account temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Account))) {
        if (temp.id == id) {
            outAccount = temp;
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

void AccountStorage::getAllAccounts(Account* outArray, int& count, int maxCount) {
    count = 0;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
    Account temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Account))) {
        if (count < maxCount) { outArray[count++] = temp; } 
        else break;
    }
    file.close();
}

bool AccountStorage::updateAccount(const Account& updatedAccount) {
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return false;
    Account temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Account))) {
        if (temp.id == updatedAccount.id) {
            file.seekp(-static_cast<int>(sizeof(Account)), std::ios::cur);
            file.write(reinterpret_cast<const char*>(&updatedAccount), sizeof(Account));
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

bool AccountStorage::deleteAccount(int id) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) return false;
    const char* tempFilename = "temp_accounts.dat";
    std::ofstream outFile(tempFilename, std::ios::binary);
    if (!outFile.is_open()) { inFile.close(); return false; }

    bool found = false;
    Account temp;
    while (inFile.read(reinterpret_cast<char*>(&temp), sizeof(Account))) {
        if (temp.id != id) {
            outFile.write(reinterpret_cast<const char*>(&temp), sizeof(Account));
        } else { found = true; }
    }
    inFile.close(); outFile.close();
    if (found) { std::remove(filename); std::rename(tempFilename, filename); } 
    else { std::remove(tempFilename); }
    return found;
}