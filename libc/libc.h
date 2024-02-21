#ifdef PLATFORM_WEB

#define ERRNO_IMPL
#include "errno.h"

#define ASSERT_IMPL
#include "assert.h"

#define FENV_IMPL
#include "fenv.h"

#define STDLIB_IMPL
#include "stdlib.h"

#define STRING_IMPL
#include "string.h"

#define TIME_IMPL
#include "time.h"

#define STDIO_IMPL
#include "stdio.h"

#include <stdbool.h>
bool is_file_ready(FILE *stream) { return stream->ready; }
#else
#include <stdbool.h>
#include <stdio.h>
bool is_file_ready(FILE *stream) { return true; }
#endif