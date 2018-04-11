#ifndef GUETZLI_STATS_H_
#define GUETZLI_STATS_H_

#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace guetzli {
  static const char* const kNumItersCnt = "number of iterations";
  static const char* const kNumItersUpCnt = "number of iterations up";
  static const char* const kNumItersDownCnt = "number of iterations down";

  struct ProcessStats {
    ProcessStats() {}
    std::map<std::string, int> counters;
    std::string* debug_output = nullptr;
    FILE* debug_output_file = nullptr;
    std::string filename;
  };

}

#endif // GUETZLI_STATS_H_