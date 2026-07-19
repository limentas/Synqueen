#pragma once

#include "platform.hpp"

#ifdef SQ_OS_WINDOWS
#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR_STR "\\"
#else
#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_STR "/"
#endif
