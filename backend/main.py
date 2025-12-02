import os
import subprocess
from datetime import datetime
from typing import Optional
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from supabase import create_client, Client

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Supabase setup
SUPABASE_URL = os.getenv("SUPABASE_URL", "")
SUPABASE_KEY = os.getenv("SUPABASE_KEY", "")
supabase: Optional[Client] = None

if SUPABASE_URL and SUPABASE_KEY:
    supabase = create_client(SUPABASE_URL, SUPABASE_KEY)

# Path to compiled scorer
SCORER_PATH = os.path.join(os.path.dirname(__file__), "scorer")

class SubmitRequest(BaseModel):
    name: Optional[str] = ""
    score: int
    max_score: int
    category: str
    note: Optional[str] = ""

def classify_score(total: int, max_score: int) -> str:
    """Call C++ scorer executable"""
    try:
        result = subprocess.run(
            [SCORER_PATH, str(total), str(max_score)],
            capture_output=True,
            text=True,
            timeout=5
        )
        return result.stdout.strip()
    except Exception as e:
        # Fallback to Python implementation
        if total < 0 or total > max_score:
            return "Error"
        t1 = int(max_score * 0.33)
        t2 = int(max_score * 0.66)
        if total <= t1:
            return "Baik"
        elif total <= t2:
            return "Perlu Perhatian Ringan"
        else:
            return "Disarankan Konsultasi"

@app.get("/")
def root():
    return {"message": "Mental Health API is running!"}

@app.get("/api/health")
def health():
    return {"status": "ok", "message": "Backend Python is running"}

@app.get("/api/classify")
def classify(total: int, max_score: int):
    category = classify_score(total, max_score)
    return {"category": category}

@app.post("/api/submit")
def submit(data: SubmitRequest):
    if not supabase:
        raise HTTPException(status_code=500, detail="Database not configured")
    
    timestamp = datetime.now().strftime("%d/%m/%Y, %H:%M:%S")
    
    record = {
        "timestamp": timestamp,
        "name": data.name or "-",
        "score": data.score,
        "max_score": data.max_score,
        "category": data.category,
        "note": data.note or ""
    }
    
    result = supabase.table("history").insert(record).execute()
    
    if result.data:
        return {
            "success": True,
            "id": result.data[0].get("id"),
            "timestamp": timestamp
        }
    raise HTTPException(status_code=500, detail="Failed to save data")

@app.get("/api/history")
def get_history():
    if not supabase:
        raise HTTPException(status_code=500, detail="Database not configured")
    
    result = supabase.table("history").select("*").order("id", desc=True).execute()
    return {"data": result.data or []}

@app.delete("/api/history")
def delete_history():
    if not supabase:
        raise HTTPException(status_code=500, detail="Database not configured")
    
    supabase.table("history").delete().neq("id", 0).execute()
    return {"success": True, "message": "All history deleted"}

if __name__ == "__main__":
    import uvicorn
    port = int(os.getenv("PORT", 8080))
    uvicorn.run(app, host="0.0.0.0", port=port)
