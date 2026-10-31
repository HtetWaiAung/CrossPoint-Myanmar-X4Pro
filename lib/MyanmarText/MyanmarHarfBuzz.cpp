#include "MyanmarHarfBuzz.h"
#include "../HarfBuzzTiny/src/hb-ot.h"
#include <new>

namespace myanmar_text {
struct MyanmarHarfBuzz::State {
  hb_blob_t* blob = nullptr;
  hb_face_t* face = nullptr;
  hb_font_t* font = nullptr;
  hb_buffer_t* buffer = nullptr;
};
MyanmarHarfBuzz::MyanmarHarfBuzz(const uint8_t* bytes, size_t length) : state_(nullptr) {
  if (!bytes || !length || length > UINT32_MAX) return;
  State* s = new State();
  s->blob = hb_blob_create(reinterpret_cast<const char*>(bytes), static_cast<unsigned>(length),
                           HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  s->face = hb_face_create(s->blob, 0);
  s->font = hb_font_create(s->face);
  hb_ot_font_set_funcs(s->font);
  s->buffer = hb_buffer_create();
  state_ = s;
}
namespace {
uint16_t be16(const uint8_t* p) { return (uint16_t(p[0]) << 8) | p[1]; }
uint32_t be32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
struct StreamState {
  MyanmarHarfBuzz::ReadAt read;
  void* context;
  uint32_t length;
};
void destroyBytes(void* p) { delete[] static_cast<uint8_t*>(p); }
hb_blob_t* streamTable(hb_face_t*, hb_tag_t tag, void* opaque) {
  auto* stream = static_cast<StreamState*>(opaque);
  if (tag == HB_TAG_NONE || stream->length < 12) return nullptr;
  uint8_t header[12];
  if (!stream->read(stream->context, 0, header, sizeof(header))) return nullptr;
  const uint32_t count = be16(header + 4);
  if (count > 4096 || 12u + count * 16u > stream->length) return nullptr;
  for (uint32_t i = 0; i < count; ++i) {
    uint8_t entry[16];
    if (!stream->read(stream->context, 12 + i * 16, entry, sizeof(entry))) return nullptr;
    if (be32(entry) != tag) continue;
    const uint32_t offset = be32(entry + 8), bytes = be32(entry + 12);
    if (offset > stream->length || bytes > stream->length - offset || bytes > 8u * 1024u * 1024u) return nullptr;
    auto* data = new (std::nothrow) uint8_t[bytes ? bytes : 1];
    if (!data) return nullptr;
    if (bytes && !stream->read(stream->context, offset, data, bytes)) { delete[] data; return nullptr; }
    return hb_blob_create(reinterpret_cast<const char*>(data), bytes, HB_MEMORY_MODE_READONLY, data, destroyBytes);
  }
  return nullptr;
}
void destroyStream(void* p) { delete static_cast<StreamState*>(p); }
}  // namespace
MyanmarHarfBuzz::MyanmarHarfBuzz(ReadAt read, void* context, uint32_t fontLength) : state_(nullptr) {
  if (!read || fontLength < 12) return;
  auto* stream = new (std::nothrow) StreamState{read, context, fontLength};
  if (!stream) return;
  State* s = new (std::nothrow) State();
  if (!s) { delete stream; return; }
  s->face = hb_face_create_for_tables(streamTable, stream, destroyStream);
  s->font = hb_font_create(s->face);
  hb_ot_font_set_funcs(s->font);
  s->buffer = hb_buffer_create();
  state_ = s;
}
MyanmarHarfBuzz::~MyanmarHarfBuzz() {
  if (!state_) return;
  if (state_->buffer) hb_buffer_destroy(state_->buffer);
  if (state_->font) hb_font_destroy(state_->font);
  if (state_->face) hb_face_destroy(state_->face);
  if (state_->blob) hb_blob_destroy(state_->blob);
  delete state_;
}
bool MyanmarHarfBuzz::ready() const {
  return state_ && state_->face && state_->font && state_->buffer &&
         hb_face_get_glyph_count(state_->face) > 0;
}
bool MyanmarHarfBuzz::shape(const char* utf8, uint16_t pixelSize,
                            std::vector<PositionedGlyph>& result) {
  result.clear();
  if (!ready() || !utf8 || !*utf8 || !pixelSize) return false;
  hb_buffer_reset(state_->buffer);
  hb_buffer_add_utf8(state_->buffer, utf8, -1, 0, -1);
  hb_buffer_set_direction(state_->buffer, HB_DIRECTION_LTR);
  hb_buffer_set_script(state_->buffer, HB_SCRIPT_MYANMAR);
  hb_buffer_set_language(state_->buffer, hb_language_from_string("my", -1));
  const int scale = static_cast<int>(pixelSize) * 64;
  hb_font_set_scale(state_->font, scale, scale);
  hb_shape(state_->font, state_->buffer, nullptr, 0);
  unsigned int n = 0;
  const hb_glyph_info_t* glyphs = hb_buffer_get_glyph_infos(state_->buffer, &n);
  const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(state_->buffer, &n);
  if (!glyphs || !positions || !n) return false;
  result.reserve(n);
  for (unsigned i = 0; i < n; ++i) {
    result.push_back({glyphs[i].codepoint, glyphs[i].cluster,
                      positions[i].x_advance, positions[i].y_advance,
                      positions[i].x_offset, positions[i].y_offset});
  }
  return true;
}
}
