function make_environment(env) {
    return new Proxy(env, {
        get(target, prop, receiver) {
            if (env[prop] !== undefined) {
                return env[prop].bind(env);
            }
            return (...args) => {
                throw new Error(`NOT IMPLEMENTED: ${prop} ${args}`);
            }
        }
    });
}

let iota = 0;
const LOG_ALL     = iota++; // Display all logs
const LOG_TRACE   = iota++; // Trace logging, intended for internal use only
const LOG_DEBUG   = iota++; // Debug logging, used for internal debugging, it should be disabled on release builds
const LOG_INFO    = iota++; // Info logging, used for program execution info
const LOG_WARNING = iota++; // Warning logging, used on recoverable failures
const LOG_ERROR   = iota++; // Error logging, used on unrecoverable failures
const LOG_FATAL   = iota++; // Fatal logging, used to abort program: exit(EXIT_FAILURE)
const LOG_NONE    = iota++; // Disable logging

const MAX_TEXTFORMAT_BUFFERS = 4;
const MAX_TEXT_BUFFER_LENGTH = 1024;

class RaylibJs {
    // TODO: We stole the font from the website
    // (https://raylib.com/) and it's slightly different than
    // the one that is "baked" into Raylib library itself. To
    // account for the differences we scale the size with a
    // magical factor.
    //
    // It would be nice to have a better approach...
    #FONT_SCALE_MAGIC = 0.65;

