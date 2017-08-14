#ifndef NINKASI_COMMON_H
#define NINKASI_COMMON_H

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>

#include "nkconfig.h"

#include "nktypes.h"
#include "nkenums.h"
#include "vmstack.h"
#include "value.h"
#include "nkopcode.h"
#include "nkexpr.h"
#include "optimize.h"
#include "tokenize.h"
#include "vmdbg.h"
#include "nkerror.h"
#include "nkdynstr.h"
#include "nkvm.h"
#include "nkcompil.h"
#include "nkstring.h"
#include "nkfunc.h"
#include "objects.h"
#include "nkmem.h"
#include "nkfail.h"

#endif // NINKASI_COMMON_H
