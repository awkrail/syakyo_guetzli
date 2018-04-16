#ifndef GUETZLI_JPEG_DATA_READER_H_
#define GUETZLI_JPEG_DATA_READER_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "guetzli/jpeg_data.h"

namespace guetzli {

enum JpegReadMode {
  JPEG_READ_HEADER,
  JPEG_READ_TABLES,
  JPEG_READ_ALL,
};

bool ReadJpeg(const uint8_t* data, const size_t len, JpegReadMode mode,
              JPEGData* jpg);

bool ReadJpeg(const std::string& data, JpegReadMode mode,
              JPEGData* jpg);

}

#endif