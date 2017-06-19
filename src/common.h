#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

// TODO: Get all this crap from common.h in the engine.
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char bool;
#define false ((bool)0)
#define true ((bool)1)


#include "vmstack.h"
#include "value.h"
#include "opcodes.h"

#endif
