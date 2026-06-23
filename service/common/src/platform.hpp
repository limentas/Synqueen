#pragma once

// Platform detection macros
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#define SQ_OS_WINDOWS 1
#elif defined(__ANDROID__) || defined(ANDROID)
#define SQ_OS_ANDROID 1
#elif defined(__linux__) || defined(__linux)
#define SQ_OS_LINUX 1
#endif
