@echo off
REM Aktifkan environment Emscripten
cd /d D:\mental-health-test\emsdk
call emsdk_env.bat

echo.
echo ✅ Environment Emscripten sudah aktif. Sekarang kamu bisa pakai emcc di terminal ini.
pause
