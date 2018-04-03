workspace "guetzli"
    configurations { "Release", "Debug" }
    language "C++"
    flags { "C++11" }
    includedirs { ".", "third_party/butteraugli" }

    filter "action:vs*"
      platforms { "x86_64", "x86" }
    
    filter "platforms:x86"
      architecture "x86"
    filter "platforms:x86_64"
      architecture "x86_64"
    
    filter "action:gmake"
      symbols "On"
    filter "configurations:Release"
      optimize "Full"
    filter {}
  
    project "guetzli_static"
      kind "StaticLib"
      files
        {
          "guetzli/*.cc",
          "guetzli/*.h",
          "third_party/butteraugli/butteraugli.cc",
          "third_party/butteraugli/butteraugli.h"
        }

