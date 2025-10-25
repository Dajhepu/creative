#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <regex>
#include <iostream>

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

using namespace std;
using namespace TgBot;

// Function to execute a shell command and return the output
string exec(const char* cmd, int& exit_code) {
    exit_code = 0;
    char buffer[128];
    string result = "";
    shared_ptr<FILE> pipe(popen(cmd, "r"), [&](FILE* p) {
        if (p) {
            int status = pclose(p);
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            }
        }
    });
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}

// Function to find the first file in a directory
string find_first_file(const string& dir_path) {
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        return "";
    }
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            return entry.path().string();
        }
    }
    return "";
}

int main() {
    char* token_env = getenv("BOT_TOKEN");
    if (token_env == nullptr) {
        printf("BOT_TOKEN environment variable not set.\n");
        return 1;
    }
    string token(token_env);

    Bot bot(token);

    bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Salom! Menga YouTube havolasini yuboring.");
    });

    bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
        if (message->text.empty() || StringTools::startsWith(message->text, "/start")) {
            return;
        }

        regex youtube_regex(R"((?:https?:\/\/)?(?:www\.)?(?:youtube\.com|youtu\.be)\/(?:watch\?v=)?([a-zA-Z0-9_-]{11}))");
        smatch match;
        if (regex_search(message->text, match, youtube_regex)) {
            string video_id = match[1];

            InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
            vector<InlineKeyboardButton::Ptr> row;

            InlineKeyboardButton::Ptr video_button(new InlineKeyboardButton);
            video_button->text = "Video";
            video_button->callbackData = "video:" + video_id;
            row.push_back(video_button);

            InlineKeyboardButton::Ptr audio_button(new InlineKeyboardButton);
            audio_button->text = "Audio";
            audio_button->callbackData = "audio:" + video_id;
            row.push_back(audio_button);

            keyboard->inlineKeyboard.push_back(row);

            bot.getApi().sendMessage(message->chat->id, "Qaysi formatda yuklab olmoqchisiz?", false, 0, keyboard);

        } else {
            bot.getApi().sendMessage(message->chat->id, "Iltimos, yaroqli YouTube havolasini yuboring.");
        }
    });

    bot.getEvents().onCallbackQuery([&bot](CallbackQuery::Ptr query) {
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
            fs::permissions(download_dir, fs::perms::all); // Set permissions to be safe
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
                    if (format == "video") {
                        bot.getApi().sendVideo(query->message->chat->id, InputFile::fromFile(file_path, "video/mp4"));
                    } else {
                        bot.getApi().sendAudio(query->message->chat->id, InputFile::fromFile(file_path, "audio/mpeg"));
                    }
                    bot.getApi().deleteMessage(status_message->chat->id, status_message->messageId);
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

    signal(SIGINT, [](int s) {
        printf("Signal %d received, exiting...\n", s);
        exit(0);
    });

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
