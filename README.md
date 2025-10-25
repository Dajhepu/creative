# C++ Telegram YouTube Downloader Bot

Bu C++ da yozilgan oddiy Telegram bot bo'lib, YouTube'dan video va audio yuklab olish imkonini beradi.

## Bog'liqliklar

Botni mahalliy kompyuteringizda ishga tushirish uchun quyidagi bog'liqliklar kerak bo'ladi. Agar siz Railway.app'dan foydalansangiz, bu bog'liqliklar `Dockerfile` orqali avtomatik ravishda o'rnatiladi.

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

## Mahalliy O'rnatish va Ishga Tushirish

1.  **Bog'liqliklarni o'rnatish (Ubuntu/Debian):**
    ```bash
    sudo apt-get update && sudo apt-get install -y g++ cmake make libssl-dev libboost-system-dev libcurl4-openssl-dev zlib1g-dev libboost-log-dev python3-pip
    pip3 install yt-dlp
    ```

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

## Railway.app'ga Joylashtirish

Ushbu loyiha Railway.app platformasiga `Dockerfile` yordamida osonlik bilan joylashtirilishi mumkin.

1.  **GitHub Repozitoriysini Yaratish:**
    *   Ushbu loyiha fayllarini o'z GitHub hisobingizdagi yangi (public yoki private) repozitoriyga yuklang.

2.  **Railway.app'da Yangi Loyiha Yaratish:**
    *   Railway.app'ga kiring va yangi loyiha yarating ("New Project").
    *   "Deploy from GitHub repo" variantini tanlang va o'zingizning repozitoriyingizni ulang.

3.  **Atrof-muhit O'zgaruvchisini Sozlash:**
    *   Loyiha sozlamalarida ("Variables" bo'limida) `BOT_TOKEN` nomli yangi o'zgaruvchi yarating.
    *   Qiymat sifatida o'zingizning Telegram bot tokeningizni kiriting.

4.  **Joylashtirish:**
    *   Railway avtomatik ravishda repozitoriyingizdagi `Dockerfile` ni aniqlaydi, Docker tasvirini yaratadi va botni ishga tushiradi.
    *   Joylashtirish jurnallarini ("Deployments" bo'limida) kuzatib borishingiz mumkin.

Bot muvaffaqiyatli joylashtirilgandan so'ng, u Telegram'da ishlashni boshlaydi.

## Qanday ishlaydi

1.  Botga YouTube havolasini yuboring.
2.  Bot sizga "Video" yoki "Audio" formatini tanlash uchun tugmalar yuboradi.
3.  Tanlovingizga qarab, bot faylni yuklab oladi va sizga yuboradi.
4.  Fayl yuborilgandan so'ng, u serverdan o'chiriladi.
