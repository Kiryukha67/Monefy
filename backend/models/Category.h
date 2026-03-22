#pragma once

struct Category {
    int id;
    char name[50];
    bool isIncome; // true - дохід, false - витрата (замість string)
};