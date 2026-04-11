cmake_minimum_required(VERSION 3.15)
project(monefy_server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

# Прибираємо попередження про DOWNLOAD_EXTRACT_TIMESTAMP
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# --- 1. ВРУЧНУ ЗАВАНТАЖУЄМО ASIO ---
FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.zip
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "Asio includes" FORCE)

# --- 2. НАЛАШТОВУЄМО ТА ЗАВАНТАЖУЄМО CROW ---
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)

FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(Crow)

# --- 3. ЗБИРАЄМО ТВОЮ ПРОГРАМУ (вказуємо, що файли лежать у backend/) ---
add_executable(monefy_server 
    backend/main.cpp
    backend/storage/CategoryStorage.cpp
    backend/storage/AccountStorage.cpp
    backend/storage/TransactionStorage.cpp
    backend/services/TransactionService.cpp
)

# --- 4. ПІДКЛЮЧАЄМО ВСЕ РАЗОМ ---
target_include_directories(monefy_server PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(monefy_server PRIVATE Crow::Crow ws2_32 mswsock)