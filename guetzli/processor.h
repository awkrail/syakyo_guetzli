#ifndef GUETZLI_PROCESSOR_H_
#define GUETZLI_PROCESSOR_H_

#include <string>
#include <vector>

// #include "guetzli/comparator.h"
#include "guetzli/jpeg_data.h"
#include "guetzli/stats.h"

namespace guetzli {
  struct Params {
    float butteraugli_target = 1.0;
    bool clear_metadata = true;
    bool try_420 = false;
    bool force_420 = false;
    bool use_silver_screen = false;
    bool new_zeroing_model = true;
  };

  bool Process(const Params& params, ProcessStats* stats,
               const std::string& in_data,
               std::string* out_data);
  
  bool Process(const Params& params, ProcessStats* stats,
               const std::vector<uint8_t>& rgb, int w, int h,
               std::string* out);
} // namespace guetzli

#endif // GUETZLI_PROCESSOR_H_