#pragma once

#include "platform.hpp"

#ifdef SQ_CC_MSVC

#define SAVE_WARNINGS() _Pragma("warning(push)")

#define SUPPRESS_DEPRECATED() _Pragma("warning(disable : 4996)")

#define SUPPRESS_DATA_LOSS() _Pragma("warning(disable : 4244 4267)")

#define RESTORE_WARNINGS() _Pragma("warning(pop)")

#elif defined(SQ_CC_CLANG)

#define SAVE_WARNINGS() _Pragma("clang diagnostic push")

#define SUPPRESS_DEPRECATED()                                                  \
  _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")

#define SUPPRESS_DATA_LOSS()                                                   \
  _Pragma("clang diagnostic ignored \"-Wconversion\"")

#define RESTORE_WARNINGS() _Pragma("clang diagnostic pop")

#elif defined(SQ_CC_GNU)

#define SAVE_WARNINGS() _Pragma("GCC diagnostic push")

#define SUPPRESS_DEPRECATED()                                                  \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")

#define SUPPRESS_DATA_LOSS() _Pragma("GCC diagnostic ignored \"-Wconversion\"")

#define RESTORE_WARNINGS() _Pragma("GCC diagnostic pop")

#else

#define SAVE_WARNINGS()
#define SUPPRESS_DEPRECATED()
#define SUPPRESS_DATA_LOSS()
#define RESTORE_WARNINGS()

#endif
