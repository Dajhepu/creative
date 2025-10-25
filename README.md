# C++ Telegram YouTube Downloader Bot

Bu C++ da yozilgan oddiy Telegram bot bo'lib, YouTube'dan video va audio yuklab olish imkonini beradi.

## Bog'liqliklar

Botni ishga tushirishdan oldin, tizimda quyidagi bog'liqliklar o'rnatilganligiga ishonch hosil qiling:

*   **C++ Compiler (g++)**
*   **CMake**
*   **Make**
*   **tgbot-cpp kutubxonasi va uning bog'liqliklari:**
    *   `libssl-dev`
    *   `libboost-system-dev`
    *   `libcurl4-openssl-dev`
    *   `zlib1g-dev`
    *   `libboost-log-dev`
*   **yt-dlp:** YouTube'dan yuklab olish uchun buyruq qatori vositasi.

## O'rnatish va Ishga Tushirish

1.  **Bog'liqliklarni o'rnatish (Ubuntu/Debian):**
    ```bash
    sudo apt-get update && sudo apt-get install -y g++ cmake make libssl-dev libboost-system-dev libcurl4-openssl-dev zlib1g-dev libboost-log-dev python3-pip
    pip3 install yt-dlp
    ```
    *Eslatma: `tgbot-cpp` kutubxonasi loyihani qurish jarayonida avtomatik ravishda o'rnatiladi.*

2.  **Loyiha manbasini klonlash:**
    ```bash
    git clone <repository_url>
    cd <repository_directory>
    ```

3.  **Loyiha qurish:**
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

4.  **Botni ishga tushirish:**
    Botni ishga tushirishdan oldin Telegram Bot Tokeningizni atrof-muhit o'zgaruvchisiga o'rnating. Tokenni [@BotFather](https://t.me/BotFather) dan olishingiz mumkin.

    ```bash
    export BOT_TOKEN="SIZNING_TELEGRAM_BOT_TOKENINGIZ"
    ./TgYoutubeBot
    ```

Bot ishga tushadi va Telegram'dan xabarlarni qabul qilishni boshlaydi.

## Qanday ishlaydi

1.  Botga YouTube havolasini yuboring.
2.  Bot sizga "Video" yoki "Audio" formatini tanlash uchun tugmalar yuboradi.
3.  Tanlovingizga qarab, bot faylni yuklab oladi va sizga yuboradi.
4.  Fayl yuborilgandan so'ng, u serverdan o'chiriladi.
