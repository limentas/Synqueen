#pragma once

#include "compilerwarnings.hpp"

// To suppress undesired warnings from Corral library
SAVE_WARNINGS()
SUPPRESS_DATA_LOSS()
#include <corral/corral.h>
RESTORE_WARNINGS()
