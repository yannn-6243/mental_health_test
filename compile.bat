@echo off
setlocal
echo 🔧 Mengaktifkan environment Emscripten...
cd /d D:\mental-health-test\emsdk
call emsdk_env.bat

echo 🔨 Compile scorer.cpp -> scorer.js / scorer.wasm...
cd /d D:\mental-health-test
emcc scorer.cpp -O3 -o scorer.js -s WASM=1 -s EXPORTED_FUNCTIONS="['_classify']" -s EXPORTED_RUNTIME_METHODS="['cwrap']" -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1

if %errorlevel% neq 0 (
  echo ❌ Compile gagal. Cek setup Emscripten dan path file.
) else (
  echo ✅ Compile selesai. File scorer.js dan scorer.wasm dibuat.
)
pause
endlocal
