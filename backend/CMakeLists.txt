cmake_minimum_required(VERSION 3.10)
project(MonefyBackend)

# Додаємо всі наші файли до збірки
add_executable(monefy_server 
    main.cpp 
    storage/UserStorage.cpp 
    storage/CategoryStorage.cpp 
    storage/AccountStorage.cpp 
    storage/TransactionStorage.cpp 
    services/TransactionService.cpp
)