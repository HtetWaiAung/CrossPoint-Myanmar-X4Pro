#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Standalone Myanmar OpenType shaping adapter. This adapter only shapes;
// integration with CrossPoint's glyph-ID drawing path is still required.
namespace myanmar_text {
struct PositionedGlyph {
  uint32_t glyphId;
  uint32_t cluster;
  int32_t xAdvance26_6;
  int32_t yAdvance26_6;
  int32_t xOffset26_6;
  int32_t yOffset26_6;
};

// Bytes are BORROWED; the TTF must stay resident while this object lives.
class MyanmarHarfBuzz {
 public:
  MyanmarHarfBuzz(const uint8_t* fontBytes, size_t length);
  // Stream source reads exactly len bytes from offset; returns false on I/O failure.
  using ReadAt = bool (*)(void* ctx, uint32_t offset, uint8_t* dest, uint32_t len);
  MyanmarHarfBuzz(ReadAt read, void* context, uint32_t fontLength);
  ~MyanmarHarfBuzz();
  MyanmarHarfBuzz(const MyanmarHarfBuzz&) = delete;
  MyanmarHarfBuzz& operator=(const MyanmarHarfBuzz&) = delete;
  bool ready() const;
  // Appends glyph IDs and 26.6 positioning scaled to the requested pixel size.
  bool shape(const char* utf8, uint16_t pixelSize, std::vector<PositionedGlyph>& result);
 private:
  struct State;
  State* state_;
};
}
