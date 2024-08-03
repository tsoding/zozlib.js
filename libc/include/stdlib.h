#ifndef _INC_STDLIB
#define _INC_STDLIB

#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#ifndef NO_STDLIB_INCLUDE

// DEFINES
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define MB_CUR_MAX 4
#define RAND_MAX 2147483647

// #define SIMPLE_PRINT
#ifdef SIMPLE_PRINT
void _print_string(const char *);
#endif

// DECLARATIONS

// STRING TO NUMBER
double atof(const char *);
int atoi(const char *);
long atol(const char *);
long long atoll(const char *);

double strtod(const char *, char **);
float strtof(const char *, char **);
long double strtold(const char *, char **);
long strtol(const char *, char **, int);
long long strtoll(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
unsigned long long strtoull(const char *, char **, int);

// RANDOM
int rand(void);
void srand(unsigned int);

// MEMORY
void *malloc(size_t byte_count);
void *calloc(size_t size, size_t element_size);
void free(void *ptr);
void *realloc(void *ptr, size_t new_byte_count);

// ENVIRONMENT
void abort(void);
void exit(int status);
void quick_exit(int);
int atexit(void (*)(void));
int at_quick_exit(void (*)(void));

// ALGORITHMS
void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*compar)(const void *, const void *));
void qsort(void *base, size_t num, size_t size, int (*compar)(const void *, const void *));

// INTEGER ARITHMETICS
int abs(int);
long labs(long);
long long llabs(long long);

typedef struct div_t div_t;
typedef struct ldiv_t ldiv_t;
typedef struct lldiv_t lldiv_t;

div_t div(int a, int b);
ldiv_t ldiv(long a, long b);
lldiv_t lldiv(long long a, long long b);

// MULTIBYTE CHARACTERS
int mblen(const char *, size_t);
int mbtowc(wchar_t *, const char *, size_t);
int wctomb(char *, wchar_t);

// MULTIBYTE STRING
size_t mbstowcs(wchar_t *dest, const char *src, size_t max);
size_t wcstombs(char *dest, const wchar_t *src, size_t max);

#endif

// IMPLEMENTATION
#ifdef STDLIB_IMPL

// MEMORY ALLOCATOR
#define MAX_PAGE_COUNT 16
#define PAGE_SIZE (64 * 1024)

#define BLOCKS_PER_PAGE 32
#define MAX_BLOCK_COUNT (MAX_PAGE_COUNT * BLOCKS_PER_PAGE)
#define DEFAULT_SIZE_OF_BLOCK (PAGE_SIZE / BLOCKS_PER_PAGE)

#define BLOCK_NUM_IN_PTR_TYPE size_t
#define BLOCK_NUM_IN_PTR_SIZE sizeof(BLOCK_NUM_IN_PTR_TYPE)

unsigned char *_heap_start();
unsigned char *_grow_memory(unsigned size);

unsigned __page_block_count[MAX_PAGE_COUNT];
size_t __used_page_count = 0;
unsigned char *__last_page_start = 0;
unsigned char *__last_page_end = 0;

size_t __used_block_start = 0;
size_t __block_size[MAX_BLOCK_COUNT];
unsigned char *__block_start[MAX_BLOCK_COUNT];
size_t __block_page_i[MAX_BLOCK_COUNT];
bool __block_used[MAX_BLOCK_COUNT];

