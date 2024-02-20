function cstrlen(mem, ptr) {
    let len = 0;
    while (mem[ptr] != 0) {
        len++;
        ptr++;
    }
    return len;
}

function cstr_by_ptr(mem_buffer, ptr) {
    const mem = new Uint8Array(mem_buffer);
    const len = cstrlen(mem, ptr);
    const bytes = new Uint8Array(mem_buffer, ptr, len);
    return new TextDecoder().decode(bytes);
}

function color_hex_unpacked(r, g, b, a) {
    r = r.toString(16).padStart(2, "0");
    g = g.toString(16).padStart(2, "0");
    b = b.toString(16).padStart(2, "0");
    a = a.toString(16).padStart(2, "0");
    return "#" + r + g + b + a;
}

function color_hex(color) {
    const r = ((color >> (0 * 8)) & 0xff).toString(16).padStart(2, "0");
    const g = ((color >> (1 * 8)) & 0xff).toString(16).padStart(2, "0");
    const b = ((color >> (2 * 8)) & 0xff).toString(16).padStart(2, "0");
    const a = ((color >> (3 * 8)) & 0xff).toString(16).padStart(2, "0");
    return "#" + r + g + b + a;
}

function getColorFromMemory(buffer, color_ptr) {
    const [r, g, b, a] = new Uint8Array(buffer, color_ptr, 4);
    return color_hex_unpacked(r, g, b, a);
}

