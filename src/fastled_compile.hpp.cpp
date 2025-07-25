// FastLED All Source Build File
// This file includes all .cpp.hpp files for unified compilation
// Generated automatically by scripts/all_source_build.py

#include "fl/compiler_control.h"

#if FASTLED_ALL_SRC

// TODO: Remove this from the top level.
#ifdef ESP32
#if defined(__has_include) && __has_include("sdkconfig.h")
#include "sdkconfig.h"
#endif  // sdkconfig
#endif  // ESP32


// Platform implementations (auto-generated)

// Third-party implementations (auto-generated, excluding FFT)

// SENSORS MODULE (hierarchical)
#ifdef FASTLED_ALL_SRC
#include "sensors/sensors_compile.hpp"

// FX MODULE (hierarchical)
#include "fx/fx_compile.hpp"

// FL MODULE (hierarchical)
#include "fl/fl_compile.hpp"

// PLATFORMS MODULE (hierarchical)
#include "platforms/platforms_compile.hpp"

// THIRD PARTY MODULE (hierarchical)
#include "third_party/third_party_compile.hpp"

// ROOT SRC MODULE (hierarchical)
#include "src_compile.hpp"
#endif // FASTLED_ALL_SRC

// SENSORS MODULE (hierarchical)
#ifdef FASTLED_ALL_SRC
#include "sensors/sensors_compile.hpp"
#endif // FASTLED_ALL_SRC



#endif // FASTLED_ALL_SRC
