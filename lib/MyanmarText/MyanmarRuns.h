#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Unicode-script span detection for the future HarfBuzz TTF shaping path.
// This utility does NOT itself shape glyphs. Keep normal punctuation attached
// to its preceding run; do not change scripts mid-combining sequence.
namespace myanmar_text {
struct Run { std::string utf8; bool myanmar; };
bool isMyanmar(uint32_t cp);
std::vector<Run> splitRuns(const std::string& utf8);
}
