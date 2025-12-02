# Build C++ scorer
FROM ubuntu:22.04 AS cpp-builder

RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY backend/scorer.cpp .
RUN g++ -std=c++17 -O2 -o scorer scorer.cpp

# Python runtime
FROM python:3.11-slim

WORKDIR /app

# Copy compiled scorer from builder
COPY --from=cpp-builder /app/scorer ./backend/scorer

# Install Python dependencies
COPY backend/requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy Python backend
COPY backend/main.py ./backend/

EXPOSE 8080

CMD ["python", "backend/main.py"]
