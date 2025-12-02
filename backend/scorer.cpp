#include <iostream>
#include <cstdlib>
#include <string>

std::string classify(int total, int maxScore) {
    if (total < 0 || total > maxScore) {
        return "Error";
    }
    
    int t1 = (int)(maxScore * 0.33);
    int t2 = (int)(maxScore * 0.66);
    
    if (total <= t1) {
        return "Baik";
    } else if (total <= t2) {
        return "Perlu Perhatian Ringan";
    } else {
        return "Disarankan Konsultasi";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: scorer <total> <maxScore>" << std::endl;
        return 1;
    }
    
    int total = std::atoi(argv[1]);
    int maxScore = std::atoi(argv[2]);
    
    std::cout << classify(total, maxScore) << std::endl;
    return 0;
}
