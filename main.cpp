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
#include <functional>

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
#include "database.h"

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
int exec_with_progress(const string& command, const function<void(const string&)>& on_new_line);
string find_first_file(const string& dir_path);

// --- Utility & Database Functions ---

bool is_admin(const User::Ptr& user) { return user && user->id == ADMIN_USER_ID; }
bool is_banned(db::Storage& storage, long long user_id) {
    auto user = storage.get_pointer<db::User>(user_id);
    return user && user->is_banned;
}

void set_setting(db::Storage& storage, const string& key, const string& value) { storage.replace(db::Setting{key, value}); }
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
        if (auto db_user = storage.get_pointer<db::User>(user->id)) {
            if (db_user->first_name != user->firstName || db_user->username != user->username) {
                db_user->first_name = user->firstName;
                db_user->username = user->username;
                storage.update(*db_user);
            }
        } else {
            storage.insert(db::User{user->id, user->firstName, user->username, false, get_current_time_str()});
        }
    } catch (const std::exception& e) {
        printf("Error upserting user %ld: %s\n", (long)user->id, e.what());
    }
}

void increment_stat(db::Storage& storage, const std::string& key) {
    try {
        if (auto stat = storage.get_pointer<db::Stat>(key)) {
            stat->value++;
            storage.update(*stat);
        } else {
            storage.replace(db::Stat{key, 1});
        }
    } catch (const std::exception& e) {
        printf("Error incrementing stat '%s': %s\n", key.c_str(), e.what());
    }
}

int exec_with_progress(const string& command, const function<void(const string&)>& on_new_line) {
    shared_ptr<FILE> pipe(popen((command + " 2>&1").c_str(), "r"), pclose);
    if (!pipe) return -1;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        on_new_line(string(buffer));
    }
    int status = pclose(pipe.get());
    return WEXITSTATUS(status);
}

string find_first_file(const string& dir_path) {
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) return "";
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) return entry.path().string();
    }
    return "";
}

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

