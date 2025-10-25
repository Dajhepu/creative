#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <regex>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Missing <filesystem> or <experimental/filesystem>"
#endif

#include <tgbot/tgbot.h>
#include "database.h" // Include the database header

using namespace std;
using namespace TgBot;
using namespace sqlite_orm;

// Global variables
long long ADMIN_USER_ID = 0;
atomic<bool> MAINTENANCE_MODE(false);

// Function prototypes
bool is_admin(const User::Ptr& user);
void handle_maintenance_command(const Bot& bot, const Message::Ptr& message, db::Storage& storage);
void handle_admin_callback(const Bot& bot, const CallbackQuery::Ptr& query, db::Storage& storage);


// --- Database & Utility Functions ---

bool is_admin(const User::Ptr& user) {
    return user && user->id == ADMIN_USER_ID;
}

bool is_banned(db::Storage& storage, long long user_id) {
    auto user = storage.get_pointer<db::User>(user_id);
    return user && user->is_banned;
}

// Settings functions
void set_setting(db::Storage& storage, const string& key, const string& value) {
    db::Setting setting{key, value};
    storage.replace(setting);
}

string get_setting(db::Storage& storage, const string& key, const string& default_value) {
    auto setting = storage.get_pointer<db::Setting>(key);
    return setting ? setting->value : default_value;
}


std::string get_current_time_str() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

void upsert_user(db::Storage& storage, const User::Ptr& user) {
    if (!user) return;
    try {
        if (!storage.get_pointer<db::User>(user->id)) {
            db::User new_user{user->id, user->firstName, user->username, false, get_current_time_str()};
            storage.insert(new_user);
            printf("New user registered: %s (ID: %ld)\n", new_user.first_name.c_str(), (long)new_user.id);
        } else {
             auto db_user = storage.get_pointer<db::User>(user->id);
             db_user->first_name = user->firstName;
             db_user->username = user->username;
             storage.update(*db_user);
        }
    } catch (const std::exception& e) {
        printf("Error upserting user %ld: %s\n", (long)user->id, e.what());
    }
}

void increment_stat(db::Storage& storage, const std::string& key) {
    try {
        auto stat = storage.get_pointer<db::Stat>(key);
        if (stat) {
            stat->value++;
            storage.update(*stat);
        } else {
            db::Stat new_stat{key, 1};
            storage.replace(new_stat);
        }
    } catch (const std::exception& e) {
        printf("Error incrementing stat '%s': %s\n", key.c_str(), e.what());
    }
}

string exec(const char* cmd, int& exit_code) {
    exit_code = 0;
    char buffer[128];
    string result = "";
    shared_ptr<FILE> pipe(popen(cmd, "r"), [&](FILE* p) {
        if (p) {
            int status = pclose(p);
            if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
        }
    });
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL) result += buffer;
    }
    return result;
}

string find_first_file(const string& dir_path) {
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) return "";
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) return entry.path().string();
    }
    return "";
}


// --- Main Application ---

