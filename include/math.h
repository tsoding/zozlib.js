// Phony math.h. Since we are compiling with --no-standard-libraries we raymath.h can't find math.h.
// But it only needs it for few function definitions. So we've put those definitions here.
#ifndef MATH_H_
#define MATH_H_
float floorf(float);
float fabsf(float);
double fabs(double);
float fmaxf(float, float);
float fminf(float, float);
float sqrtf(float);
float atan2f(float, float);
float cosf(float);
float sinf(float);
float acosf(float);
float asinf(float);
double tan(double);
#endif // MATH_H_
