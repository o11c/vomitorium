#pragma once

#include "vgcc/vgcc-version-check.h"

#define V(...) HAS_VGCC_VERSION(__VA_ARGS__)
#define G(...) HAS_GCC_VERSION(__VA_ARGS__)

#if !G(4, 6)
# define nullptr NULL
#endif

#if !G(4, 7)
# define override /*nothing*/
#endif

