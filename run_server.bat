@echo off
REM Ganti jalur di bawah ini dengan lokasi folder proyek Anda yang sebenarnya
cd /d "D:\prg mental health"

echo.
echo --- 1. Mengompilasi Logika C++ (logic.cpp ke logic.exe) ---
g++ logic.cpp -o logic.exe

REM Cek apakah kompilasi berhasil (exit code 0)
if errorlevel 1 goto :error_compile

echo --- 2. Menjalankan Server Python Flask ---
python app.py

REM Cek apakah Python berhasil dijalankan (jika tidak dihentikan oleh pengguna)
if errorlevel 1 goto :error_python

goto :end

:error_compile
echo.
echo ====================================================================
echo ERROR: Kompilasi C++ GAGAL!
echo Pastikan g++ (MinGW) sudah terinstal dan ditambahkan ke PATH.
echo Periksa syntax di logic.cpp.
echo ====================================================================
pause
goto :end

:error_python
echo.
echo ====================================================================
echo ERROR: Server Python GAGAL dijalankan.
echo Pastikan semua library Flask (flask, flask_cors, dsb) sudah terinstal.
echo ====================================================================
pause
goto :end

:end