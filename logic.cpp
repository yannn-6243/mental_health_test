#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
using namespace std; 

// Logika Reverse Scoring: Q5 dan Q9 adalah reverse score.
const vector<bool> REVERSE_FLAGS = {
    false, false, false, false, true, 
    false, false, false, true, false
};

int hitung_skor(const vector<int>& jawaban) {
    if (jawaban.size() != REVERSE_FLAGS.size()) {
        return -1; 
    }

    int total = 0;
    for (size_t i = 0; i < jawaban.size(); ++i) {
        int v = jawaban[i];
        if (v < 0 || v > 3) return -2; // Error pada nilai input
        
        // Logika Reverse Scoring: jika TRUE, skor = 3 - nilai_jawaban
        int score = REVERSE_FLAGS[i] ? (3 - v) : v;
        total += score;
    }
    return total;
}

int main(int argc, char* argv[]) {
    // Memeriksa jumlah argumen
    if (argc != 11) { 
        cerr << "C++ ERROR: Harus menerima 10 jawaban sebagai argumen. Diterima: " << (argc - 1) << endl;
        return 1;
    }

    vector<int> jawaban;
    for (int i = 1; i < argc; ++i) {
        try {
            jawaban.push_back(stoi(argv[i]));
        } catch (const exception& e) {
            cerr << "C++ ERROR: Argumen ke-" << i << " (" << argv[i] << ") bukan angka valid." << endl;
            return 1;
        }
    }

    int total_skor = hitung_skor(jawaban);
    
    if (total_skor == -2) {
        cerr << "C++ ERROR: Nilai jawaban di luar rentang 0-3." << endl;
        return 1;
    }

    cout << total_skor << endl; 
    return 0; 
}