#ifndef GUETZLI_JPEG_HUFFMAN_DECODE_H_
#define GUETZLI_JPEG_HUFFMAN_DECODE_H_

#include <inttypes.h>

namespace guetzli {

static const int kJpegHuffmanRootTableBits = 8;
// Maximum huffman lookup table size.
static const int kJpegHuffmanLutSize = 758;

struct HuffmanTableEntry {
  HuffmanTableEntry() : bits(0), value(0xffff) {}

  uint8_t bits;
  uint16_t value;
};

} // namespace guetzli

#endif // GUETZLI_JPEG_HUFFMAN_DECODE_H_