size_t _next_unused_block(size_t start)
{
  for (unsigned i = start; i < MAX_BLOCK_COUNT; i++)
  {
    if (!__block_used[i])
      return i;
  }

  return MAX_BLOCK_COUNT;
}
void _add_pages_for(size_t byte_count)
{
  unsigned page_count;
  unsigned char *page_start;
  size_t pages_size;

  if (__used_page_count == 0)
  {
    page_start = _heap_start();
    const size_t heap_start_num = (size_t)page_start;
    const size_t page_size_left = PAGE_SIZE - (heap_start_num % PAGE_SIZE);
    page_count = page_size_left < byte_count ? ceil((double)(byte_count - page_size_left) / PAGE_SIZE) : 0;
    if (page_count != 0)
    {
      _grow_memory(page_count);
    }
    pages_size = page_size_left + page_count * PAGE_SIZE;
    page_count++;
  }
  else
  {
    page_count = ceil((double)byte_count / PAGE_SIZE);
    page_start = _grow_memory(page_count);
    pages_size = page_count * PAGE_SIZE;
  }

  for (unsigned i = 0; i < page_count * BLOCKS_PER_PAGE; i++)
  {
    __block_start[__used_block_start + i] = page_start + i * DEFAULT_SIZE_OF_BLOCK;
    __block_size[__used_block_start + i] = DEFAULT_SIZE_OF_BLOCK;
    __block_page_i[__used_block_start + i] = i / BLOCKS_PER_PAGE;
    __block_used[__used_page_count + i] = false;
  }
  __used_block_start += page_count * BLOCKS_PER_PAGE;

  for (unsigned i = 0; i < page_count; i++)
  {
    __page_block_count[__used_page_count + i] = BLOCKS_PER_PAGE;
  }
  __used_page_count += page_count;

  __last_page_start = page_start;
  __last_page_end = page_start + pages_size;
}
size_t _find_block(size_t start, size_t byte_count, long long end)
{
  // end == -1 : do not add new page & just end with return MAX_BLOCK_COUNT
  // end <= -2 : use default end of MAX_BLOCK_COUNT
  size_t MAX = end > MAX_BLOCK_COUNT ? end : MAX_BLOCK_COUNT;

  size_t unused = _next_unused_block(start);
  if (unused >= MAX)
  {
    if (end != -1)
    {
      _add_pages_for(byte_count);
      unused = _next_unused_block(start);
    }
  }
  while (unused < MAX)
  {
    unsigned int bs = __block_size[unused];

    // block does not fit
    if (bs < byte_count)
    {
      // Try to create block with large enough size
      unsigned size = bs;
      unsigned i = 1;
      while (!__block_used[unused + i] && unused + i < MAX && size < byte_count)
      {
        size += __block_size[unused + i];
        i++;
      }

      if (size >= byte_count)
      {
        __block_size[unused] = size;

        for (unsigned j = 1; j < i; j++)
        {
          __block_size[unused + j] = 0;
          __block_start[unused + j] = 0;
          __page_block_count[__block_page_i[unused + j]]--;
        }
        bs = size;
      }
    }

    // Found fitting block || new blocks were added
    if (bs >= byte_count)
    {
      if (unused > 0 && !__block_used[unused - 1] && __block_size[unused - 1] == 0) // block unused & not assigned
      {
        unsigned split_i = unused - 1;
        __block_start[split_i] = __block_start[unused];
        __block_size[split_i] = byte_count;
        __block_used[split_i] = true;

        __block_start[unused] += byte_count;
        __block_size[unused] -= byte_count;

        return split_i;
      }
      else if (unused + 1 < MAX && !__block_used[unused + 1] && __block_size[unused + 1] == 0)
      {
        unsigned split_i = unused + 1;
        __block_start[split_i] = __block_start[unused] + byte_count;
        __block_size[split_i] = __block_size[unused];

        __block_size[unused] = byte_count;

        return unused;
      }
      return unused; // cannot split blocks
    }

    unused = _next_unused_block(start);
    if (unused >= MAX)
    {
      if (end != -1)
      {
        _add_pages_for(byte_count);
        unused = _next_unused_block(start);
      }
    }
  }

  return MAX_BLOCK_COUNT;
}

void *malloc(size_t byte_count)
{
  if (__used_page_count == 0)
  {
    _add_pages_for(byte_count);
  }

  size_t block = _find_block(0, byte_count + BLOCK_NUM_IN_PTR_SIZE, -2);

  if (block < MAX_BLOCK_COUNT)
  {
    // Store block number at start
    ((BLOCK_NUM_IN_PTR_TYPE *)(__block_start[block]))[0] = block;
    __block_used[block] = true;
    return __block_start[block] + BLOCK_NUM_IN_PTR_SIZE; // give ptr to 4 byte after start (after block num)
  }
  else
  {
    return (void *)0;
  }
}
void *calloc(size_t size, size_t element_size)
{
  return malloc(size * element_size);
}
void free(void *ptr)
{
  unsigned block = ((unsigned *)ptr)[-1]; // get block number from before ptr

  if (0 <= block && block < MAX_BLOCK_COUNT)
  {
    if (__block_start[block] == (unsigned char *)ptr - 4)
    {
      __block_used[block] = false;
    }
    else
    {
#ifdef SIMPLE_PRINT
      _print_string("ERROR: Segmentation fault: Invalid ptr given to free");
#endif
    }
  }
  else
  {
#ifdef SIMPLE_PRINT
    _print_string("ERROR: Index out of bounds: Invalid ptr given to free");
#endif
  }
}

void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);

