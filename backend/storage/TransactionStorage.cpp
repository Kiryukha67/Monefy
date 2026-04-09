#include "TransactionStorage.h"
#include <fstream>
#include <cstdio>

void TransactionStorage::addTransaction(const Transaction& transaction) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(&transaction), sizeof(Transaction));
        file.close();
    }
}

bool TransactionStorage::getTransactionById(int id, Transaction& outTransaction) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    Transaction temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Transaction))) {
        if (temp.id == id) {
            outTransaction = temp;
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

void TransactionStorage::getAllTransactions(Transaction* outArray, int& count, int maxCount) {
    count = 0;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
    Transaction temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Transaction))) {
        if (count < maxCount) { outArray[count++] = temp; } 
        else break;
    }
    file.close();
}

bool TransactionStorage::updateTransaction(const Transaction& updatedTransaction) {
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return false;
    Transaction temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(Transaction))) {
        if (temp.id == updatedTransaction.id) {
            file.seekp(-static_cast<int>(sizeof(Transaction)), std::ios::cur);
            file.write(reinterpret_cast<const char*>(&updatedTransaction), sizeof(Transaction));
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

bool TransactionStorage::deleteTransaction(int id) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) return false;
    const char* tempFilename = "temp_transactions.dat";
    std::ofstream outFile(tempFilename, std::ios::binary);
    if (!outFile.is_open()) { inFile.close(); return false; }

    bool found = false;
    Transaction temp;
    while (inFile.read(reinterpret_cast<char*>(&temp), sizeof(Transaction))) {
        if (temp.id != id) {
            outFile.write(reinterpret_cast<const char*>(&temp), sizeof(Transaction));
        } else { found = true; }
    }
    inFile.close(); outFile.close();
    if (found) { std::remove(filename); std::rename(tempFilename, filename); } 
    else { std::remove(tempFilename); }
    return found;
}