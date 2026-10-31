#include "MyanmarRuns.h"
#include <cstddef>

namespace myanmar_text {
bool isMyanmar(uint32_t cp) {
  return (cp >= 0x1000 && cp <= 0x109F) || (cp >= 0xAA60 && cp <= 0xAA7F) ||
         (cp >= 0xA9E0 && cp <= 0xA9FF);
}
static uint32_t readCp(const std::string& s, size_t& i) {
  const unsigned char c = static_cast<unsigned char>(s[i++]);
  if (c < 0x80) return c;
  const int extra = (c & 0xE0) == 0xC0 ? 1 : ((c & 0xF0) == 0xE0 ? 2 : ((c & 0xF8) == 0xF0 ? 3 : 0));
  if (!extra || i + extra > s.size()) return 0xFFFD;
  uint32_t cp = c & ((1u << (6 - extra)) - 1);
  for (int j = 0; j < extra; ++j) {
    const unsigned char d = static_cast<unsigned char>(s[i]);
    if ((d & 0xC0) != 0x80) return 0xFFFD;
    ++i; cp = (cp << 6) | (d & 0x3F);
  }
  return cp;
}
std::vector<Run> splitRuns(const std::string& s) {
  std::vector<Run> runs;
  size_t i = 0;
  while (i < s.size()) {
    size_t start = i;
    const uint32_t cp = readCp(s, i);
    const bool script = isMyanmar(cp);
    // ASCII spaces and punctuation inherit previous script run to avoid
    // breaking a run between a base character and neutral punctuation.
    const bool neutral = cp <= 0x20 || (cp < 0x80 && !((cp >= 'a' && cp <= 'z') ||
                                                         (cp >= 'A' && cp <= 'Z') ||
                                                         (cp >= '0' && cp <= '9')));
    const bool kind = neutral && !runs.empty() ? runs.back().myanmar : script;
    if (runs.empty() || runs.back().myanmar != kind) runs.push_back({"", kind});
    runs.back().utf8.append(s, start, i - start);
  }
  return runs;
}
}