    #reset() {
        this.previous = undefined;
        this.wasm = undefined;
        this.ctx = undefined;
        this.dt = undefined;
        this.targetFPS = 60;
        this.entryFunction = undefined;
        this.prevPressedKeyState = new Set();
        this.currentPressedKeyState = new Set();
        this.currentMouseWheelMoveState = 0;
        this.currentMousePosition = {x: 0, y: 0};
        this.images = [];
        this.quit = false;
        this.textFormatBufferIndex = 0;
    }

    constructor() {
        this.#reset();
    }

    stop() {
        this.quit = true;
    }

    async start({ wasmPath, canvasId }) {
        if (this.wasm !== undefined) {
            console.error("The game is already running. Please stop() it first.");
            return;
        }

        const canvas = document.getElementById(canvasId);
        this.ctx = canvas.getContext("2d");
        if (this.ctx === null) {
            throw new Error("Could not create 2d canvas context");
        }

        this.wasm = await WebAssembly.instantiateStreaming(fetch(wasmPath), {
            env: make_environment(this)
        });

        const keyDown = (e) => {
            this.currentPressedKeyState.add(glfwKeyMapping[e.code]);
        };
        const keyUp = (e) => {
            this.currentPressedKeyState.delete(glfwKeyMapping[e.code]);
        };
        const wheelMove = (e) => {
          this.currentMouseWheelMoveState = Math.sign(-e.deltaY);
        };
        const mouseMove = (e) => {
            this.currentMousePosition = {x: e.clientX, y: e.clientY};
        };
        window.addEventListener("keydown", keyDown);
        window.addEventListener("keyup", keyUp);
        window.addEventListener("wheel", wheelMove);
        window.addEventListener("mousemove", mouseMove);

        this.wasm.instance.exports.main();
        const next = (timestamp) => {
            if (this.quit) {
                this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
                window.removeEventListener("keydown", keyDown);
                this.#reset()
                return;
            }
            this.dt = (timestamp - this.previous)/1000.0;
            this.previous = timestamp;
            this.entryFunction();
            window.requestAnimationFrame(next);
        };
        window.requestAnimationFrame((timestamp) => {
            this.previous = timestamp;
            window.requestAnimationFrame(next);
        });
    }

    InitWindow(width, height, title_ptr) {
        this.ctx.canvas.width = width;
        this.ctx.canvas.height = height;
        const buffer = this.wasm.instance.exports.memory.buffer;
        document.title = cstr_by_ptr(buffer, title_ptr);
    }

    WindowShouldClose(){
        return false;
    }

    SetTargetFPS(fps) {
        console.log(`The game wants to run at ${fps} FPS, but in Web we gonna just ignore it.`);
        this.targetFPS = fps;
    }

    GetScreenWidth() {
        return this.ctx.canvas.width;
    }

    GetScreenHeight() {
        return this.ctx.canvas.height;
    }

    GetFrameTime() {
        // TODO: This is a stopgap solution to prevent sudden jumps in dt when the user switches to a differen tab.
        // We need a proper handling of Target FPS here.
        return Math.min(this.dt, 1.0/this.targetFPS);
    }

    BeginDrawing() {}

    EndDrawing() {
        this.prevPressedKeyState.clear();
        this.prevPressedKeyState = new Set(this.currentPressedKeyState);
        this.currentMouseWheelMoveState = 0.0;
    }

    DrawCircleV(center_ptr, radius, color_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const [x, y] = new Float32Array(buffer, center_ptr, 2);
        const [r, g, b, a] = new Uint8Array(buffer, color_ptr, 4);
        const color = color_hex_unpacked(r, g, b, a);
        this.ctx.beginPath();
        this.ctx.arc(x, y, radius, 0, 2*Math.PI, false);
        this.ctx.fillStyle = color;
        this.ctx.fill();
    }

    ClearBackground(color_ptr) {
        this.ctx.fillStyle = getColorFromMemory(this.wasm.instance.exports.memory.buffer, color_ptr);
        this.ctx.fillRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
    }

    // RLAPI void DrawText(const char *text, int posX, int posY, int fontSize, Color color);       // Draw text (using default font)
    DrawText(text_ptr, posX, posY, fontSize, color_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const text = cstr_by_ptr(buffer, text_ptr);
        const color = getColorFromMemory(buffer, color_ptr);
        fontSize *= this.#FONT_SCALE_MAGIC;
        this.ctx.fillStyle = color;
        // TODO: since the default font is part of Raylib the css that defines it should be located in raylib.js and not in index.html
        this.ctx.font = `${fontSize}px grixel`;

        const lines = text.split('\n');
        for (var i = 0; i < lines.length; i++) {
            this.ctx.fillText(lines[i], posX, posY + fontSize + (i * fontSize));
        }
    }

    // RLAPI void DrawRectangle(int posX, int posY, int width, int height, Color color);                        // Draw a color-filled rectangle
    DrawRectangle(posX, posY, width, height, color_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const color = getColorFromMemory(buffer, color_ptr);
        this.ctx.fillStyle = color;
        this.ctx.fillRect(posX, posY, width, height);
    }

    IsKeyPressed(key) {
        return !this.prevPressedKeyState.has(key) && this.currentPressedKeyState.has(key);
    }
    IsKeyDown(key) {
        return this.currentPressedKeyState.has(key);
    }
    GetMouseWheelMove() {
      return this.currentMouseWheelMoveState;
    }
    IsGestureDetected() {
        return false;
    }

    TextFormat(text_ptr, args_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const fmt_text = cstr_fmt_by_ptr(buffer, text_ptr, args_ptr);
        const fmt_text_len = fmt_text.length + 1;

        const heap_base = this.wasm.instance.exports.__heap_base.value;
        // TODO: Check if the values exceeds the heap end
        // const heap_end = this.wasm.instance.exports.__heap_end.value;
        const heap_ptr = heap_base + this.textFormatBufferIndex * MAX_TEXT_BUFFER_LENGTH;

        // Inserting "..." at the end of the string to mark as truncated
        if (fmt_text_len >= MAX_TEXT_BUFFER_LENGTH) {
            fmt_text = fmt_text.substring(0, MAX_TEXT_BUFFER_LENGTH - 4) + "...";
            fmt_text_len = fmt_text.length + 1;
        }

        // TODO: Other functions still override this allocation, need to implement
        // allocator which manages the ownership of chunks.
        const bytes = new Uint8Array(buffer, heap_ptr, fmt_text_len);
        for (let i = 0; i < fmt_text.length; i++) {
            bytes[i] = fmt_text.charCodeAt(i);
        }

        // Mark end with null
        bytes[fmt_text.length] = 0;
        this.textFormatBufferIndex += 1;
        if (this.textFormatBufferIndex >= MAX_TEXTFORMAT_BUFFERS) this.textFormatBufferIndex = 0;

        return heap_ptr;
    }


    TraceLog(logLevel, text_ptr, ... args) {
        // TODO: Implement printf style formatting for TraceLog
        const buffer = this.wasm.instance.exports.memory.buffer;
        const text = cstr_by_ptr(buffer, text_ptr);
        switch(logLevel) {
        case LOG_ALL:     console.log(`ALL: ${text} ${args}`);     break;
        case LOG_TRACE:   console.log(`TRACE: ${text} ${args}`);   break;
        case LOG_DEBUG:   console.log(`DEBUG: ${text} ${args}`);   break;
        case LOG_INFO:    console.log(`INFO: ${text} ${args}`);    break;
        case LOG_WARNING: console.log(`WARNING: ${text} ${args}`); break;
        case LOG_ERROR:   console.log(`ERROR: ${text} ${args}`);   break;
        case LOG_FATAL:   throw new Error(`FATAL: ${text}`);
        case LOG_NONE:    console.log(`NONE: ${text} ${args}`);    break;
        }
    }

    GetMousePosition(result_ptr) {
        const bcrect = this.ctx.canvas.getBoundingClientRect();
        const x = this.currentMousePosition.x - bcrect.left;
        const y = this.currentMousePosition.y - bcrect.top;

        const buffer = this.wasm.instance.exports.memory.buffer;
        new Float32Array(buffer, result_ptr, 2).set([x, y]);
    }

    CheckCollisionPointRec(point_ptr, rec_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const [x, y] = new Float32Array(buffer, point_ptr, 2);
        const [rx, ry, rw, rh] = new Float32Array(buffer, rec_ptr, 4);
        return ((x >= rx) && x <= (rx + rw) && (y >= ry) && y <= (ry + rh));
    }

    Fade(result_ptr, color_ptr, alpha) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const [r, g, b, _] = new Uint8Array(buffer, color_ptr, 4);
        const newA = Math.max(0, Math.min(255, 255.0*alpha));
        new Uint8Array(buffer, result_ptr, 4).set([r, g, b, newA]);
    }

    DrawRectangleRec(rec_ptr, color_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const [x, y, w, h] = new Float32Array(buffer, rec_ptr, 4);
        const color = getColorFromMemory(buffer, color_ptr);
        this.ctx.fillStyle = color;
        this.ctx.fillRect(x, y, w, h);
    }

    DrawRectangleLinesEx(rec_ptr, lineThick, color_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const [x, y, w, h] = new Float32Array(buffer, rec_ptr, 4);
        const color = getColorFromMemory(buffer, color_ptr);
        this.ctx.strokeStyle = color;
        this.ctx.lineWidth = lineThick;
        this.ctx.strokeRect(x + lineThick/2, y + lineThick/2, w - lineThick, h - lineThick);
    }

    MeasureText(text_ptr, fontSize) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const text = cstr_by_ptr(buffer, text_ptr);
        fontSize *= this.#FONT_SCALE_MAGIC;
        this.ctx.font = `${fontSize}px grixel`;
        return this.ctx.measureText(text).width;
    }

    TextSubtext(text_ptr, position, length) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const text = cstr_by_ptr(buffer, text_ptr);
        const subtext = text.substring(position, length);

        var bytes = new Uint8Array(buffer, 0, subtext.length+1);
        for(var i = 0; i < subtext.length; i++) {
            bytes[i] = subtext.charCodeAt(i);
        }
        bytes[subtext.length] = 0;

        return bytes;
    }

    // RLAPI Texture2D LoadTexture(const char *fileName);
    LoadTexture(result_ptr, filename_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const filename = cstr_by_ptr(buffer, filename_ptr);

        var result = new Uint32Array(buffer, result_ptr, 5)
        var img = new Image();
        img.src = filename;
        this.images.push(img);

        result[0] = this.images.indexOf(img);
        // TODO: get the true width and height of the image
        result[1] = 256; // width
        result[2] = 256; // height
        result[3] = 1; // mipmaps
        result[4] = 7; // format PIXELFORMAT_UNCOMPRESSED_R8G8B8A8

        return result;
    }

    // RLAPI void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
    DrawTexture(texture_ptr, posX, posY, color_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const [id, width, height, mipmaps, format] = new Uint32Array(buffer, texture_ptr, 5);
        // // TODO: implement tinting for DrawTexture
        // const tint = getColorFromMemory(buffer, color_ptr);

        this.ctx.drawImage(this.images[id], posX, posY);
    }

    // TODO: codepoints are not implemented
    LoadFontEx(result_ptr, fileName_ptr/*, fontSize, codepoints, codepointCount*/) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const fileName = cstr_by_ptr(buffer, fileName_ptr);
        // TODO: dynamically generate the name for the font
        // Support more than one custom font
        const font = new FontFace("myfont", `url(${fileName})`);
        document.fonts.add(font);
        font.load();
    }

    GenTextureMipmaps() {}
    SetTextureFilter() {}

    MeasureTextEx(result_ptr, font, text_ptr, fontSize, spacing) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const text = cstr_by_ptr(buffer, text_ptr);
        const result = new Float32Array(buffer, result_ptr, 2);
        this.ctx.font = fontSize+"px myfont";
        const metrics = this.ctx.measureText(text)
        result[0] = metrics.width;
        result[1] = fontSize;
    }

    DrawTextEx(font, text_ptr, position_ptr, fontSize, spacing, tint_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        const text = cstr_by_ptr(buffer, text_ptr);
        const [posX, posY] = new Float32Array(buffer, position_ptr, 2);
        const tint = getColorFromMemory(buffer, tint_ptr);
        this.ctx.fillStyle = tint;
        this.ctx.font = fontSize+"px myfont";
        this.ctx.fillText(text, posX, posY + fontSize);
    }

    raylib_js_set_entry(entry) {
        this.entryFunction = this.wasm.instance.exports.__indirect_function_table.get(entry);
    }
}

