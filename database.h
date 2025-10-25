#pragma once

#include <string>
#include <sqlite_orm/sqlite_orm.h>

namespace db {

// Represents a user in the database
struct User {
    int64_t id;
    std::string first_name;
    std::string username;
    bool is_banned;
    std::string created_at;
};

// Represents a key-value setting in the database
struct Setting {
    std::string key;
    std::string value;
};

// Represents a statistic counter in the database
struct Stat {
    std::string key;
    int64_t value;
};


// Creates and returns a storage (database connection) object.
// The database file is named "bot_database.sqlite".
// This function also syncs the schema, creating tables if they don't exist.
inline auto init_storage(const std::string& path) {
    using namespace sqlite_orm;
    return make_storage(path,
                        make_table("users",
                                   make_column("id", &User::id, primary_key()),
                                   make_column("first_name", &User::first_name),
                                   make_column("username", &User::username),
                                   make_column("is_banned", &User::is_banned, default_value(false)),
                                   make_column("created_at", &User::created_at)),
                        make_table("settings",
                                   make_column("key", &Setting::key, primary_key()),
                                   make_column("value", &Setting::value)),
                        make_table("stats",
                                   make_column("key", &Stat::key, primary_key()),
                                   make_column("value", &Stat::value)));
}

using Storage = decltype(init_storage(""));

} // namespace db
