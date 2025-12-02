# ... (Baris-baris sebelumnya)
# JANGAN LUPA MENGUBAH NAMA FUNGSI DI logic.cpp DULU!
RUN emcc logic.cpp \
    -o scorer.js \
    -s EXPORTED_FUNCTIONS="['_hitung_skor_wasm', '_malloc', '_free']" \
    -s MODULARIZE=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" \
    -O3
# ...
# Salin file aplikasi Flask dan Frontend
COPY app.py /app/
COPY index.html /app/
COPY Procfile /app/

# Jalankan skrip setup database pertama kali (jika SQLite belum ada)
# Ini membuat tabel di database SQLite saat image dibuat
RUN python -c "from app import app, db; with app.app_context(): db.create_all()"

# Ekspos port dan tentukan command start
ENV PORT 8080
EXPOSE $PORT
CMD ["gunicorn", "app:app", "--bind", "0.0.0.0:8080"]