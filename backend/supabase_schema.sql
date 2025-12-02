-- Run this in Supabase SQL Editor to create the history table

CREATE TABLE IF NOT EXISTS history (
    id SERIAL PRIMARY KEY,
    timestamp TEXT NOT NULL,
    name TEXT,
    score INTEGER NOT NULL,
    max_score INTEGER NOT NULL,
    category TEXT NOT NULL,
    note TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Enable Row Level Security (optional, for public access)
ALTER TABLE history ENABLE ROW LEVEL SECURITY;

-- Allow all operations for anonymous users (for demo purposes)
CREATE POLICY "Allow all" ON history FOR ALL USING (true) WITH CHECK (true);