// --- Main Application ---
int main() {
    char* token_env = getenv("BOT_TOKEN");
    if (!token_env) { printf("BOT_TOKEN not set.\n"); return 1; }
    string token(token_env);

    char* admin_id_env = getenv("ADMIN_USER_ID");
    if (!admin_id_env) { printf("ADMIN_USER_ID not set.\n"); return 1; }
    ADMIN_USER_ID = stoll(admin_id_env);

    auto storage = db::init_storage("bot.db");
    storage.sync_schema();
    printf("Database synced.\n");

    MAINTENANCE_MODE = (get_setting(storage, "maintenance_mode", "off") == "on");
    printf("Maintenance mode: %s\n", MAINTENANCE_MODE ? "ON" : "OFF");

    Bot bot(token);

    auto handle_download = [&](long long chat_id, Message::Ptr status_message, const string& command, const string& final_media_type) {
        regex re_progress(R"(\[download\]\s+([0-9.]+)%)");
        int last_percent = -1;
        string full_output;

        int exit_code = exec_with_progress(command, [&](const string& line) {
            full_output += line;
            smatch match;
            if (regex_search(line, match, re_progress) && match.size() > 1) {
                try {
                    double percent_double = stod(match[1]);
                    int percent = static_cast<int>(percent_double);
                    if (percent > last_percent && (percent % 10 == 0 || percent == 100)) {
                        last_percent = percent;
                        bot.getApi().editMessageText("Yuklanmoqda... " + to_string(percent) + "%", status_message->chat->id, status_message->messageId);
                    }
                } catch (...) {}
            }
        });

        if (exit_code != 0) {
            bot.getApi().editMessageText("Xatolik: Yuklab bo'lmadi yoki natija topilmadi.", status_message->chat->id, status_message->messageId);
            printf("yt-dlp error: %s\n", full_output.c_str());
            return;
        }

        string file_path = find_first_file("downloads");
        if (!file_path.empty()) {
            bot.getApi().editMessageText(string(final_media_type == "audio" ? "🎧 Audio" : "🎬 Video") + " yuborilmoqda...", status_message->chat->id, status_message->messageId);
            try {
                if (final_media_type == "audio") bot.getApi().sendAudio(chat_id, InputFile::fromFile(file_path, "audio/mpeg"));
                else bot.getApi().sendVideo(chat_id, InputFile::fromFile(file_path, "video/mp4"));

                bot.getApi().deleteMessage(status_message->chat->id, status_message->messageId);
                increment_stat(storage, "downloads_count");
            } catch (const exception& e) {
                bot.getApi().editMessageText("Faylni yuborishda xatolik: " + string(e.what()), status_message->chat->id, status_message->messageId);
            }
            fs::remove(file_path);
        } else {
            bot.getApi().editMessageText("Xatolik: Yuklangan faylni topa olmadim.", status_message->chat->id, status_message->messageId);
        }
    };

    // --- ADMIN COMMANDS ---
    bot.getEvents().onCommand("admin", [&bot](Message::Ptr message) {
        if (!is_admin(message->from)) return;
        // ... (implementation unchanged)
    });

    // ... (other admin commands)

    // --- USER-FACING HANDLERS ---
    bot.getEvents().onCommand("start", [&bot, &storage](Message::Ptr message) {
        upsert_user(storage, message->from);
        if (is_banned(storage, message->from->id)) return;
        string text = "Assalomu alaykum! 👋\n\nMen YouTube'dan video va audio yuklashga yordam beraman.\n\n"
                      "Menga quyidagilardan birini yuboring:\n"
                      "1. **YouTube havolasi:** Video/audio formatini tanlash imkonini beradi.\n"
                      "2. **Qo'shiq nomi:** (masalan, 'Xcho - Vorovala') Avtomatik qidirib, audio yuboradi.\n\n"
                      "Marhamat, boshlang!";
        bot.getApi().sendMessage(message->chat->id, text, nullptr, nullptr, nullptr, "Markdown");
    });

    bot.getEvents().onAnyMessage([&](Message::Ptr message) {
        upsert_user(storage, message->from);
        if ((!is_admin(message->from) && (MAINTENANCE_MODE || is_banned(storage, message->from->id))) || message->text.empty() || StringTools::startsWith(message->text, "/")) {
            if (MAINTENANCE_MODE && !is_admin(message->from)) bot.getApi().sendMessage(message->chat->id, "⚠️ Texnik ishlar olib borilmoqda.");
            return;
        }

        thread([&, message]() {
            try {
                fs::create_directories("downloads");
                for(const auto& file : fs::directory_iterator("downloads")) fs::remove_all(file.path());

                regex youtube_regex(R"((?:https?:\/\/)?(?:www\.)?(?:youtube\.com|youtu\.be)\/(?:watch\?v=)?([a-zA-Z0-9_-]{11}))");
                smatch match;
                if (regex_search(message->text, match, youtube_regex)) {
                    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
                    vector<InlineKeyboardButton::Ptr> row;
                    InlineKeyboardButton::Ptr video_btn(new InlineKeyboardButton), audio_btn(new InlineKeyboardButton);
                    video_btn->text = "Video"; video_btn->callbackData = "video:" + match[1].str();
                    audio_btn->text = "Audio"; audio_btn->callbackData = "audio:" + match[1].str();
                    row.push_back(video_btn); row.push_back(audio_btn);
                    keyboard->inlineKeyboard.push_back(row);
                    bot.getApi().sendMessage(message->chat->id, "Qaysi formatda yuklab olmoqchisiz?", nullptr, nullptr, keyboard);
                } else {
                    Message::Ptr status_message = bot.getApi().sendMessage(message->chat->id, "⏳ Qidirilmoqda: " + message->text);
                    string query = message->text;
                    string::size_type pos = query.find_first_of(";'\"`|&"); if (pos != string::npos) query = query.substr(0, pos);
                    string command = "yt-dlp --progress -o 'downloads/%(title)s.%(ext)s' -x --audio-format mp3 \"ytsearch1:" + query + "\"";
                    handle_download(message->chat->id, status_message, command, "audio");
                }
            } catch (const exception& e) { printf("onAnyMessage error: %s\n", e.what()); }
        }).detach();
    });

    bot.getEvents().onCallbackQuery([&](CallbackQuery::Ptr query) {
        upsert_user(storage, query->from);
        if (is_admin(query->from)) { handle_admin_callback(bot, query, storage); return; }
        if (MAINTENANCE_MODE) { bot.getApi().answerCallbackQuery(query->id, "Botda texnik ishlar olib borilmoqda.", true); return; }
        if (is_banned(storage, query->from->id)) { bot.getApi().answerCallbackQuery(query->id); return; }

        string data = query->data;
        size_t colon_pos = data.find(':');
        if (colon_pos == string::npos) return;

        string format = data.substr(0, colon_pos);
        string video_id = data.substr(colon_pos + 1);
        string url = "https://www.youtube.com/watch?v=" + video_id;

        thread([&, query, format, url]() {
            bot.getApi().answerCallbackQuery(query->id);
            Message::Ptr status_message = bot.getApi().sendMessage(query->message->chat->id, "Yuklanish boshlanmoqda...");
            try {
                fs::create_directories("downloads");
                for(const auto& file : fs::directory_iterator("downloads")) fs::remove_all(file.path());
                string command;
                if (format == "video") {
                    command = "yt-dlp --progress -o 'downloads/%(title)s.%(ext)s' -f 'bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]/best' " + url;
                } else {
                    command = "yt-dlp --progress -o 'downloads/%(title)s.%(ext)s' -x --audio-format mp3 " + url;
                }
                handle_download(query->message->chat->id, status_message, command, format);
            } catch (const exception& e) {
                 bot.getApi().editMessageText("Umumiy xatolik: " + string(e.what()), status_message->chat->id, status_message->messageId);
            }
        }).detach();
    });

    signal(SIGINT, [](int s) { printf("Signal %d received, exiting...\n", s); exit(0); });
    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();
        TgLongPoll longPoll(bot);
        while (true) { printf("Long poll started\n"); longPoll.start(); }
    } catch (exception& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}
