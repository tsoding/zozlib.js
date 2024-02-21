const RAND_MAX = INT_MAX;

// https://stackoverflow.com/questions/521295/seeding-the-random-number-generator-in-javascript
function splitmix32(a) {
  return function () {
    a |= 0;
    a = (a + 0x9e3779b9) | 0;
    var t = a ^ (a >>> 16);
    t = Math.imul(t, 0x21f0aaad);
    t = t ^ (t >>> 15);
    t = Math.imul(t, 0x735a2d97);
    return ((t = t ^ (t >>> 15)) >>> 0) / 4294967296;
  };
}

function cstr_by_ptr_len(mem_buffer, ptr, str_len) {
  const bytes = new Uint8Array(mem_buffer, ptr, str_len);
  return new TextDecoder().decode(bytes);
}

class ExitWasmException extends Error {
  constructor() {
    super('This is a exception thrown to exist wasm');
    this.name = this.constructor.name;
  }
}

class StdlibJs {
  #reset() {
    this.__memory = undefined;
    this.__fn_table = undefined;

    // STRING TO NUMBER
    this.__had_dot = undefined;
    this.__had_e = undefined;
    this.__had_x = undefined;

    this.___had_helpers = {};
    this.___had_helpers.is_second_dot = (x) => {
      if (str[i] === '.') {
        if (!this.__had_dot) {
          this.__had_dot = true;
          return false;
        } else {
          return true;
        }
      }
      return false;
    };
    this.___had_helpers.is_second_e = (x) => {
      if (str[i] === 'e' || str[i] === 'E') {
        if (!this.__had_e) {
          this.__had_e = true;
          return false;
        } else {
          return true;
        }
      }
      return false;
    };
    this.___had_helpers.is_second_x = (x) => {
      if (str[i] === 'x' || str[i] === 'X') {
        if (!this.__had_x) {
          this.__had_x = true;
          return false;
        } else {
          return true;
        }
      }
      return false;
    };

    // RANDOM
    this.__current_random_generator = splitmix32(1);

