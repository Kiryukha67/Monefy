#pragma once
#include "../models/Category.h"

class CategoryStorage {
private:
    const char* filename = "categories.dat";
public:
    void addCategory(const Category& category);
    bool getCategoryById(int id, Category& outCategory);
    void getAllCategories(Category* outArray, int& count, int maxCount);
    bool updateCategory(const Category& updatedCategory);
    bool deleteCategory(int id);
};