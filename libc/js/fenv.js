const FeRoundingMode = {
  FE_TONEAREST: 0,
  FE_DOWNWARD: 1,
  FE_UPWARD: 2,
  FE_TOWARDZERO: 3,
};

const fe_exception_flag = {
  FE_DIVBYZERO: 0x01, // Pole error: division by zero, or some other asymptotically infinite result (from finite arguments).
  FE_INEXACT: 0x02, // Inexact: the result is not exact.
  FE_INVALID: 0x04, // Domain error: At least one of the arguments is a value for which the function is not defined.
  FE_OVERFLOW: 0x08, // Overflow range error: The result is too large in magnitude to be represented as a value of the return type.
  FE_UNDERFLOW: 0x10, // Underflow range error: The result is too small in magnitude to be represented as a value of the return type.
  FE_ALL_EXCEPT: 0x1f, // All exceptions (selects all of the exceptions supported by the implementation).
};

// https://en.cppreference.com/w/c/numeric/fenv
class FeEnvJs {
  #reset() {
    this.__memory = undefined;

    this.__double_view = new Float64Array(1);
    this.__float_view = new Float32Array(this.__double_view.buffer);

    this.___fe_rounding_mode_default = FeRoundingMode.FE_DOWNWARD;
    this.___fe_rounding_mode = this.___fe_rounding_mode_default;

    this;
  }

  constructor() {
    this.#reset();
  }

  _js_fesetround(round) {
    this.___fe_rounding_mode = round;
  }
}
