cmake_minimum_required(VERSION 3.11)
project(TgYoutubeBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(TgBot REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# Add sqlite_orm library from the vendor directory
include_directories(vendor/sqlite_orm/include)

add_executable(TgYoutubeBot main.cpp)

target_link_libraries(TgYoutubeBot PRIVATE
    TgBot::TgBot
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    Threads::Threads
    SQLite::SQLite3
)
