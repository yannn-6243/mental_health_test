# Mental Health Test - C++ Backend

Backend API menggunakan C++ dengan Crow framework dan SQLite.

## API Endpoints

| Method | Endpoint | Deskripsi |
|--------|----------|-----------|
| GET | `/` | Health check |
| GET | `/api/health` | Status API |
| POST | `/api/submit` | Simpan hasil tes |
| GET | `/api/history` | Ambil semua riwayat |
| DELETE | `/api/history` | Hapus semua riwayat |
| DELETE | `/api/history/:id` | Hapus riwayat by ID |

## Request/Response Examples

### POST /api/submit
```json
// Request
{
  "name": "John",
  "score": 25,
  "max_score": 60,
  "category": "Perlu Perhatian Ringan",
  "note": "Merasa sedikit cemas"
}

// Response
{
  "success": true,
  "id": 1,
  "timestamp": "02/12/2025, 14:30:00"
}
```

### GET /api/history
```json
{
  "data": [
    {
      "id": 1,
      "timestamp": "02/12/2025, 14:30:00",
      "name": "John",
      "score": 25,
      "max_score": 60,
      "category": "Perlu Perhatian Ringan",
      "note": "Merasa sedikit cemas"
    }
  ]
}
```

## Deploy ke Railway

1. Push folder `backend` ke GitHub repository
2. Buka [railway.app](https://railway.app)
3. New Project → Deploy from GitHub repo
4. Pilih folder `backend`
5. Railway akan otomatis detect Dockerfile dan build
6. Setelah deploy, copy URL yang diberikan Railway
7. Update `API_BASE_URL` di frontend

## Local Development

### Prerequisites
- g++ (C++17 support)
- libsqlite3-dev
- libasio-dev atau libboost-dev

### Build & Run
```bash
# Download Crow header
wget https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow-v1.0+5.tar.gz
tar -xzf crow-v1.0+5.tar.gz
mv crow-v1.0+5/include/crow_all.h .

# Compile
g++ -std=c++17 -O2 -o server main.cpp -lsqlite3 -lpthread -DCROW_ENABLE_CORS

# Run
./server
```

Server akan berjalan di `http://localhost:8080`
