#pragma once

struct Account {
    int id;
    char name[64];
    double balance;
    int currencyId;
    char icon[16]; // <--- Додано поле для емодзі/іконки
};
