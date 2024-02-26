#ifndef _INC_ASSERT
#define _INC_ASSERT

#ifdef NDEBUG

#define assert(expression) ((void)0)

#else

#ifndef NO_ASSERT_INCLUDE
void _assert(const char *message, const char *file, unsigned line);
#endif

#ifdef ASSERT_IMPL

#define assert(expression) (void)((!!(expression)) || \
                                  (_assert(#expression, __FILE__, (unsigned)(__LINE__)), 0))

// Cannot import stdlib or stdio as it is imported by them
void exit(int);
int printf(const char *, ...);
void _assert(const char *message, const char *file, unsigned line)
{
  printf("Assertion failed at %s : %i\n\t%s", file, line, message);
  exit(1);
}
#endif

#endif

#endif // _INC_ASSERT