    // ENVIRONMENT
    this.__ctx = undefined;
    this.__change_running_fn = undefined;
    this.__wasm = undefined;
    this.__at_exit_fns = [];
    this.__at_quick_exit_fns = [];
  }

  constructor() {
    this.#reset();
  }

  init(ctx, wasm, change_running_fn) {
    this.__ctx = ctx;
    this.__wasm = wasm;
    this.__memory = wasm.instance.exports.memory;
    this.__fn_table = wasm.instance.exports.__indirect_function_table;
    this.__change_running_fn = change_running_fn;
  }
  handle_exit() {
    for (let i = 0; i < this.__at_exit_fns.length; i++) {
      this.__at_exit_fns[this.__at_exit_fns.length - 1 - i]();
    }
  }

  // STRING TO NUMBER
  atof = (str_ptr) => {
    var str = cstr_by_ptr(this.__memory.buffer, str_ptr);
    return parseFloat(str);
  };
  atoi = (str_ptr) => {
    var str = cstr_by_ptr(this.__memory.buffer, str_ptr);
    return parseInt(str);
  };
  atol = this.atoi;
  atoll = this.atoi;

  _next_is_num(i, str) {
    return (
      (str[i] >= '0' && str[i] <= '9') ||
      (str[i] === '-' &&
        str[i + 1] !== undefined &&
        !isNaN(parseInt(str[i + 1], 10)))
    );
  }
  _parse_num(start_i, str) {
    let i = start_i;

    // eat until first non number char
    this.__had_dot = false;
    this.__had_e = false;
    while (
      str[i] !== undefined &&
      (!isNaN(parseInt(str[i], 10)) ||
        str[i] === '-' ||
        str[i] === '.' ||
        str[i].toLowerCase() === 'e') &&
      !this.___had_helpers.is_second_dot(str[i]) &&
      !this.___had_helpers.is_second_e(str[i])
    ) {
      i++;
    }

    return [parseFloat(str), i];
  }

  _next_is_nan(i, str) {
    return (
      (str[i] === '-' &&
        str[i + 1] !== undefined &&
        str[i + 1].toLowerCase() === 'n') ||
      str[i].toLowerCase() === 'n'
    );
  }
  _parse_nan(start_i, str, is_float) {
    let i = start_i;

    let is_negative = false;
    if (str[i] === '-') {
      is_negative = true;
      i++;
    }

    i++;
    const valid2 = str[i] === 'A' || str[i] === 'a';
    i++;
    const valid3 = str[i] === 'N' || str[i] === 'n';
    i++;

    if (!valid2 || !valid3) {
      i -= 3;
      return [0, i];
    } else if (str[i] === '(') {
      i++;
      const substr = str.substring(i);
      const num = parseInt(substr);

      // eat until first non number char
      while (!isNaN(parseInt(str[i], 10))) {
        i++;
      }

      if (str[i] === ')') {
        i++;
        if (num <= 0 || num > 0xffffffff) {
          return [is_negative ? -NaN : NaN, i];
        } else {
          if (is_float) {
            let nan = new Uint32Array(1);
            let buf_float = new Float32Array(nan.buffer, 0, 1);

            nan[0] = 0x7fc00000;
            nan[0] |= 0x3fffff & num;

            return [is_negative ? -buf_float[0] : buf_float[0], i];
          } else {
            let nan = new Uint32Array(2);
            let buf_double = new Float64Array(nan.buffer, 0, 1);

            if (ENDIANNESS === 'big') {
              nan[0] = 0x7ff80000;
              // nan[0] |= 0x000fffff & num;
              nan[1] |= num;
            } else if (ENDIANNESS === 'little') {
              nan[1] = 0x7ff80000;
              // nan[1] |= 0x000fffff & num;
              nan[0] |= num;
            }
            return [is_negative ? -buf_double[0] : buf_double[0], i];
          }
        }
      } else {
        return [0, start_i];
      }
    } else {
      return [is_negative ? -NaN : NaN, i];
    }
  }

  _next_is_inf(i, str) {
    return (
      (str[i] === '-' &&
        str[i + 1] !== undefined &&
        str[i + 1].toLowerCase() === 'i') ||
      str[i].toLowerCase() === 'i'
    );
  }
  _parse_inf(start_i, str) {
    let i = start_i;

    let is_negative = false;
    if (str[i] === '-') {
      is_negative = true;
      i++;
    }

    i++;
    const find = 'infinity';
    let j = 1;
    while (
      i < str.length &&
      j < find.length &&
      str[i].toLowerCase() === find[j].toLowerCase()
    ) {
      i++;
      j++;
    }

    if (j >= 3 && j <= 7) {
      i -= j - 3;
      return [is_negative ? -Infinity : Infinity, i];
    } else if (j === 8) {
      return [is_negative ? -Infinity : Infinity, i];
    } else {
      i -= j;
      return [0, i];
    }
  }

  _strtodf(str_ptr, end_ptr, is_float) {
    let str = cstr_by_ptr(this.__memory.buffer, str_ptr);
    let i = 0;

    // eat whitespace
    while (str.length < i && ' \t\n\r\v'.indexOf(str[i]) > -1) {
      i++;
    }

    let res;
    if (str[i] === undefined) {
      res = 0;
    } else if (this._next_is_num(i, str)) {
      [res, i] = this._parse_num(i, str);
    } else if (this._next_is_nan(i, str)) {
      [res, i] = this._parse_nan(i, str, is_float);
    } else if (this._next_is_inf(i, str)) {
      [res, i] = this._parse_inf(i, str);
    } else {
      res = 0;
    }

    // eat whitespace
    while (str.length < i && ' \t\n\r\v'.indexOf(str[i]) > -1) {
      i++;
    }

    var buf = new Uint32Array(this.__memory.buffer, end_ptr);
    buf.set([str_ptr + i]); // do not find
    return res;
  }

  strtod = (str_ptr, end_ptr) => this._strtodf(str_ptr, end_ptr, false);
  strtof = (str_ptr, end_ptr) => this._strtodf(str_ptr, end_ptr, true);
  strtol = (str_ptr, end_ptr, base) => {
    var str = cstr_by_ptr(this.__memory.buffer, str_ptr);

    var i = 0;

    // eat whitespace
    while (str.length < i && ' \t\n\r\v'.indexOf(str[i]) > -1) {
      i++;
    }

    // eat until first non number char
    this.__had_x = false;
    while (
      !isNaN(parseInt(str[i], base)) &&
      !this.___had_helpers.is_second_x(str[i]) // TODO: x is accepted for bases other than 0 and 16
    ) {
      i++;
    }

    var buf = Uint32Array(this.__memory.buffer, end_ptr);
    buf.set([str_ptr + i]); // do not find
    return parseInt(str, base);
  };
  strtoll = this.strtol;
  strtoul = this.strtol;
  strtoull = this.strtol;

  // RANDOM
  rand = () => {
    return this.__current_random_generator() * RAND_MAX;
  };
  srand = (seed) => {
    this.__current_random_generator = splitmix32(seed);
  };

  // MEMORY ALLOCATOR
  _heap_start = () => {
    return this.__wasm.instance.exports.__heap_base;
  };
  _grow_memory = (page_size) => {
    var first_free_mem_index = this.__memory.buffer.byteLength;
    this.__memory.grow(page_size);
    return first_free_mem_index;
  };

  // ENVIRONMENT
  abort = () => {
    this.__change_running_fn(() => {
      const w = this.__ctx.canvas.width;
      const h = this.__ctx.canvas.height;
      const old_style = this.__ctx.fillStyle;

      this.__ctx.fillStyle = '#fa4141';
      this.__ctx.fillRect(0, 0, w, h);

      const fontSize = 20;
      const text = `Program aborted`;
      this.__ctx.font = `${fontSize}px grixel`;
      this.__ctx.fillStyle = 'black';
      const textWidth = this.__ctx.measureText(text);
      this.__ctx.fillText(text, w / 2 - textWidth.width / 2, h / 2 + fontSize);

      this.__ctx.fillStyle = old_style;
    });
    throw new ExitWasmException(); // exit wasm
  };
  exit = (status) => {
    for (let i = 0; i < this.__at_exit_fns.length; i++) {
      this.__at_exit_fns[this.__at_exit_fns.length - 1 - i]();
    }

    this.__change_running_fn(() => {
      const w = this.__ctx.canvas.width;
      const h = this.__ctx.canvas.height;
      const old_style = this.__ctx.fillStyle;

      this.__ctx.fillStyle = status == 0 ? '#37bd3b' : '#fa4141';
      this.__ctx.fillRect(0, 0, w, h);

      const fontSize = 20;
      const text = `Exited with status ${status}`;
      this.__ctx.font = `${fontSize}px grixel`;
      this.__ctx.fillStyle = 'black';
      const textWidth = this.__ctx.measureText(text);
      this.__ctx.fillText(text, w / 2 - textWidth.width / 2, h / 2 + fontSize);

      this.__ctx.fillStyle = old_style;
    });
    throw new ExitWasmException(); // exit wasm
  };
  quick_exit = (status) => {
    for (let i = 0; i < this.__at_quick_exit_fn.length; i++) {
      this.__at_quick_exit_fn[this.__at_quick_exit_fn.length - 1 - i]();
    }

    this.__change_running_fn(() => {
      const w = this.__ctx.canvas.width;
      const h = this.__ctx.canvas.height;
      const old_style = this.__ctx.fillStyle;

      this.__ctx.fillStyle = status == 0 ? '#37bd3b' : '#fa4141';
      this.__ctx.fillRect(0, 0, w, h);

      const fontSize = 20;
      const text = `Exited with status ${status}`;
      this.__ctx.font = `${fontSize}px grixel`;
      this.__ctx.fillStyle = 'black';
      const textWidth = this.__ctx.measureText(text);
      this.__ctx.fillText(text, w / 2 - textWidth.width / 2, h / 2 + fontSize);

      this.__ctx.fillStyle = old_style;
    });
    throw new ExitWasmException(); // exit wasm
  };
  atexit = (fn_ptr) => {
    try {
      this.__at_exit_fns.push(this.__fn_table.get(fn_ptr));
    } catch (e) {
      return 1;
    }
    return 0;
  };
  at_quick_exit = (fn_ptr) => {
    try {
      this.__at_quick_exit_fns.push(this.__fn_table.get(fn_ptr));
    } catch (e) {
      return 1;
    }
    return 0;
  };

  // INTEGER ARITHMETICS
  abs = Math.abs;
  labs = this.abs;
  llabs = this.abs;

  // MULTIBYTE CHARACTERS (UTF-ONLY (LC_CTYPE=UTF-8))
  mblen = (mbc_ptr, max) => {
    if (mbc_ptr == 0) return 0; // not state dependant

    let str = cstr_by_ptr_len(mbc_ptr, max);
    let first_char = str[0];
    return new Blob([first_char]).size;
  };
  mbtowc = (wc_ptr, mbc_ptr, max) => {
    if (mbc_ptr == 0) return 0; // not state dependant

    let str = cstr_by_ptr_len(mbc_ptr, max);

    if (wc_ptr != 0) {
      // assume sizeof(wchar_t) == 4
      let buf = new Int32Array(this.__memory.buffer, wc_ptr, max);
      buf[0] = str.codePointAt(0);
    }

    return new Blob([str[0]]).size;
  };
  wctomb = (mbc_ptr, wc) => {
    if (mbc_ptr == 0) return 0; // not state dependant

    let mbc;
    try {
      mbc = String.fromCodePoint(wc);
    } catch (e) {
      return -1; // handles invalid code points
    }

    let encoded = new TextEncoder().encode(mbc);
    let buf = new Int8Array(this.__memory.buffer, mbc_ptr, encoded.byteLength);

    for (i = 0; i < encoded.byteLength; i++) {
      buf[i] = encoded[i];
    }

    return encoded.length;
  };

  mbstowcs = (dest_ptr, src_ptr, max) => {
    const src_buf = new Int8Array(this.__memory.buffer, src_ptr, max);
    const str = new TextDecoder('utf-8').decode(src_buf);

    let dest_buf = new Int32Array(this.__memory.buffer, dest_ptr, max);

    for (let i = 0; i < Math.min(str.length, max); i++) {
      dest_buf[i] = str.codePointAt(i);
    }
    if (str.length < max) {
      dest_buf[str.length] = 0; // add '\0';
    }

    return Math.min(str.length, max);
  };
  wcstombs = (dest_ptr, src_ptr, max) => {
    const src_buf = new Int32Array(this.__memory.buffer, src_ptr, max);
    const str = new TextDecoder('utf-8').decode(src_buf);

    let cur_len = 0;

    let last = src_buf[0];
    for (let i = 0; i < max && last != 0; i++) {
      last = String.fromCodePoint(src_buf[i]);
      let len = this.wctomb(dest_ptr + cur_len);
      if (len < 0) return -1;
      cur_len += len;
    }

    return cur_len;
  };
}
