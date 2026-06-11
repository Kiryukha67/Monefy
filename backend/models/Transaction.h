#pragma once

struct Transaction {
    int id;
    int userId;
    double amount;
    bool isIncome; 
    char date[20]; // формат "DD-MM-YYYY"
    int categoryId;
    int accountId;
};
