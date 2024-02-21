// https=//www.gnu.org/software/libc/manual/html_node/Floating-Point-Parameters.html
const FLT_RADIX = 2; // assume to always be 2 (is not for IBM 360 or derivatives)
const FP_ILOGB0 = 1 << 31; // -2147483648
const FP_ILOGBNAN = 1 << 31; // -2147483648
const INT_MAX = 2147483647;
const UINT_MAX = -1 >>> 0;
const INT_MIN = 1 << 31;
const UINT_MIN = 0;
const SMALLEST_DENORM = Math.pow(2, -1074);

// ENDIANNESS
function find_endianness() {
  var double_view = new Float64Array(1);
  var int_view = new Uint32Array(double_view.buffer);
  double_view[0] = -0;

  if (int_view[0] === 0) return 'little';
  else return 'big';
}
const ENDIANNESS = find_endianness();

// https=//blog.codefrau.net/2014/08/deconstructing-floats-frexp-and-ldexp.html
function frexp(value) {
  if (value === 0) return [value, 0];
  let data = new DataView(new ArrayBuffer(8));
  data.setFloat64(0, value);
  let bits = (data.getUint32(0) >>> 20) & 0x7ff;
  if (bits === 0) {
    // denormal
    data.setFloat64(0, value * Math.pow(2, 64)); // exp + 64
    bits = ((data.getUint32(0) >>> 20) & 0x7ff) - 64;
  }
  let exponent = bits - 1022;
  let mantissa = ldexp(value, -exponent);
  return [mantissa, exponent];
}
function ldexp(mantissa, exponent) {
  let steps = Math.min(3, Math.ceil(Math.abs(exponent) / 1023));
  let result = mantissa;
  for (let i = 0; i < steps; i++)
    result *= Math.pow(2, Math.floor((exponent + i) / steps));
  return result;
}

// Implementation of https://de.wikipedia.org/wiki/Fehlerfunktion
function erf(x) {
  const c = [
    1.26551223, 1.00002368, 0.37409196, 0.09678418, 0.18628806, 0.27886807,
    1.13520398, 1.48851587, 0.82215223, 0.17087277,
  ];

  const tau = (x) => {
    const t = 1 / (1 + 0.5 * Math.abs(x));

    exp_inner = 0;
    exp_inner += -c[0];
    t_mul = t;
    exp_inner += c[1] * t_mul;
    t_mul *= t;
    exp_inner += c[2] * t_mul;
    t_mul *= t;
    exp_inner += c[3] * t_mul;
    t_mul *= t;
    exp_inner += -c[4] * t_mul;
    t_mul *= t;
    exp_inner += c[5] * t_mul;
    t_mul *= t;
    exp_inner += -c[6] * t_mul;
    t_mul *= t;
    exp_inner += c[7] * t_mul;
    t_mul *= t;
    exp_inner += -c[8] * t_mul;
    t_mul *= t;
    exp_inner += c[9] * t_mul;

    return t * Math.exp(-(x * x) + exp_inner);
  };

  if (x >= 0) {
    return 1 - tau(x);
  } else {
    return tau(-x) - 1;
  }
}

