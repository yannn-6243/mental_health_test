# app.py
import os
import subprocess
import logging
from datetime import datetime
from io import BytesIO
from flask import Flask, request, jsonify, send_file
from flask_cors import CORS 
from flask_sqlalchemy import SQLAlchemy 
from sqlalchemy.exc import SQLAlchemyError 

# Konfigurasi Logging
logging.basicConfig(level=logging.INFO)

app = Flask(__name__)

# --- KONFIGURASI DATABASE ---
db_path = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'mental_health_history.db')
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///{db_path}'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
CORS(app) 

# --- MODEL DATA SQLAlchemy ---
class TesResult(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    name = db.Column(db.String(100), nullable=True)
    note = db.Column(db.String(255), nullable=True)
    total_score = db.Column(db.Integer, nullable=False)
    category = db.Column(db.String(50), nullable=False)
    
    def __repr__(self):
        return f'<TesResult {self.id} Score: {self.total_score}>'


# --- LOGIKA KLASIFIKASI ---
def classify(total):
    """Menentukan kategori, saran, dan warna berdasarkan skor total (0-30)."""
    if total <= 9:
        return {'cat': 'Baik', 'advice': 'Pertahankan pola hidup sehat...', 'color': '#16a34a'}
    elif total <= 19:
        return {'cat': 'Perlu Perhatian Ringan', 'advice': 'Coba atur jadwal, kurangi begadang...', 'color': '#f59e0b'}
    else: 
        return {'cat': 'Disarankan Konsultasi', 'advice': 'Pertimbangkan konsultasi...', 'color': '#ef4444'}


# --- ROUTE API 1: Menyimpan Hasil Tes ---
@app.route('/api/save', methods=['POST'])
def calculate_and_save():
    data = request.json
    answers = data.get('answers', [])
    
    if len(answers) != 10:
        return jsonify({"error": "Harus ada 10 jawaban."}), 400

    # 1. Persiapan Eksekusi C++
    cpp_executable = os.path.abspath("./logic.exe") 
    command = [cpp_executable] + [str(a) for a in answers]
    
    total_score = 0
    classification = {}

    try:
        # 2. Menjalankan C++
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        
        # Cek output C++
        total_score = int(result.stdout.strip())
        classification = classify(total_score)
        
    except subprocess.CalledProcessError as e:
        error_msg = e.stderr.strip() or "Kesalahan tidak diketahui dari logika C++."
        app.logger.error(f"C++ Execution Error: {error_msg}")
        return jsonify({"error": f"Kesalahan saat menjalankan logika C++: {error_msg}"}), 500
        
    except ValueError:
        app.logger.error(f"C++ output invalid: '{result.stdout.strip()}'")
        return jsonify({"error": "C++ mengembalikan output yang tidak valid."}), 500

    
    # 3. Penyimpanan ke Database (Jika C++ Sukses)
    try:
        new_result = TesResult(
            name=data.get('name', '').strip(),
            note=data.get('note', '').strip(),
            total_score=total_score,
            category=classification['cat']
        )
        db.session.add(new_result)
        db.session.commit()
        
    except SQLAlchemyError as e:
        app.logger.error(f"DATABASE ERROR: Gagal commit ke SQLite: {e}")
        db.session.rollback()
        return jsonify({"error": "Gagal menyimpan data ke database (SQLAlchemy Error)."}), 500
        
    except Exception as e:
        app.logger.error(f"Server Internal Error: {e}")
        db.session.rollback()
        return jsonify({"error": "Kesalahan server internal tidak terduga."}), 500


    # 4. Mengirimkan hasil kembali ke frontend (Jika C++ dan DB Sukses)
    return jsonify({
        "status": "success",
        "total_score": total_score,
        "category": classification['cat'],
        "advice": classification['advice'],
        "color": classification['color']
    })


# --- ROUTE API 2: Mengambil Riwayat Tes dari Database (GET) ---
@app.route('/api/history', methods=['GET'])
def get_history():
    results = TesResult.query.order_by(TesResult.timestamp.desc()).all()
    
    history_list = []
    for r in results:
        history_list.append({
            'timestamp': r.timestamp.strftime('%Y-%m-%d %H:%M:%S'),
            'name': r.name,
            'total': r.total_score,
            'category': r.category,
            'note': r.note
        })
        
    return jsonify(history_list)


# --- ROUTE API 3: Hapus Semua Riwayat (DELETE) ---
@app.route('/api/clear_history', methods=['DELETE'])
def clear_history():
    try:
        num_deleted = db.session.query(TesResult).delete()
        db.session.commit()
        app.logger.info(f"Deleted {num_deleted} records from database.")
        return jsonify({"status": "success", "message": f"{num_deleted} riwayat berhasil dihapus."}), 200
    except SQLAlchemyError as e:
        db.session.rollback()
        app.logger.error(f"DATABASE ERROR: Gagal menghapus riwayat: {e}")
        return jsonify({"status": "error", "error": "Gagal menghapus data dari database."}), 500


# --- ROUTE API 4: Ekspor Data ke CSV (GET) ---
@app.route('/api/export_csv', methods=['GET'])
def export_csv():
    results = TesResult.query.order_by(TesResult.timestamp.asc()).all()
    
    csv_data = ['"timestamp","name","total","category","note"']
    
    def clean_csv_field(s):
        if s is None:
            return ''
        return str(s).replace('"', '""').replace('\n', ' ')

    for r in results:
        row = [
            clean_csv_field(r.timestamp.strftime('%Y-%m-%d %H:%M:%S')),
            clean_csv_field(r.name),
            str(r.total_score),
            clean_csv_field(r.category),
            clean_csv_field(r.note)
        ]
        csv_data.append(f'"{row[0]}","{row[1]}",{row[2]},"{row[3]}","{row[4]}"')

    csv_output = "\n".join(csv_data)
    
    buffer = BytesIO()
    buffer.write(csv_output.encode('utf-8'))
    buffer.seek(0)
    
    return send_file(
        buffer,
        mimetype='text/csv',
        as_attachment=True,
        download_name='mental_check_history.csv'
    )


# --- MAIN EXECUTION ---
if __name__ == '__main__':
    with app.app_context():
        db.create_all()
        
    app.run(host='0.0.0.0', port=8080, debug=True)