#ifndef GUETZLI_PROCESSOR_H_
#define GUETZLI_PROCESSOR_H_

#include <string>
#include <vector>

// #include "guetzli/comparator.h"
// #include "guetzli/jpeg_data.h"
// #include "guetzli/stats.h"

namespace guetzli {
  struct Params {
    float butteraugli_target = 1.0;
    bool clear_metadata = true;
    bool try_420 = false;
    bool force_420 = false;
    bool use_silver_screen = false;
    bool new_zeroing_model = true;
  };
} // namespace guetzli

#endif // GUETZLI_PROCESSOR_H_