void *realloc(void *ptr, size_t new_byte_count)
{
  if (ptr == NULL)
  {
    return malloc(new_byte_count);
  }

  unsigned block = ((unsigned *)ptr)[-1]; // get block number from before ptr

  if (0 <= block && block < MAX_BLOCK_COUNT)
  {
    if (__block_start[block] != (unsigned char *)ptr - 4)
    {
#ifdef SIMPLE_PRINT
      _print_string("ERROR: Segmentation fault: Invalid ptr given to realloc");
#endif
      return (void *)0;
    }
  }
  else
  {
#ifdef SIMPLE_PRINT
    _print_string("ERROR: Index out of bounds: Invalid ptr given to realloc");
#endif
    return (void *)0;
  }

  size_t old___block_size = __block_size[block];
  unsigned char *old___block_start = __block_start[block];

  if (__block_size[block] < new_byte_count + BLOCK_NUM_IN_PTR_SIZE)
  {
    __block_used[block] = false; // mark temporary as unused

    size_t new_block = _find_block(block, new_byte_count + BLOCK_NUM_IN_PTR_SIZE, -1);
    if (new_block >= MAX_BLOCK_COUNT)
    {
      new_block = _find_block(0, new_byte_count + BLOCK_NUM_IN_PTR_SIZE, block);
    }
    if (new_block >= MAX_BLOCK_COUNT)
    {
      new_block = _find_block(block, new_byte_count + BLOCK_NUM_IN_PTR_SIZE, -2);
    }

    if (block < new_block || (block > new_block && __block_size[block] != 0))
    {
      // block is before new block
      // or old block was after but was not overwritten
      memcpy(__block_start[new_block], old___block_start, old___block_size);
      __block_used[new_block] = true;
      ((BLOCK_NUM_IN_PTR_TYPE *)(__block_start[block]))[0] = new_block;
      return __block_start[new_block] + BLOCK_NUM_IN_PTR_SIZE;
    }
    else if (block > new_block)
    {
      // old block is in new block
      memmove(__block_start[new_block], old___block_start, old___block_size);
      __block_used[new_block] = true;
      ((BLOCK_NUM_IN_PTR_TYPE *)(__block_start[block]))[0] = new_block;
      return __block_start[new_block] + BLOCK_NUM_IN_PTR_SIZE;
    }
    else // ==
    {
      __block_used[block] = true;
      return __block_start[block] + BLOCK_NUM_IN_PTR_SIZE;
    }
  }
  else if (__block_size[block] > new_byte_count + BLOCK_NUM_IN_PTR_SIZE)
  {
    return __block_start[block] + BLOCK_NUM_IN_PTR_SIZE;
  }
  else
  {
    return __block_start[block] + BLOCK_NUM_IN_PTR_SIZE;
  }
}

// ALGORITHMS
// https://cplusplus.com/reference/cstdlib/bsearch/
void *bsearch(const void *key, const void *base, size_t num, size_t size, int (*compar)(const void *, const void *))
{
  size_t start = 0;
  size_t end = num;
  size_t middle = num / 2;
  const unsigned char *ptr = (const unsigned char *)base + (middle * size);
  int cmp = compar(key, ptr);
  while (cmp != 0)
  {
    if (cmp < 0)
    {
      end = middle - 1;
    }
    else
    {
      start = middle + 1;
    }

    middle = start + (end - start) / 2;
    ptr = (const unsigned char *)base + (middle * size);
    cmp = compar(key, ptr);
  }

  return (void *)ptr;
}
// https://cplusplus.com/reference/cstdlib/qsort/
void qsort(void *base, size_t num, size_t size, int (*compar)(const void *, const void *))
{
  unsigned char *tmp = (unsigned char *)malloc(size);
  unsigned char *base_c = (unsigned char *)base;

  for (size_t i = 1; i < num; i++)
  {
    size_t j = 0;

    // while want to insert is larger
    int cmp = compar(base_c + i * size, base_c + j * size);
    while (cmp >= 0)
    {
      j++;
      cmp = compar(base_c + i * size, base_c + j * size);
    }

    // swap
    for (size_t k = 0; k < size; k++)
    {
      tmp[k] = base_c[i + k];
      base_c[i + k] = base_c[j + k];
      base_c[j + k] = tmp[k];
    }
    break;
  }

  free(tmp);
}

// INTEGER ARITHMETICS
struct div_t
{
  int quot;
  int rem;
};
struct ldiv_t
{
  long quot;
  long rem;
};
struct lldiv_t
{
  long long quot;
  long long rem;
};

struct div_t div(int a, int b)
{
  const int quot = a / b;
  const int rem = a - (quot * b);
  struct div_t res;
  res.quot = quot;
  res.rem = rem;
  return res;
};
struct ldiv_t ldiv(long a, long b)
{
  const long quot = a / b;
  const long rem = a - (quot * b);
  struct ldiv_t res;
  res.quot = quot;
  res.rem = rem;
  return res;
}
struct lldiv_t lldiv(long long a, long long b)
{
  const long long quot = a / b;
  const long long rem = a - (quot * b);
  struct lldiv_t res;
  res.quot = quot;
  res.rem = rem;
  return res;
}

#endif
#endif