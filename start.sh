#!/bin/sh

# Agar biror buyruq xatolik bilan yakunlansa, skriptni darhol to'xtatish
set -e

# yt-dlp ni eng so'nggi versiyaga yangilash
echo "Updating yt-dlp to the latest version..."
pip3 install --upgrade yt-dlp

# Asosiy bot dasturini ishga tushirish
echo "Update complete. Starting the bot..."
exec ./TgYoutubeBot
