#pragma once

struct Transaction {
    int id;
    double amount;
    bool isIncome; 
    char date[20]; // формат "DD-MM-YYYY"
    int categoryId;
    int accountId;
};