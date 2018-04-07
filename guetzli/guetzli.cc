#include <iostream>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception> // std::set_terminate
#include <memory> // smart pointer
#include <string>
#include <sstream>
#include <string.h>
#include "png.h"
#include "guetzli/processor.h"
#include "guetzli/quality.h"
// #include "guetzli/stats.h"
// #include "guetzli/jpeg_data.h"
// #include "guetzli/jpeg_data_reader.h"

namespace {

  constexpr int kDefaultJPEGQuality = 95;
  constexpr int kLowestMemusageMB = 100; // in MB
  constexpr int kDefaultMemlimitMB = 6000; // in MB

  void TerminateHandler() {
    fprintf(stderr, "Unhandled exception. Most likely insufficient memory available.\n"
          "Make sure that there is 300MB/MPix of memory available.\n");
    exit(1);
  }

  void Usage() {
    fprintf(stderr,
        "Guetzli JPEG compressor. Usage: \n"
        "guetzli [flags] input_filename output_filename\n"
        "\n"
        "Flags:\n"
        "  --verbose    - Print a verbose trace of all attempts to standard output.\n"
        "  --quality Q  - Visual quality to aim for, expressed as a JPEG quality value.\n"
        "                 Default value is %d.\n"
        "  --memlimit M - Memory limit in MB. Guetzli will fail if unable to stay under\n"
        "                 the limit. Default limit is %d MB.\n"
        "  --nomemlimit - Do not limit memory usage.\n", kDefaultJPEGQuality, kDefaultMemlimitMB);
    exit(1);
  }

  std::string ReadFileOrDie(const char* filename) {
    bool read_from_stdin = strncmp(filename, "-", 2) == 0;
    FILE* f = read_from_stdin ? stdin : fopen(filename, "rb");
    if(!f) {
      perror("Can't open input file");
      exit(1);
    }

    std::string result;
    off_t buffer_size = 8192;

    if(fseek(f, 0, SEEK_END) == 0) {
      buffer_size = std::max<off_t>(ftell(f), 1); // get buffersize
      if(fseek(f, 0, SEEK_SET) != 0) {
        perror("fseek");
        exit(1);
      }
    } else if (ferror(f)) {
      perror("fseek");
      exit(1);
    }

    std::unique_ptr<char []> buf(new char[buffer_size]);
    while(!feof(f)) {
      size_t read_bytes = fread(buf.get(), sizeof(char), buffer_size, f);
      if(ferror(f)) {
        perror("fread");
        exit(1);
      }
      result.append(buf.get(), read_bytes);
    }

    fclose(f);
    return result;
  }
}

int main(int argc, char** argv) {
  std::set_terminate(TerminateHandler);
  
  int verbose = 0;
  int quality = kDefaultJPEGQuality;
  int memlimit_mb = kDefaultMemlimitMB;

  int opt_idx = 1;
  for(; opt_idx < argc; opt_idx++) {
    if(strnlen(argv[opt_idx], 2) < 2 || argv[opt_idx][0] != '-' || argv[opt_idx][1] != '-') {
      break;
    }
    if(!strcmp(argv[opt_idx], "--verbose")) {
      verbose = 1;
    } else if (!strcmp(argv[opt_idx], "--quality")) {
      opt_idx++;
      if(opt_idx >= argc) Usage();
      quality = atoi(argv[opt_idx]);
    } else if (!strcmp(argv[opt_idx], "--memlimit")) {
      opt_idx++;
      if(opt_idx >= argc) Usage();
      memlimit_mb = atoi(argv[opt_idx]);
    } else if (!strcmp(argv[opt_idx], "--nomemlimit")) {
      memlimit_mb = -1;
    } else if (!strcmp(argv[opt_idx], "--")) {
      opt_idx++;
      break;
    } else {
      fprintf(stderr, "Unknown commandline flag : %s\n", argv[opt_idx]);
      Usage();
    }
  }

  if(argc - opt_idx != 2) {
    Usage();
  }

  std::string in_data = ReadFileOrDie(argv[opt_idx]);
  std::string out_data;

  guetzli::Params params;
  float q = guetzli::ButteraugliScoreForQuality(quality);
  std::cout << q << std::endl;
  //params.butteraugli_target = static_cast<float>(guetzli::ButteraugliScoreForQuality(quality));
}
