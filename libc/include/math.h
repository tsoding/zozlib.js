#ifndef _INC_MATH
#define _INC_MATH

#include <fenv.h> // wasm needs the export

#define INFINITY (1.0 / 0.0)
#define NAN (0.0 / 0.0)
#define HUGE_VAL (double)INFINITY
#define HUGE_VALF INFINITY
#define HUGE_VALL (long double)INFINITY

#define FP_ILOGB0 -2147483648
#define FP_ILOGBNAN -2147483648

#define FP_FAST_FMA 1
#define FP_FAST_FMAF 1
#define FP_FAST_FMAL 1

#define FP_INFINITE 0x01
#define FP_NAN 0x02
#define FP_NORMAL 0x04
#define FP_SUBNORMAL 0x08 // https://stackoverflow.com/questions/8341395/what-is-a-subnormal-floating-point-number
#define FP_ZERO 0x10

// CLASSIFICATION
#define fpclassify(arg)                                                                                                             \
  (arg == 0 ? FP_ZERO : (isnan(arg) ? FP_NAN : (isinf(arg) ? FP_INFINITE : ((arg > 0 ? arg : -arg) < 1 * 2 ^ (-126)) ? FP_SUBNORMAL \
                                                                                                                     : FP_NORMAL)))
#define isinf(arg) ((float)arg == INFINITY)
#define isfinite(arg) !isinf(arg)
#define isnan(arg) (arg != arg)
#define isnormal(arg) (!isnan(arg) && isfinite(arg) && (arg > 0 ? arg : -arg) >= 1 * 2 ^ (-126))
#define signbit(arg) (isnan(arg) ? 1 : arg < 0)

// COMPARISON
#define isgreater(x, y) (x > y)
#define isgreaterequal(x, y) (x >= y)
#define isless(x, y) (x < y)
#define islessequal(x, y) (x <= y)
#define islessgreater(x, y) (x < y || x > y)
#define isunordered(x, y) (isnan(x) || isnan(y))

// FUNCTIONS
double atan(double);
double cos(double);
double sin(double);
double tan(double);
double tanh(double);
double frexp(double, int *);
double modf(double, double *);
double ceil(double);
double fabs(double);
double floor(double);

double acos(double);
double asin(double);
double atan2(double, double);
double cosh(double);
double sinh(double);
double exp(double);
double ldexp(double, int);
double log(double);
double log10(double);
double pow(double, double);
double sqrt(double);
double fmod(double, double);

double infinity(void);
double nan(const char *);
double copysign(double, double);
double logb(double);
int ilogb(double);

double asinh(double);
double cbrt(double);
double nextafter(double, double);
double rint(double);
double scalbn(double, int);

double exp2(double);
double scalbln(double, long int);
double tgamma(double);
double nearbyint(double);
long int lrint(double);
long long int llrint(double);
double round(double);
long int lround(double);
long long int llround(double);
double trunc(double);
double remquo(double, double, int *);
double fdim(double, double);
double fmax(double, double);
double fmin(double, double);
double fma(double, double, double);

double log1p(double);
double expm1(double);

double acosh(double);
double atanh(double);
double remainder(double, double);
double gamma(double);
double lgamma(double);
double erf(double);
double erfc(double);
double log2(double);

double hypot(double, double);

// Single Precision
float atanf(float);
float cosf(float);
float sinf(float);
float tanf(float);
float tanhf(float);
float frexpf(float, int *);
float modff(float, float *);
float ceilf(float);
float fabsf(float);
float floorf(float);

float acosf(float);
float asinf(float);
float atan2f(float, float);
float coshf(float);
float sinhf(float);
float expf(float);
float ldexpf(float, int);
float logf(float);
float log10f(float);
float powf(float, float);
float sqrtf(float);
float fmodf(float, float);

float exp2f(float);
float scalblnf(float, long int);
float tgammaf(float);
float nearbyintf(float);
long int lrintf(float);
long long int llrintf(float);
float roundf(float);
long int lroundf(float);
long long int llroundf(float);
float truncf(float);
float remquof(float, float, int *);
float fdimf(float, float);
float fmaxf(float, float);
float fminf(float, float);
float fmaf(float, float, float);

float infinityf(void);
float nanf(const char *);
float copysignf(float, float);
float logbf(float);
int ilogbf(float);

float asinhf(float);
float cbrtf(float);
float nextafterf(float, float);
float rintf(float);
float scalbnf(float, int);
float log1pf(float);
float expm1f(float);

float acoshf(float);
float atanhf(float);
float remainderf(float, float);
float gammaf(float);
float lgammaf(float);
float erff(float);
float erfcf(float);
float log2f(float);
float hypotf(float, float);

// long double (js has no support for this but is like a define over the double variant)
long double atanl(long double);
long double cosl(long double);
long double sinl(long double);
long double tanl(long double);
long double tanhl(long double);
long double frexpl(long double, int *);
long double modfl(long double, long double *);
long double ceill(long double);
long double fabsl(long double);
long double floorl(long double);
long double log1pl(long double);
long double expm1l(long double);

long double acosl(long double);
long double asinl(long double);
long double atan2l(long double, long double);
long double coshl(long double);
long double sinhl(long double);
long double expl(long double);
long double ldexpl(long double, int);
long double logl(long double);
long double log10l(long double);
long double powl(long double, long double);
long double sqrtl(long double);
long double fmodl(long double, long double);
long double hypotl(long double, long double);

long double copysignl(long double, long double);
long double nanl(const char *);
int ilogbl(long double);
long double asinhl(long double);
long double cbrtl(long double);
long double nextafterl(long double, long double);
float nexttowardf(float, long double);
double nexttoward(double, long double);
long double nexttowardl(long double, long double);
long double logbl(long double);
long double log2l(long double);
long double rintl(long double);
long double scalbnl(long double, int);
long double exp2l(long double);
long double scalblnl(long double, long);
long double tgammal(long double);
long double nearbyintl(long double);
long int lrintl(long double);
long long int llrintl(long double);
long double roundl(long double);
long lroundl(long double);
long long int llroundl(long double);
long double truncl(long double);
long double remquol(long double, long double, int *);
long double fdiml(long double, long double);
long double fmaxl(long double, long double);
long double fminl(long double, long double);
long double fmal(long double, long double, long double);

long double acoshl(long double);
long double atanhl(long double);
long double remainderl(long double, long double);
long double lgammal(long double);
long double erfl(long double);
long double erfcl(long double);

#endif // _INC_MATH
