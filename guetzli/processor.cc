#include "guetzli/processor.h"

namespace guetzli {

bool Process(const Params& params, ProcessStats* stats,
             const std::string& data,
             std::string* jpg_out) {
               return true;
             }

bool Process(const Params& params, ProcessStats* stats,
             const std::vector<uint8_t>& rgb, int w, int h,
             std::string* jpg_out) {
               return true;
             }
}