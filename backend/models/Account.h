#pragma once

struct Account {
    int id;
    int userId;
    char name[64];
    double balance;
    int currencyId;
    char icon[16]; // <--- Додано поле для емодзі/іконки
};
