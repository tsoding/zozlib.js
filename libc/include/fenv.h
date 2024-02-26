#ifndef _INC_FENV
#define _INC_FENV

// DEFINES

#define FE_DOWNWARD 0
#define FE_TONEAREST 1
#define FE_TOWARDZERO 2
#define FE_UPWARD 3

#define FE_DIVBYZERO 0x01  // Pole error: division by zero, or some other asymptotically infinite result (from finite arguments).
#define FE_INEXACT 0x02    // Inexact: the result is not exact.
#define FE_INVALID 0x04    // Domain error: At least one of the arguments is a value for which the function is not defined.
#define FE_OVERFLOW 0x08   // Overflow range error: The result is too large in magnitude to be represented as a value of the return type.
#define FE_UNDERFLOW 0x10  // Underflow range error: The result is too small in magnitude to be represented as a value of the return type.
#define FE_ALL_EXCEPT 0x1F // All exceptions (selects all of the exceptions supported by the implementation).

#ifndef NO_FENV_INCLUDE

// TYPES
typedef int fexcept_t;
typedef struct fenv_t fenv_t;

// DEFINITIONS
fenv_t *_create_default_env();
#define FE_DFL_ENV _create_default_env()

// EXCEPTIONS
int feclearexcept(int excepts);
int feraiseexcept(int excepts);
int fegetexceptflag(fexcept_t *flagp, int excepts);
int fesetexceptflag(const fexcept_t *flagp, int excepts);
int fetestexcept(int excepts);

// ENV
int fegetenv(fenv_t *envp);
int fesetenv(const fenv_t *envp);
int feholdexcept(fenv_t *envp);
int feupdateenv(const fenv_t *envp);

// ROUNDING
int fegetround(void);
int fesetround(int mode);

#endif

// IMPLEMENTATION
#ifdef FENV_IMPL

#ifdef NO_FENV_INCLUDE
typedef int fexcept_t;
#endif
struct fenv_t
{
  int exception_flags;
  int rounding_mode;
};

//
// NOT PER THREAD
int __fe_exception_flag = 0;
int __fe_rounding_mode = 0;

struct fenv_t __fe_env;
struct fenv_t *_create_default_env()
{
  __fe_env.exception_flags = 0;
  __fe_env.rounding_mode = 0;
  return &__fe_env;
}

// EXCEPTIONS
int feclearexcept(int excepts)
{
  __fe_exception_flag = __fe_exception_flag & ~excepts;
  return 0;
}
int feraiseexcept(int excepts)
{
  __fe_exception_flag = __fe_exception_flag | excepts;
  return 0;
}
int fegetexceptflag(fexcept_t *flagp, int excepts)
{
  *flagp = excepts;
  return 0;
}
int fesetexceptflag(const fexcept_t *flagp, int excepts)
{
  __fe_exception_flag = __fe_exception_flag | (*flagp) & excepts;
  return 0;
}
int fetestexcept(int excepts)
{
  return __fe_exception_flag & excepts;
}

// ENV
int fegetenv(struct fenv_t *envp)
{
  envp->exception_flags = __fe_exception_flag;
  envp->rounding_mode = __fe_rounding_mode;
  return 0;
}
int fesetenv(const struct fenv_t *envp)
{
  __fe_exception_flag = envp->exception_flags;
  __fe_rounding_mode = envp->rounding_mode;
  return 0;
}
int feholdexcept(struct fenv_t *envp)
{
  fegetenv(envp);

  __fe_exception_flag = 0;
  __fe_rounding_mode = 0;

  return 0;
}
int feupdateenv(const struct fenv_t *envp)
{
  fesetenv(envp);
  // raise the exceptions ?
  return 0;
}

// ROUNDING
int fegetround(void)
{
  return __fe_rounding_mode;
}
void _js_fesetround(int mode);
int fesetround(int mode)
{
  __fe_rounding_mode = mode;
  _js_fesetround(mode);
  return 0;
}
#endif
#endif