const glfwKeyMapping = {
    "Space":          32,
    "Quote":          39,
    "Comma":          44,
    "Minus":          45,
    "Period":         46,
    "Slash":          47,
    "Digit0":         48,
    "Digit1":         49,
    "Digit2":         50,
    "Digit3":         51,
    "Digit4":         52,
    "Digit5":         53,
    "Digit6":         54,
    "Digit7":         55,
    "Digit8":         56,
    "Digit9":         57,
    "Semicolon":      59,
    "Equal":          61,
    "KeyA":           65,
    "KeyB":           66,
    "KeyC":           67,
    "KeyD":           68,
    "KeyE":           69,
    "KeyF":           70,
    "KeyG":           71,
    "KeyH":           72,
    "KeyI":           73,
    "KeyJ":           74,
    "KeyK":           75,
    "KeyL":           76,
    "KeyM":           77,
    "KeyN":           78,
    "KeyO":           79,
    "KeyP":           80,
    "KeyQ":           81,
    "KeyR":           82,
    "KeyS":           83,
    "KeyT":           84,
    "KeyU":           85,
    "KeyV":           86,
    "KeyW":           87,
    "KeyX":           88,
    "KeyY":           89,
    "KeyZ":           90,
    "BracketLeft":    91,
    "Backslash":      92,
    "BracketRight":   93,
    "Backquote":      96,
    //  GLFW_KEY_WORLD_1   161 /* non-US #1 */
    //  GLFW_KEY_WORLD_2   162 /* non-US #2 */
    "Escape":         256,
    "Enter":          257,
    "Tab":            258,
    "Backspace":      259,
    "Insert":         260,
    "Delete":         261,
    "ArrowRight":     262,
    "ArrowLeft":      263,
    "ArrowDown":      264,
    "ArrowUp":        265,
    "PageUp":         266,
    "PageDown":       267,
    "Home":           268,
    "End":            269,
    "CapsLock":       280,
    "ScrollLock":     281,
    "NumLock":        282,
    "PrintScreen":    283,
    "Pause":          284,
    "F1":             290,
    "F2":             291,
    "F3":             292,
    "F4":             293,
    "F5":             294,
    "F6":             295,
    "F7":             296,
    "F8":             297,
    "F9":             298,
    "F10":            299,
    "F11":            300,
    "F12":            301,
    "F13":            302,
    "F14":            303,
    "F15":            304,
    "F16":            305,
    "F17":            306,
    "F18":            307,
    "F19":            308,
    "F20":            309,
    "F21":            310,
    "F22":            311,
    "F23":            312,
    "F24":            313,
    "F25":            314,
    "NumPad0":        320,
    "NumPad1":        321,
    "NumPad2":        322,
    "NumPad3":        323,
    "NumPad4":        324,
    "NumPad5":        325,
    "NumPad6":        326,
    "NumPad7":        327,
    "NumPad8":        328,
    "NumPad9":        329,
    "NumpadDecimal":  330,
    "NumpadDivide":   331,
    "NumpadMultiply": 332,
    "NumpadSubtract": 333,
    "NumpadAdd":      334,
    "NumpadEnter":    335,
    "NumpadEqual":    336,
    "ShiftLeft":      340,
    "ControlLeft" :   341,
    "AltLeft":        342,
    "MetaLeft":       343,
    "ShiftRight":     344,
    "ControlRight":   345,
    "AltRight":       346,
    "MetaRight":      347,
    "ContextMenu":    348,
    //  GLFW_KEY_LAST   GLFW_KEY_MENU
}

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
