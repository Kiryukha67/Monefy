#pragma once

struct Transaction {
    int id;
    int userId;
    double amount;
    bool isIncome; 
    char date[20];
    int categoryId;
    int accountId;
};