function cstr_fmt_by_ptr(mem_buffer, fmt_ptr, arg_ptr) {
    const fmt = cstr_by_ptr(mem_buffer, fmt_ptr);

    // VarArgs Chunk
    let args = mem_buffer.slice(arg_ptr);
    let result = "";
    let fmt_buffer = fmt.split("");
    let fmt_cur = 0;

    let pad_width,
        precision,
        pad_with_zero,
        pad_with_space,
        justify_right,
        pre_with_polarity,
        pre_with_space,
        pre_with_format,
        capitalize,
        precise,
        bit_size,
        radix;

    reset_state();

    function reset_state() {
        bit_size = 32;
        pad_width = 0;
        precision = 0;
        radix = 10;
        pad_with_zero = false;
        pad_with_space = false;
        justify_right = true;

        pre_with_polarity = false;
        pre_with_space = false;
        pre_with_format = false;

        capitalize = false;
        precise = false;
    }

    function parse_num(cursor) {
        let width_end = cursor;
        while (fmt_buffer[width_end]) {
            if (/\d/.test(fmt_buffer[width_end])) width_end++;
            else break;
        }
        let num = fmt_buffer.splice(cursor, width_end - cursor).join("");
        return parseInt(num);
    }

    // Grab the view of args based on specifier and shift the args
    function shift_args(view) {
        args = args.slice(view.BYTES_PER_ELEMENT);
        return view.at(0);
    }

    while (fmt_cur < fmt_buffer.length) {
        if (fmt_buffer[fmt_cur] !== "%") {
            // Normal character, copy it to the temp string
            const str = fmt_buffer[fmt_cur++];
            result += str;
            continue;
        }

        // Peek only next character and splice the modifiers.
        // So we can always simply look one char ahead
        const peek_idx = fmt_cur + 1;
        const peek_char = fmt_buffer[peek_idx];
        switch (peek_char) {
            case undefined: {
                fmt_cur++;
                break;
            }

            case "%": {
                result += "%";
                fmt_cur += 2;
                break;
            }

            case "+": {
                pre_with_polarity = true;
                fmt_buffer.splice(peek_idx, 1);
                break;
            }

            case "-": {
                justify_right = false;
                fmt_buffer.splice(peek_idx, 1);
                break;
            }

            case " ": {
                pre_with_space = !pre_with_polarity;
                fmt_buffer.splice(peek_idx, 1);
                break;
            }

            case ".": {
                precise = true;
                fmt_buffer.splice(peek_idx, 1);
                break;
            }

            case "#": {
                pre_with_format = true;
                fmt_buffer.splice(peek_idx, 1);
                break;
            }

            case "0": {
                if (precise) {
                    precision = parse_num(peek_idx);
                } else {
                    pad_with_zero = true;
                    pad_width = parse_num(peek_idx);
                }
                break;
            }

            case "1":
            case "2":
            case "3":
            case "4":
            case "5":
            case "6":
            case "7":
            case "8":
            case "9": {
                if (precise) {
                    precision = parse_num(peek_idx);
                } else {
                    pad_with_space = true;
                    pad_width = parse_num(peek_idx);
                }
                break;
            }

            // Modifiers
            case "l": {
                bit_size = bit_size === 32 ? 64 : 32;
                fmt_buffer.splice(peek_idx, 1);
            }

            // In JS there are no number below 32 bit, so we are ignoring
            // also clang bakes the overflown and underflown values as it is
            case "h": {
                fmt_buffer.splice(peek_idx, 1);
                break;
            }

            case "c": {
                const char_code = shift_args(new Uint32Array(args));
                let str = String.fromCharCode(char_code);
                let pad_char;

                if (pad_with_zero || pad_with_space) pad_char = " ";
                else pad_char = "";

                if (justify_right) str = str.padStart(pad_width, pad_char);
                else str = str.padEnd(pad_width, pad_char);

                result += str;
                fmt_cur += 2;
                reset_state();
                break;
            }

            case "s": {
                const str_ptr = shift_args(new Uint32Array(args));
                let str = cstr_by_ptr(mem_buffer, str_ptr);
                let pad_char;

                if (pad_with_zero || pad_with_space) pad_char = " ";
                else pad_char = "";

                if (justify_right) str = str.padStart(pad_width, pad_char);
                else str = str.padEnd(pad_width, pad_char);

                if (precise) str = str.substring(0, precision);

                result += str;
                fmt_cur += 2;
                reset_state();
                break;
            }

            case "o":
            case "x":
            case "X":
            case "u": {
                let num;
                let pad_char, pre_char;

                if (peek_char === "o") {
                    radix = 8;
                } else if (peek_char === "x") {
                    radix = 16;
                } else if (peek_char === "X") {
                    radix = 16;
                    capitalize = true;
                } else {
                    console.error("radix unreachable");
                }

                if (bit_size === 32) {
                    num = shift_args(new Uint32Array(args));
                } else {
                    unaligned = args.byteLength % 8;
                    args.slice(unaligned);
                    num = shift_args(new BigUint64Array(args));
                }

                if (pad_with_zero) pad_char = "0";
                else if (pad_with_space) pad_char = " ";
                else pad_char = "";

                if (pre_with_format && radix === 8) pre_char = "0";
                else if (pre_with_format && radix === 16) pre_char = "0x";
                else pre_char = "";

                if (precise) {
                    pad_char = " ";
                    num = num.toString(radix).padStart(precision, "0");
                }

                if (justify_right) {
                    num = num.toString(radix).padStart(pad_width, pad_char);
                } else {
                    pad_char = " ";
                    num = num.toString(radix).padEnd(pad_width, pad_char);
                }

                num = pre_char + num;

                if (capitalize) num = num.toUpperCase();

                result += num;
                fmt_cur += 2;
                reset_state();
                break;
            }

            case "i":
            case "d": {
                let num;
                let pad_char, pre_char;

                if (bit_size === 32) {
                    num = shift_args(new Int32Array(args));
                } else {
                    unaligned = args.byteLength % 8;
                    args = args.slice(unaligned);
                    num = shift_args(new BigInt64Array(args));
                }

                if (pad_with_zero) pad_char = "0";
                else if (pad_with_space) pad_char = " ";
                else pad_char = "";

                if (pre_with_polarity && num > 0) pre_char = "+";
                else if (pre_with_space && num > 0) pre_char = " ";
                else pre_char = "";

                if (precise) {
                    pad_char = " ";
                    num = num.toString().padStart(precision, "0");
                }

                if (justify_right) {
                    num = num.toString().padStart(pad_width, pad_char);
                } else {
                    pad_char = " ";
                    num = num.toString().padEnd(pad_width, pad_char);
                }

                num = pre_char + num;

                result += num;
                fmt_cur += 2;
                reset_state();
                break;
            }

            case "E": {
                capitalize = true;
            }
            case "e": {
                // Align Bytes by 8
                unaligned = args.byteLength % 8;
                args = args.slice(unaligned);

                let num = shift_args(new Float64Array(args));
                let pad_char, pre_char;

                if (pad_with_zero) pad_char = "0";
                else if (pad_with_space) pad_char = " ";
                else pad_char = "";

                if (pre_with_polarity && num > 0) pre_char = "+";
                else if (pre_with_space && num > 0) pre_char = " ";
                else pre_char = "";

                if (precise) num = num.toExponential(precision);
                else num = num.toExponential(6);

                if (justify_right) {
                    num = num.toString().padStart(pad_width, pad_char);
                } else {
                    pad_char = " ";
                    num = num.toString().padEnd(pad_width, pad_char);
                }

                if (capitalize) num = num.toUpperCase();

                num = pre_char + num;

                result += num;
                fmt_cur += 2;
                reset_state();
                break;
            }
            case "F":
            case "G": {
                capitalize = true;
            }
            case "g":
            case "f": {
                // Align Bytes by 8
                unaligned = args.byteLength % 8;
                args = args.slice(unaligned);

                let num = shift_args(new Float64Array(args));
                let pad_char, pre_char;

                if (pad_with_zero) pad_char = "0";
                else if (pad_with_space) pad_char = " ";
                else pad_char = "";

                if (pre_with_polarity && num > 0) pre_char = "+";
                else if (pre_with_space && num > 0) pre_char = " ";
                else pre_char = "";

                if (precise) num = num.toFixed(precision);
                else num = num.toFixed(6);

                if (justify_right) {
                    num = num.toString().padStart(pad_width, pad_char);
                } else {
                    pad_char = " ";
                    num = num.toString().padEnd(pad_width, pad_char);
                }

                if (capitalize) num = num.toUpperCase();

                num = pre_char + num;

                result += num;
                fmt_cur += 2;
                reset_state();
                break;
            }

            // TODO: unsupported format specifiers, copy it to the temp string
            // Flags   => *
            // Len Mod => j z t L
            // Specify => a A n p
            default: {
                const str = fmt_buffer[fmt_cur++];
                result += str;
                break;
            }
        }
    }

    return result;
}