class MathJs {
  #reset() {
    this.__wasm = undefined;
    this.__memory = undefined;
    this.__fenv = new FeEnvJs();
    this.__stdlib = new StdlibJs();
  }

  constructor() {
    this.#reset();
  }

  init(wasm) {
    this.__wasm = wasm;
    this.__memory = wasm.instance.exports.memory;
  }

  // Double precision

  // misc
  fabs = Math.abs;
  fmod = (x, y) => x % y;
  remainder = (x, y) => {
    // https://en.cppreference.com/w/c/numeric/math/remainder
    if (isNaN(x) || isNaN(y)) return NaN;
    if (!isFinite(x) || y === 0) {
      this.__wasm.instance.exports.feraiseexcept(fe_exception_flag.FE_INVALID);
      return NaN;
    }

    return x % y;
  };
  remquo = (x, y, quo_ptr) => {
    // https://en.cppreference.com/w/c/numeric/math/remquo
    if (isNaN(x) || isNaN(y)) return NaN;
    if (!isFinite(x) || y === 0) {
      this.__wasm.instance.exports.feraiseexcept(fe_exception_flag.FE_INVALID);
      return NaN;
    }

    let remainder = x % y;

    let sign = 0;
    if (remainder === 0) {
      if (x !== 0) sign = Math.sign(x);
      else if (x === -0) sign = -1;
      else sign = 1;
    } else if (remainder < 0) sign = -1;
    else sign = 1;

    if (x === 0 && y === 0)
      this.__wasm.instance.exports.feraiseexcept(fe_exception_flag.FE_INVALID);
    if (y === 0) {
      this.__wasm.instance.exports.feraiseexcept(
        fe_exception_flag.FE_DIVBYZERO
      );
    }

    let quo = Math.floor(x / y);
    quo &= 0x7fffffff;

    let buf = new Uint32Array(this.__memory.buffer, quo_ptr, 1);
    buf.set([sign * quo]);
    return remainder;
  };
  fma = (x, y, z) => x * y + z;
  fmax = Math.max;
  fmin = Math.min;
  fdim = (x, y) => (x > y ? x - y : 0); // https://en.cppreference.com/w/c/numeric/math/fdim
  infinity = () => Infinity;
  nan = (content_ptr) => {
    const content_str = cstr_by_ptr(this.__memory.buffer, content_ptr);
    const num = parseInt(content_str);

    if (num <= 0 || num > 0xffffffff || num === NaN) {
      return NaN;
    } else {
      let nan = new Uint32Array(2);
      let buf_double = new Float64Array(nan.buffer, 0, 1);

      if (ENDIANNESS === 'big') {
        nan[0] = 0x80000000;
        nan[0] |= 0x7ff00000;
        // nan[0] |= 0x000fffff & num;
        nan[1] |= num;
      } else if (ENDIANNESS === 'little') {
        nan[1] = 0x80000000;
        nan[1] |= 0x7ff00000;
        // nan[1] |= 0x000fffff & num;
        nan[0] |= num;
      } else {
        throw new Error(
          'Could not determine if numbers are little or big ENDIANNESS'
        );
      }
      return buf_double[0];
    }
  };

  // exponential
  exp = Math.exp;
  exp2 = (x) => Math.pow(2, x);
  expm1 = Math.expm1;
  log = Math.log;
  log2 = Math.log2 || ((x) => Math.log(x) * Math.LOG2E);
  log10 = Math.log10;
  log1p = Math.log1p;
  ilogb = (x) => {
    // https://en.cppreference.com/w/c/numeric/math/ilogb
    if (x === 0) return FP_ILOGB0;
    else if (!isFinite(x)) return INT_MAX;
    else if (isNaN(x)) return FP_ILOGBNAN;

    return Math.floor(this.logb(x));
  };
  logb = (x) => Math.log(x) / Math.log(FLT_RADIX);

  // power
  pow = Math.pow;
  sqrt = (x) => {
    if (x < 0) {
      this.__wasm.instance.exports.feraiseexcept(fe_exception_flag.FE_INVALID);
    }
    return Math.sqrt(x);
  };
  cbrt = Math.cbrt;
  hypot = Math.hypot;

  //trigonometric
  sin = Math.sin;
  cos = Math.cos;
  tan = Math.tan;
  asin = Math.asin;
  acos = Math.acos;
  atan = Math.atan;
  atan2 = Math.atan2;

  // hyperbolic
  sinh = Math.sinh;
  cosh = Math.cosh;
  tanh = Math.tanh;
  asinh = Math.asinh;
  acosh = Math.acosh;
  atanh = Math.atanh;

  // error/gamma
  erf = erf;
  erfc = (x) => 1 - this.erf(x);
  lgamma = (x) => {
    throw new ERROR('MATH GAMMA functions not implemented.');
  };
  tgamma = (x) => {
    throw new ERROR('MATH GAMMA functions not implemented.');
  };

  // floating point to int
  ceil = Math.ceil;
  floor = Math.floor;
  trunc = Math.trunc;
  round = Math.round;
  lround = Math.round;
  llround = Math.round;
  nearbyint = (x) => {
    let rounding_mode_before = this.__fenv.___fe_rounding_mode;
    this.__fenv.___fe_rounding_mode = FeRoundingModes.FE_TONEAREST;
    const res = this.rint(x);
    this.__fenv.___fe_rounding_mode = rounding_mode_before;
    return res;
  };
  rint = (x) => {
    switch (this.__fenv.___fe_rounding_mode) {
      case FeRoundingModes.FE_DOWNWARD:
        return Math.floor(x);
      case FeRoundingModes.FE_UPWARD:
        return Math.ceil(x);
      case FeRoundingModes.FE_TOWARDZERO:
        return x >= 0 ? Math.floor(x) : Math.ceil(x);
      case FeRoundingModes.FE_TONEAREST:
        return Math.floor(x) + 0.5 <= x ? Math.ceil(x) : Math.floor(x);
    }
  };
  lrint = this.rint;
  llrint = this.rint;

  // floating point manipulation
  frexp = (x, exponent_ptr) => {
    const [mantissa, exponent] = frexp(x);
    let buf = new Uint32Array(this.__memory.buffer, exponent_ptr, 1);
    buf.set([exponent]);
    return mantissa;
  };
  ldexp = (x, y) => x * Math.pow(2, y);
  modf = (x, int_part_ptr) => {
    // TODO: This implementation is right for small values but is not C compliant
    const int_part = Math.floor(x);
    let buf = new Uint32Array(this.__memory.buffer, int_part_ptr, 1);
    buf.set([int_part]);
    this.__wasm.instance.exports.feraiseexcept(fe_exception_flag.FE_INEXACT);
    return x - int_part;
  };
  scalbln = (x, y) => x * Math.pow(FLT_RADIX, y);
  scalbn = this.scalbln;
  nextafter = (from, to) => {
    // https://github.com/scijs/nextafter (MIT)
    // https://www.npmjs.com/package/double-bits (MIT)
    if (isNaN(from) || isNaN(to)) return NaN;
    else if (from === to) return from;
    else if (from === 0) return to < 0 ? -SMALLEST_DENORM : SMALLEST_DENORM;

    double_view[0] = from;

    var lo = ENDIANNESS === 'little' ? float_view[0] : float_view[1];
    var hi = ENDIANNESS === 'little' ? float_view[1] : float_view[0];

    if (y > x === x > 0) {
      if (lo === UINT_MAX) {
        hi += 1;
        lo = 0;
      } else {
        lo += 1;
      }
    } else {
      if (lo === 0) {
        lo = UINT_MAX;
        hi -= 1;
      } else {
        lo -= 1;
      }
    }

    // TODO: raise fe exceptions

    return double_view[0];
  };
  nexttoward = this.nextafter;
  copysign = (x, y) => Math.abs(x) * Math.sign(y);

  // Single precision
  atanf = this.atan;
  cosf = this.cos;
  sinf = this.sin;
  tanf = this.tan;
  tanhf = this.tanh;
  frexpf = this.frexp;
  modff = this.modf;
  ceilf = this.ceil;
  fabsf = this.fabs;
  floorf = this.floor;

  acosf = this.acos;
  atan2f = this.atan2;
  coshf = this.cosh;
  sinhf = this.sinh;
  expf = this.exp;
  ldexpf = this.ldexp;
  logf = this.log;
  log10f = this.log10;
  powf = this.pow;
  sqrtf = this.sqrt;
  fmodf = this.fmod;

  exp2f = this.exp2;
  scalblnf = this.scalbln;
  tgammaf = this.tgamma;
  nearbyintf = this.nearbyint;
  lrintf = this.lrint;
  llrintf = this.llrint;
  roundf = this.round;
  lroundf = this.lround;
  llroundf = this.llround;
  truncf = this.trunc;
  remquof = this.remquo;
  fdimf = this.fdim;
  fmaxf = this.fmax;
  fminf = this.fmin;
  fmaf = this.fma;

  infinityf = this.infinity;
  nanf = this.nan;
  copysignf = this.copysign;
  logbf = this.logb;
  ilogbf = this.ilogb;

  asinhf = this.asinh;
  cbrtf = this.cbrt;
  nextafterf = this.nextafter;
  rintf = this.rint;
  scalbnf = this.scalbn;
  log1pf = this.log1p;
  expm1f = this.expm1;

  acoshf = this.acosh;
  atanhf = this.atanh;
  remainderf = this.remainder;
  lgammaf = this.lgamma;
  erff = this.erf;
  erfcf = this.erfc;
  log2f = this.log2;
  hypotf = this.hypot;

  // long double precision
  // (WARNING: only for completness, is not more preccise)

  atanl = this.atan;
  cosl = this.cos;
  sinl = this.sin;
  tanl = this.tan;
  tanhl = this.tanh;
  frexpl = this.frexp;
  modfl = this.modf;
  ceill = this.ceil;
  fabsl = this.fabs;
  floorl = this.floor;

  acosl = this.acos;
  atan2l = this.atan2;
  coshl = this.cosh;
  sinhl = this.sinh;
  expl = this.exp;
  ldexpl = this.ldexp;
  logl = this.log;
  log10l = this.log10;
  powl = this.pow;
  sqrtl = this.sqrt;
  fmodl = this.fmod;

  exp2l = this.exp2;
  scalblnl = this.scalbln;
  tgammal = this.tgamma;
  nearbyintl = this.nearbyint;
  lrintl = this.lrint;
  llrintl = this.llrint;
  roundl = this.round;
  lroundl = this.lround;
  llroundl = this.llround;
  truncl = this.trunc;
  remquol = this.remquo;
  fdiml = this.fdim;
  fmaxl = this.fmax;
  fminl = this.fmin;
  fmal = this.fma;

  infinityl = this.infinity;
  nanl = this.nan;
  copysignl = this.copysign;
  logbl = this.logb;
  ilogbl = this.ilogb;

  asinhl = this.asinh;
  cbrtl = this.cbrt;
  nextafterl = this.nextafter;
  rintl = this.rint;
  scalbnl = this.scalbn;
  log1pl = this.log1p;
  expm1l = this.expm1;

  acoshl = this.acosh;
  atanhl = this.atanh;
  remainderl = this.remainder;
  lgammal = this.lgamma;
  erfl = this.erf;
  erfcl = this.erfc;
  log2l = this.log2;
  hypotl = this.hypot;
}
