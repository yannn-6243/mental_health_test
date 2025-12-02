#include <string>
#include <emscripten.h>

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* classify(int total, int maxScore) {
  static std::string result;
  if (total < 0 || total > maxScore) {
    result = "Error";
    return result.c_str();
  }
  // Thresholds: <=33% good, <=66% mild, else consult
  int t1 = (int)(maxScore * 0.33);
  int t2 = (int)(maxScore * 0.66);
  if (total <= t1) {
    result = "Baik";
  } else if (total <= t2) {
    result = "Perlu Perhatian Ringan";
  } else {
    result = "Disarankan Konsultasi";
  }
  return result.c_str();
}

}
