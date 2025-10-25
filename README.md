# C++ Telegram YouTube Downloader Bot

Bu C++ da yozilgan Telegram bot bo'lib, YouTube'dan video va audio yuklab olish imkonini beradi. Botda to'liq funksional admin paneli mavjud.

## Asosiy Xususiyatlar
- YouTube'dan video va audio yuklab olish.
- Foydalanuvchilarni boshqarish (ban/unban).
- Bot faoliyati statistikasi.
- Barcha foydalanuvchilarga ommaviy xabar yuborish (broadcast).
- Texnik xizmat ko'rsatish rejimi.

## Bog'liqliklar
Botni mahalliy kompyuteringizda ishga tushirish uchun quyidagi bog'liqliklar kerak bo'ladi. Agar siz Railway.app'dan foydalansangiz, bu bog'liqliklar `Dockerfile` orqali avtomatik ravishda o'rnatiladi.

*   **C++ Compiler (g++)**
*   **CMake**
*   **Make**
*   **tgbot-cpp kutubxonasi va uning bog'liqliklari:**
    *   `libssl-dev`, `libboost-system-dev`, `libcurl4-openssl-dev`, `zlib1g-dev`, `libboost-log-dev`
*   **SQLite3:** Ma'lumotlar bazasi uchun (`libsqlite3-dev`).
*   **yt-dlp:** YouTube'dan yuklab olish uchun (`python3-pip` orqali).

## Mahalliy O'rnatish va Ishga Tushirish
1.  **Bog'liqliklarni o'rnatish (Ubuntu/Debian):**
    ```bash
    sudo apt-get update && sudo apt-get install -y g++ cmake make libssl-dev libboost-system-dev libcurl4-openssl-dev zlib1g-dev libboost-log-dev libsqlite3-dev python3-pip
    pip3 install yt-dlp
    ```

2.  **Loyiha manbasini klonlash va submodullarni yuklash:**
    ```bash
    git clone --recurse-submodules <repository_url>
    cd <repository_directory>
    ```

3.  **Loyiha qurish:**
    ```bash
    mkdir build && cd build
    cmake ..
    make
    ```

4.  **Botni ishga tushirish:**
    Botni ishga tushirishdan oldin kerakli atrof-muhit o'zgaruvchilarini o'rnating.
    *   `BOT_TOKEN`: [@BotFather](https://t.me/BotFather) dan olingan bot tokeni.
    *   `ADMIN_USER_ID`: Sizning Telegram foydalanuvchi ID raqamingiz. Uni bilish uchun [@userinfobot](https://t.me/userinfobot) kabi botlardan foydalanishingiz mumkin.

    ```bash
    export BOT_TOKEN="SIZNING_TELEGRAM_BOT_TOKENINGIZ"
    export ADMIN_USER_ID="SIZNING_TELEGRAM_ID_RAQAMINGIZ"
    ./TgYoutubeBot
    ```

## Railway.app'ga Joylashtirish
1.  **GitHub Repozitoriysini Yaratish:**
    *   Ushbu loyiha fayllarini o'z GitHub hisobingizdagi yangi repozitoriyga yuklang.

2.  **Railway.app'da Yangi Loyiha Yaratish:**
    *   Railway.app'ga kiring, yangi loyiha yarating va o'zingizning GitHub repozitoriyingizni ulang.

3.  **Atrof-muhit O'zgaruvchilarini Sozlash:**
    *   Loyiha sozlamalarida ("Variables" bo'limida) quyidagi o'zgaruvchilarni yarating:
        *   `BOT_TOKEN`: Sizning Telegram bot tokeningiz.
        *   `ADMIN_USER_ID`: Sizning Telegram ID raqamingiz.

4.  **Joylashtirish:**
    *   Railway avtomatik ravishda `Dockerfile` ni aniqlaydi va botni ishga tushiradi.

## Admin Paneli Buyruqlari
Admin panelini ochish uchun botga `/admin` buyrug'ini yuboring. Barcha admin buyruqlari faqat `ADMIN_USER_ID` da ko'rsatilgan foydalanuvchi uchun ishlaydi.

*   `/admin`: Boshqaruv panelining asosiy menyusini ochadi.
*   `/ban <user_id>`: Foydalanuvchini bloklaydi.
*   `/unban <user_id>`: Foydalanuvchini blokdan chiqaradi.
*   `/broadcast <xabar>`: Barcha faol foydalanuvchilarga ommaviy xabar yuboradi.
*   `/maintenance on|off`: Botda texnik xizmat ko'rsatish rejimini yoqadi yoki o'chiradi.