int main() {
    char* token_env = getenv("BOT_TOKEN");
    if (token_env == nullptr) { printf("BOT_TOKEN environment variable not set.\n"); return 1; }
    string token(token_env);

    char* admin_id_env = getenv("ADMIN_USER_ID");
    if (admin_id_env == nullptr) { printf("ADMIN_USER_ID environment variable not set.\n"); return 1; }
    ADMIN_USER_ID = stoll(admin_id_env);

    auto storage = db::init_storage("bot.db");
    printf("Database bot.db synced successfully.\n");

    // Load initial maintenance mode state
    MAINTENANCE_MODE = (get_setting(storage, "maintenance_mode", "off") == "on");
    printf("Maintenance mode is initially %s\n", MAINTENANCE_MODE ? "ON" : "OFF");

    Bot bot(token);

    // --- ADMIN COMMANDS ---
    bot.getEvents().onCommand("admin", [&bot](Message::Ptr message) {
        if (!is_admin(message->from)) return;
        InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
        vector<InlineKeyboardButton::Ptr> row1, row2;
        InlineKeyboardButton::Ptr stats_btn(new InlineKeyboardButton), users_btn(new InlineKeyboardButton);
        stats_btn->text = "📊 Statistika"; stats_btn->callbackData = "admin_stats";
        users_btn->text = "👥 Foydalanuvchilar"; users_btn->callbackData = "admin_users";
        row1.push_back(stats_btn); row1.push_back(users_btn);
        InlineKeyboardButton::Ptr broadcast_btn(new InlineKeyboardButton), maintenance_btn(new InlineKeyboardButton);
        broadcast_btn->text = "📢 Ommaviy xabar"; broadcast_btn->callbackData = "admin_broadcast";
        maintenance_btn->text = "⚙️ Texnik rejim"; maintenance_btn->callbackData = "admin_maintenance";
        row2.push_back(broadcast_btn); row2.push_back(maintenance_btn);
        keyboard->inlineKeyboard.push_back(row1); keyboard->inlineKeyboard.push_back(row2);
        bot.getApi().sendMessage(message->chat->id, "Salom, Admin! Boshqaruv paneliga xush kelibsiz.", nullptr, nullptr, keyboard);
    });

    auto user_mod_command = [&](Message::Ptr message, bool ban) {
        if (!is_admin(message->from)) return;
        stringstream ss(message->text);
        string command;
        long long user_id;
        ss >> command >> user_id;
        if (ss.fail()) { bot.getApi().sendMessage(message->chat->id, "Noto'g'ri format. Misol: " + command + " 123456"); return; }
        try {
            auto user = storage.get_pointer<db::User>(user_id);
            if (!user) { bot.getApi().sendMessage(message->chat->id, "Foydalanuvchi topilmadi."); return; }
            user->is_banned = ban;
            storage.update(*user);
            bot.getApi().sendMessage(message->chat->id, "Foydalanuvchi " + to_string(user_id) + (ban ? " bloklandi." : " blokdan chiqarildi."));
        } catch (const exception& e) { bot.getApi().sendMessage(message->chat->id, "Xatolik: " + string(e.what())); }
    };

    bot.getEvents().onCommand("ban", [&](Message::Ptr message){ user_mod_command(message, true); });
    bot.getEvents().onCommand("unban", [&](Message::Ptr message){ user_mod_command(message, false); });

    bot.getEvents().onCommand("broadcast", [&bot, &storage](Message::Ptr message) {
        if (!is_admin(message->from)) return;
        const string& text = message->text;
        string broadcast_msg = text.substr(text.find(' ') + 1);
        if (broadcast_msg.empty() || broadcast_msg == "/broadcast") {
            bot.getApi().sendMessage(message->chat->id, "Xabar matnini kiriting. Misol: /broadcast Salom!");
            return;
        }
        thread([&bot, &storage, broadcast_msg]() {
            try {
                auto users = storage.get_all<db::User>(where(c(&db::User::is_banned) == false));
                bot.getApi().sendMessage(ADMIN_USER_ID, to_string(users.size()) + " ta foydalanuvchiga xabar yuborish boshlandi...");
                int success_count = 0;
                for (const auto& user : users) {
                    try {
                        bot.getApi().sendMessage(user.id, broadcast_msg);
                        success_count++;
                        this_thread::sleep_for(chrono::milliseconds(100));
                    } catch (...) {}
                }
                bot.getApi().sendMessage(ADMIN_USER_ID, to_string(success_count) + " ta foydalanuvchiga xabar muvaffaqiyatli yuborildi.");
            } catch (const exception& e) {
                bot.getApi().sendMessage(ADMIN_USER_ID, "Broadcast xatosi: " + string(e.what()));
            }
        }).detach();
    });

    bot.getEvents().onCommand("maintenance", [&bot, &storage](Message::Ptr message){ handle_maintenance_command(bot, message, storage); });

    // --- USER-FACING HANDLERS ---
    bot.getEvents().onAnyMessage([&bot, &storage](Message::Ptr message) {
        upsert_user(storage, message->from);
        if (is_admin(message->from)) { // Admins can bypass some checks
             if (message->text.empty() || StringTools::startsWith(message->text, "/")) return;
        } else {
            if (MAINTENANCE_MODE) {
                bot.getApi().sendMessage(message->chat->id, "⚠️ Botda texnik ishlar olib borilmoqda. Iltimos, keyinroq urinib ko'ring.");
                return;
            }
            if (is_banned(storage, message->from->id) || message->text.empty() || StringTools::startsWith(message->text, "/")) {
                return;
            }
        }

        regex youtube_regex(R"((?:https?:\/\/)?(?:www\.)?(?:youtube\.com|youtu\.be)\/(?:watch\?v=)?([a-zA-Z0-9_-]{11}))");
        smatch match;
        if (regex_search(message->text, match, youtube_regex)) {
             string video_id = match[1];
            InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
            vector<InlineKeyboardButton::Ptr> row;
            InlineKeyboardButton::Ptr video_btn(new InlineKeyboardButton), audio_btn(new InlineKeyboardButton);
            video_btn->text = "Video"; video_btn->callbackData = "video:" + video_id;
            audio_btn->text = "Audio"; audio_btn->callbackData = "audio:" + video_id;
            row.push_back(video_btn); row.push_back(audio_btn);
            keyboard->inlineKeyboard.push_back(row);
            bot.getApi().sendMessage(message->chat->id, "Qaysi formatda yuklab olmoqchisiz?", nullptr, nullptr, keyboard);
        } else {
            bot.getApi().sendMessage(message->chat->id, "Iltimos, yaroqli YouTube havolasini yuboring.");
        }
    });

    bot.getEvents().onCallbackQuery([&bot, &storage](CallbackQuery::Ptr query) {
        upsert_user(storage, query->from);
        if (is_admin(query->from)) {
            handle_admin_callback(bot, query, storage);
            return;
        }

        if (MAINTENANCE_MODE) {
            bot.getApi().answerCallbackQuery(query->id, "Botda texnik ishlar olib borilmoqda.", true);
            return;
        }
        if (is_banned(storage, query->from->id)) { bot.getApi().answerCallbackQuery(query->id); return; }

        string data = query->data;
        size_t colon_pos = data.find(':');
        if (colon_pos == string::npos) return;
        string format = data.substr(0, colon_pos);
        string video_id = data.substr(colon_pos + 1);
        string url = "https://www.youtube.com/watch?v=" + video_id;
        bot.getApi().answerCallbackQuery(query->id, "Yuklab olinmoqda...");
        Message::Ptr status_message = bot.getApi().sendMessage(query->message->chat->id, "Yuklab olish boshlanmoqda... Iltimos, kuting.");
        try {
            const string download_dir = "downloads";
            fs::create_directory(download_dir);
            fs::permissions(download_dir, fs::perms::all);
            for(const auto& file : fs::directory_iterator(download_dir)) fs::remove_all(file.path());
            string command;
            string output_template = download_dir + "/%(title)s.%(ext)s";
            if (format == "video") {
                command = "yt-dlp -o '" + output_template + "' -f 'bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]/best' " + url;
            } else {
                command = "yt-dlp -o '" + output_template + "' -x --audio-format mp3 " + url;
            }
            int exit_code;
            string exec_output = exec(command.c_str(), exit_code);
            if (exit_code != 0) {
                 bot.getApi().editMessageText("Yuklab olishda xatolik yuz berdi. `yt-dlp` xatosi.", status_message->chat->id, status_message->messageId);
                 printf("yt-dlp error output:\n%s\n", exec_output.c_str());
                 return;
            }
            string file_path = find_first_file(download_dir);
            if (!file_path.empty()) {
                bot.getApi().editMessageText("Fayl yuborilmoqda...", status_message->chat->id, status_message->messageId);
                try {
                    if (format == "video") bot.getApi().sendVideo(query->message->chat->id, InputFile::fromFile(file_path, "video/mp4"));
                    else bot.getApi().sendAudio(query->message->chat->id, InputFile::fromFile(file_path, "audio/mpeg"));
                    bot.getApi().deleteMessage(status_message->chat->id, status_message->messageId);
                    increment_stat(storage, "downloads_count");
                } catch (const exception& send_e) {
                     bot.getApi().editMessageText("Faylni yuborishda xatolik: " + string(send_e.what()), status_message->chat->id, status_message->messageId);
                }
                fs::remove(file_path);
            } else {
                bot.getApi().editMessageText("Yuklab olingan faylni topa olmadim.", status_message->chat->id, status_message->messageId);
            }
        } catch (const exception& e) {
            bot.getApi().editMessageText("Umumiy xatolik: " + string(e.what()), status_message->chat->id, status_message->messageId);
        }
    });

    signal(SIGINT, [](int s) { printf("Signal %d received, exiting...\n", s); exit(0); });
    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();
        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (exception& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}

// --- Admin Function Implementations ---

void handle_maintenance_command(const Bot& bot, const Message::Ptr& message, db::Storage& storage) {
    if (!is_admin(message->from)) return;
    stringstream ss(message->text);
    string command, mode;
    ss >> command >> mode;
    if (mode == "on" || mode == "off") {
        MAINTENANCE_MODE = (mode == "on");
        set_setting(storage, "maintenance_mode", mode);
        bot.getApi().sendMessage(message->chat->id, "Texnik rejim " + mode + " holatiga o'tkazildi.");
    } else {
        bot.getApi().sendMessage(message->chat->id, "Noto'g'ri format. Misol: /maintenance on|off");
    }
}

void handle_admin_callback(const Bot& bot, const CallbackQuery::Ptr& query, db::Storage& storage) {
    bot.getApi().answerCallbackQuery(query->id);
    const string& data = query->data;

    if (data == "admin_users") {
        try {
            auto users = storage.get_all<db::User>();
            stringstream ss;
            ss << "Foydalanuvchilar Ro'yxati (" << users.size() << " ta):\n\n";
            for (const auto& user : users) {
                ss << "ID: " << user.id << "\n" << "Ism: " << user.first_name << "\n" << "Status: " << (user.is_banned ? "Bloklangan" : "Faol") << "\n\n";
            }
            bot.getApi().sendMessage(query->message->chat->id, ss.str());
        } catch (const exception& e) { bot.getApi().sendMessage(query->message->chat->id, "Xatolik: " + string(e.what())); }
    } else if (data == "admin_stats") {
        try {
            auto total_users = storage.count<db::User>();
            auto banned_users = storage.count<db::User>(where(c(&db::User::is_banned) == true));
            auto downloads = storage.get_pointer<db::Stat>("downloads_count");
            stringstream ss;
            ss << "📊 Bot Statistikasi\n\n"
               << "Jami foydalanuvchilar: " << total_users << "\n"
               << "Bloklanganlar: " << banned_users << "\n"
               << "Muvaffaqiyatli yuklashlar: " << (downloads ? downloads->value : 0) << "\n";
            bot.getApi().sendMessage(query->message->chat->id, ss.str());
        } catch (const exception& e) { bot.getApi().sendMessage(query->message->chat->id, "Xatolik: " + string(e.what())); }
    } else if (data == "admin_broadcast") {
        bot.getApi().sendMessage(query->message->chat->id, "Barcha foydalanuvchilarga yuborish uchun xabar matnini `/broadcast <xabar>` buyrug'i orqali yuboring.");
    } else if (data == "admin_maintenance") {
        string current_status = MAINTENANCE_MODE ? "YOQILGAN" : "O'CHIRILGAN";
        bot.getApi().sendMessage(query->message->chat->id, "Texnik rejim hozir " + current_status + ".\n\nO'zgartirish uchun `/maintenance on` yoki `/maintenance off` buyruqlaridan foydalaning.");
    }
}
