#include "guetzli/jpeg_data_reader.h"
#include "guetzli/jpeg_huffman_decode.h"

#include <iostream> // debug

#include <algorithm>
#include <stdio.h>
#include <string.h>

namespace guetzli {

namespace {

#define VERIFY_LEN(n)                                                     \
  if(*pos + (n) > len) {                                                  \
    fprintf(stderr, "Unexpected end of input: pos=%d need=%d len=%d\n",   \
            static_cast<int>(*pos), static_cast<int>(n),                  \
            static_cast<int>(len));                                       \
    jpg->error = JPEG_UNEXPECTED_EOF;                                     \
    return false;                                                         \
  }

#define VERIFY_INPUT(var, low, high, code)                                \
  if( var < low || var > high) {                                          \
    fprintf(stderr, "Invalid %s: %d\n", #var, static_cast<int>(var));     \
    jpg->error = JPEG_INVALID_ ## code;                                   \
    return false;                                                         \
  }

#define VERIFY_MARKER_END()                                             \
  if (start_pos + marker_len != *pos) {                                 \
    fprintf(stderr, "Invalid marker length: declared=%d actual=%d\n",   \
            static_cast<int>(marker_len),                               \
            static_cast<int>(*pos - start_pos));                        \
    jpg->error = JPEG_WRONG_MARKER_SIZE;                                \
    return false;                                                       \
  }

#define EXPECT_MARKER() \
  if(pos + 2 > len || data[pos] != 0xff) {                                \
    fprintf(stderr, "Marker byte (0xff) expected, found : %d "            \
            "pos=%d len=%d\n",                                            \
            (pos < len ? data[pos] : 0), static_cast<int>(pos),           \
            static_cast<int>(len));                                       \
    jpg->error = JPEG_MARKER_BYTE_NOT_FOUND;                              \
    return true;                                                          \
  }

inline int ReadUint8(const uint8_t* data, size_t* pos) {
  return data[(*pos)++];
}

inline int ReadUint16(const uint8_t* data, size_t* pos) {
  int v = (data[*pos] << 8) + data[*pos + 1];
  *pos += 2;
  return v;
}

inline int DivCeil(int a, int b) {
  return (a + b - 1) / b;
}

bool ProcessSOF(const uint8_t* data, const size_t len,
                JpegReadMode mode, size_t* pos, JPEGData* jpg) {
  // なぜかwidthだけ見ている
  if(jpg->width != 0) {
    fprintf(stderr, "Duplicate SOF marker.\n");
    jpg->error = JPEG_DUPLICATE_SOF;
    return false;
  }

  const size_t start_pos = *pos;
  VERIFY_LEN(8);
  size_t marker_len = ReadUint16(data, pos);
  int precision = ReadUint8(data, pos); // 8しか許されてないようだが...?
  int height = ReadUint16(data, pos);
  int width = ReadUint16(data, pos);
  int num_components = ReadUint8(data, pos);
  VERIFY_INPUT(precision, 8, 8, PRECISION);
  VERIFY_INPUT(height, 1, 65535, HEIGHT);
  VERIFY_INPUT(width, 1, 65535, WIDTH);
  VERIFY_INPUT(num_components, 1, kMaxComponents, NUMCOMP);
  VERIFY_LEN(3 * num_components);
  jpg->height = height;
  jpg->width = width;
  jpg->components.resize(num_components);

  // Read sampling factors and quant table index for each component.
  std::vector<bool> ids_seen(256, false);
  for(size_t i = 0; i < jpg->components.size(); ++i) {
    const int id = ReadUint8(data, pos);
    if(ids_seen[id]) {
      fprintf(stderr, "Duplicate ID %d in SOF. \n", id);
      jpg->error = JPEG_DUPLICATE_COMPONENT_ID;
      return false;
    }
    ids_seen[id] = true;
    jpg->components[i].id = id;
    int factor = ReadUint8(data, pos);
    int h_samp_factor = factor >> 4;
    int v_samp_factor = factor & 0xf;
    VERIFY_INPUT(h_samp_factor, 1, 15, SAMP_FACTOR);
    VERIFY_INPUT(v_samp_factor, 1, 15, SAMP_FACTOR);
    jpg->components[i].h_samp_factor = h_samp_factor;
    jpg->components[i].v_samp_factor = v_samp_factor;
    jpg->components[i].quant_idx = ReadUint8(data, pos);
    jpg->max_h_samp_factor = std::max(jpg->max_h_samp_factor, h_samp_factor);
    jpg->max_v_samp_factor = std::max(jpg->max_v_samp_factor, v_samp_factor);
  }

  // Sampling Factor must not be 0
  jpg->MCU_rows = DivCeil(jpg->height, jpg->max_v_samp_factor * 8);
  jpg->MCU_cols = DivCeil(jpg->width, jpg->max_h_samp_factor * 8);

  // Compute the block dimensions for each component
  if(mode == JPEG_READ_ALL) {
    for(size_t i = 0; i < jpg->components.size(); ++i) {
      JPEGComponent* c = &jpg->components[i];
      if(jpg->max_h_samp_factor % c->h_samp_factor != 0 ||
         jpg->max_v_samp_factor % c->v_samp_factor != 0) {
           fprintf(stderr, "Non-integral subsampling ratios.\n");
           jpg->error = JPEG_INVALID_SAMPLING_FACTORS;
           return false;
         }
      c->width_in_blocks = jpg->MCU_cols * c->h_samp_factor;
      c->height_in_blocks = jpg->MCU_rows * c->v_samp_factor;
      const uint64_t num_blocks =
          static_cast<uint64_t>(c->width_in_blocks) * c->height_in_blocks;
      if(num_blocks > (1ull << 21)) {
        fprintf(stderr, "Image too large\n");
        jpg->error = JPEG_IMAGE_TOO_LARGE;
        return false;
      }
      c->num_blocks = static_cast<int>(num_blocks);
      c->coeffs.resize(c->num_blocks * kDCTBlockSize);
    }
  }
  VERIFY_MARKER_END();
  return true;
}

bool ProcessSOS(const uint8_t* data, const size_t len, size_t* pos,
                JPEGData* jpg) {}

// DHT定義
// http://d.hatena.ne.jp/kuriken12/20100116/1263624765
/**
 * 0xFF 0xDE
 * Lh .. マーカーの長さ
 * Tc .. DC成分かAC成分か
 * Th ~ 最後まで : 繰り返し
 * Th => id(多くの場合はYについてAC, DCを持ち CbCrでAC, DCを一つ持つ => 合計4つ)
**/
bool ProcessDHT(const uint8_t* data, const size_t len,
                JpegReadMode mode,
                std::vector<HuffmanTableEntry>* dc_huff_lut,
                std::vector<HuffmanTableEntry>* ac_huff_lut,
                size_t* pos,
                JPEGData* jpg) {
  const size_t start_pos = *pos;
  VERIFY_LEN(2);
  size_t marker_len = ReadUint16(data, pos);
  if(marker_len == 2) {
    fprintf(stderr, "DHT marker: no Huffman table found");
    jpg->error = JPEG_EMPTY_DHT;
  }
  // 日曜日に読み直し
  return true;
}

bool ProcessDQT(const uint8_t* data, const size_t len, size_t* pos,
                JPEGData* jpg) {
  const size_t start_pos = *pos;
  VERIFY_LEN(2);
  size_t marker_len = ReadUint16(data, pos);
  if(marker_len == 2) {
    fprintf(stderr, "DQT marker: no quantization table found\n");
    jpg->error = JPEG_EMPTY_DQT;
  }
  while(*pos < start_pos + marker_len && jpg->quant.size() < kMaxQuantTables) {
    VERIFY_LEN(1);
    int quant_table_index = ReadUint8(data, pos);
    int quant_table_precision = quant_table_index >> 4;
    quant_table_index &= 0xf;
    VERIFY_INPUT(quant_table_index, 0, 3, QUANT_TBL_INDEX);
    VERIFY_LEN((quant_table_precision ? 2 : 1) * kDCTBlockSize);
    JPEGQuantTable table;
    table.index = quant_table_index;
    table.precision = quant_table_precision;
    for(int i=0; i < kDCTBlockSize; ++i) {
      int quant_val = quant_table_precision ? ReadUint16(data, pos) : ReadUint8(data, pos);
      VERIFY_INPUT(quant_val, 1, 65535, QUANT_VAL);
      table.values[kJPEGNaturalOrder[i]] = quant_val; // igZag->Parseして元に戻す
    }
    table.is_last = (*pos == start_pos + marker_len);
    jpg->quant.push_back(table);
  }
  VERIFY_MARKER_END();
  return true;
}

// Restart interval
bool ProcessDRI(const uint8_t* data, const size_t len, size_t* pos,
                JPEGData* jpg) {
  if(jpg->restart_interval > 0) {
    fprintf(stderr, "Duplicate DRI marker.\n");
    jpg->error = JPEG_DUPLICATE_DRI;
    return false;
  }
  const size_t start_pos = *pos;
  VERIFY_LEN(4);
  size_t marker_len = ReadUint16(data, pos);
  int restart_interval = ReadUint16(data, pos);
  jpg->restart_interval = restart_interval;
  return true;             
}

// Save APP marker, and not recognize other information about APP.
bool ProcessAPP(const uint8_t* data, const size_t len, size_t* pos,
                JPEGData* jpg) {
  VERIFY_LEN(2);
  size_t marker_len = ReadUint16(data, pos);
  VERIFY_INPUT(marker_len, 2, 65535, MARKER_LEN);
  VERIFY_LEN(marker_len - 2);

  // Save the marker type together with the app data.
  std::string app_str(reinterpret_cast<const char*>(
      &data[*pos - 3]), marker_len + 1);
  *pos += marker_len - 2;
  jpg->app_data.push_back(app_str);
  return true;              
}

// Save COM marker as a string
bool ProcessCOM(const uint8_t* data, const size_t len, size_t* pos,
                JPEGData* jpg) {
  VERIFY_LEN(2);
  size_t marker_len = ReadUint16(data, pos);
  VERIFY_INPUT(marker_len, 2, 65535, MARKER_LEN);
  VERIFY_LEN(marker_len - 2);
  std::string com_str(reinterpret_cast<const char*>(
      &data[*pos - 2]), marker_len);
  *pos += marker_len - 2;
  jpg->com_data.push_back(com_str);
  return true;              
}

bool ProcessScan(const uint8_t* data, const size_t len,
                 const std::vector<HuffmanTableEntry>& dc_huff_lut,
                 const std::vector<HuffmanTableEntry>& ac_huff_lut,
                 uint16_t scan_progression[kMaxComponents][kDCTBlockSize],
                 bool is_progressive,
                 size_t* pos,
                 JPEGData* jpg) {
  // 未実装
  return true;
  }

bool FixupIndexes(JPEGData* jpg) {
  // 未実装
  return true;
}


size_t FindNextMarker(const uint8_t* data, const size_t len, size_t pos) {
  static const uint8_t kIsValidMarker[] = {
    1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
  };
  size_t num_skipped = 0;
  while(pos + 1 < len &&
        (data[pos] != 0xff || data[pos+1] < 0xc0)) {
    ++pos;
    ++num_skipped;
  }
  return num_skipped;
}

}

// 参考URL
/**
 *  https://www.setsuki.com/hsp/ext/jpg.htm
 *  https://hp.vector.co.jp/authors/VA032610/contents.htm
 **/

bool ReadJpeg(const uint8_t* data, const size_t len, JpegReadMode mode,
              JPEGData* jpg) {
  size_t pos = 0;
  EXPECT_MARKER();
  int marker = data[pos + 1];
  pos += 2;
  if(marker != 0xd8) {
    fprintf(stderr, "Did not find expected SOI marker, actual=%d\n", marker);
    jpg->error = JPEG_SOI_NOT_FOUND;
    return false;
  }

  // Huffman絡みのデータについてよく分からない
  // DHT定義 : ProcessDHTで利用する
  int lut_size = kMaxHuffmanTables * kJpegHuffmanLutSize;
  std::vector<HuffmanTableEntry> dc_huff_lut(lut_size);
  std::vector<HuffmanTableEntry> ac_huff_lut(lut_size);
  bool found_sof = false;
  uint16_t scan_progression[kMaxComponents][kDCTBlockSize] = { {0} };

  bool is_progressive = false;

  do {
    // Read Next Marker
    size_t num_skipped = FindNextMarker(data, len, pos);
    if(num_skipped > 0) {
      jpg->marker_order.push_back(0xff);
      jpg->inter_marker_data.push_back(
        std::string(reinterpret_cast<const char*>(&data[pos]),
                                    num_skipped));
      pos += num_skipped;
    }
    EXPECT_MARKER();
    marker = data[pos + 1];
    pos += 2;
    bool ok = true;

    switch(marker) {
      case 0xc0: // ベースラインDCT
      case 0xc1: // 拡張シーケンシャル
      case 0xc2: // プログレッシブDCT
        is_progressive = (marker == 0xc2);
        ok = ProcessSOF(data, len, mode, &pos, jpg);
        found_sof = true;
        break;
      case 0xc4: // ハフマン法テーブル定義
        ok = ProcessDHT(data, len, mode, &dc_huff_lut, &ac_huff_lut, &pos, jpg);
        break;
      case 0xd0:
      case 0xd1:
      case 0xd2:
      case 0xd3:
      case 0xd4:
      case 0xd5:
      case 0xd6:
      case 0xd7:
        // Restart Maker don't have any data.
        break;
      case 0xd9:
        // end marker
        break;
      case 0xda: // SOS
        if(mode == JPEG_READ_ALL) {
          ok = ProcessScan(data, len, dc_huff_lut, ac_huff_lut,
                          scan_progression, is_progressive, &pos, jpg);
        }
        break;
      case 0xdb: // 量子化テーブル定義
        ok = ProcessDQT(data, len, &pos, jpg);
        break;
      case 0xdd: // リスタートの間隔の定義
        ok = ProcessDRI(data, len, &pos, jpg);
        break;
      case 0xe0:
      case 0xe1:
      case 0xe2:
      case 0xe3:
      case 0xe4:
      case 0xe5:
      case 0xe6:
      case 0xe7:
      case 0xe8:
      case 0xe9:
      case 0xea:
      case 0xeb:
      case 0xec:
      case 0xed:
      case 0xee:
      case 0xef:
        if(mode != JPEG_READ_TABLES) {
          ok = ProcessAPP(data, len, &pos, jpg);
        }
        break;
      case 0xfe:
        if(mode != JPEG_READ_TABLES) {
          ok = ProcessCOM(data, len, &pos, jpg);
        }
        break;
      default:
        fprintf(stderr, "Unsupported marker: %d pos=%d len=%d\n",
                marker, static_cast<int>(pos), static_cast<int>(len));
        jpg->error = JPEG_UNSUPPORTED_MARKER;
        ok = false;
        break;
    }
    if(!ok) {
      return false;
    }
    jpg->marker_order.push_back(marker);
    if(mode == JPEG_READ_HEADER && found_sof) {
      break;
    }
  } while (marker != 0xd9);

  // .. => データすべて読み込んでいる? => 補足でデータがあるかもしれない
  if(mode == JPEG_READ_ALL) {
    if(pos < len) {
      jpg->tail_data.assign(reinterpret_cast<const char*>(&data[pos]),
                            len - pos);
    }
    if(!FixupIndexes(jpg)) {
      return false;
    }
    if(jpg->huffman_code.size() == 0) {
      fprintf(stderr, "Need at least one Huffman code table. \n");
      jpg->error = JPEG_HUFFMAN_TABLE_ERROR;
      return false;
    }
    if(jpg->huffman_code.size() >= kMaxDHTMarkers) {
      fprintf(stderr, "Too many Huffman tables. \n");
      jpg->error = JPEG_HUFFMAN_TABLE_ERROR;
      return false;
    }
  }

  //int lut_size = kMaxHuffmanTables * kJpegHuffmanLut
  std::cout << marker << std::endl;

  return true;
}

bool ReadJpeg(const std::string& data, JpegReadMode mode,
              JPEGData* jpg) {
  return ReadJpeg(reinterpret_cast<const uint8_t*>(data.data()),
                  static_cast<const size_t>(data.size()),
                  mode, jpg);
}
} // namespace guetzli