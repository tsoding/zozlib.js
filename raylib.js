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
        this.currentLogTypeLevel = TraceLogLevel.LOG_INFO;
        this.quit = false;
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
        this.ctx.fillText(text, posX, posY + fontSize);
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
        const text = cstr_by_ptr(buffer, text_ptr);
        const arg_arr = args_ptr_to_array(text, args_ptr, buffer);
        const msg = PRINTJ.vsprintf(text, arg_arr);
        
        var bytes = new Uint8Array(buffer, 0, msg.length+1);
        for(var i=0;i<msg.length;i++){
            bytes[i] = msg.charCodeAt(i);
        }
        bytes[msg.length] = 0;

        return bytes;
    }

    TraceLog(logLevel, text_ptr, args_ptr) { 
      if (logLevel < this.currentLogTypeLevel) return;

      const buffer = this.wasm.instance.exports.memory.buffer;
      const text = cstr_by_ptr(buffer, text_ptr);
      const arg_arr = args_ptr_to_array(text, args_ptr, buffer);
      const msg = PRINTJ.vsprintf(text, arg_arr);

      switch (logLevel)
      {
          case TraceLogLevel.LOG_TRACE:   console.log("TRACE: " + msg);  break;
          case TraceLogLevel.LOG_DEBUG:   console.log("DEBUG: " + msg);  break;
          case TraceLogLevel.LOG_INFO:    console.log("INFO: " + msg);  break;
          case TraceLogLevel.LOG_WARNING: console.warn("WARNING: " + msg);  break;
          case TraceLogLevel.LOG_ERROR:   console.error("ERROR: " + msg);  break;
          case TraceLogLevel.LOG_FATAL:   throw new Error("FATAL: " + msg);
          default: break;
      }
    }

    SetTraceLogLevel(logLevel){
      this.currentLogTypeLevel = logLevel;
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

const TraceLogLevel = {
    LOG_ALL:     0, // Display all logs
    LOG_TRACE:   1, // Trace logging, intended for internal use only
    LOG_DEBUG:   2, // Debug logging, used for internal debugging, it should be disabled on release builds
    LOG_INFO:    3, // Info logging, used for program execution info
    LOG_WARNING: 4, // Warning logging, used on recoverable failures
    LOG_ERROR:   5, // Error logging, used on unrecoverable failures
    LOG_FATAL:   6, // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    LOG_NONE:    7, // Disable logging
} ;

function round4(x){
    return Math.ceil(x / 4) * 4;
}

function round8(x){
  return Math.ceil(x / 8) * 8;
}

function args_ptr_to_array(fmt, args_ptr, buffer) {
  const tokens = PRINTJ._tokenize(fmt);
  var args = []
  var args_offset = 0;

  for(var i=0;i<tokens.length;i++){
      var token = tokens[i];
      specifier = (token[0]).charCodeAt(0);
      
      switch(specifier) {
          case /*S*/  83: throw Error("%S not implemented!");
          case /*s*/ 115: 
            args_offset = round4(args_offset);
            const str_ptr = new DataView(buffer, args_ptr + args_offset, 4).getInt32(0, true)
            args.push(cstr_by_ptr(buffer, str_ptr));
            args_offset += 4;
            break;
          case /*C*/  67: throw Error("%C not implemented!");
          case /*c*/  99: 
            args.push(new DataView(buffer, args_ptr + args_offset, 4).getInt8(0, true));
            args_offset += 1;
            break;
          case /*D*/  68: throw Error("%D not implemented!");
          case /*d*/ 100:
          case /*i*/ 105: 
            args_offset = round4(args_offset);
            args.push(new DataView(buffer, args_ptr + args_offset, 4).getInt32(0, true));
            args_offset += 4;
            break;
          case /*U*/  85: throw Error("%U not implemented!");
          case /*O*/  79: throw Error("%O not implemented!");
          case /*u*/ 117: 
          case /*o*/ 111: 
          case /*x*/ 120:
          case /*X*/  88: 
            args_offset = round4(args_offset);
            args.push(new DataView(buffer, args_ptr + args_offset, 4).getUint32(0, true));
            args_offset += 4;
            break;
          case /*B*/  66: throw Error("%B not implemented!");
          case /*b*/  98: throw Error("%b not implemented!");
          case /*F*/  70:
          case /*f*/ 102: 
          case /*E*/  69:
          case /*e*/ 101:
            args_offset = round8(args_offset);
            args.push(new DataView(buffer, args_ptr + args_offset, 8).getFloat64(0, true));
            args_offset += 8;
            break;
          case /*G*/  71: throw Error("%G not implemented!");
          case /*g*/ 103: throw Error("%g not implemented!");
          case /*A*/  65: throw Error("%A not implemented!");
          case /*a*/  97: throw Error("%a not implemented!");
          case /*p*/ 112: 
            args_offset = round4(args_offset);
            args.push(new DataView(buffer, args_ptr + args_offset, 4).getInt32(0, true));
            args_offset += 4;
            break;
          case /*n*/ 110: throw Error("%n not implemented!");
          case /*m*/ 109: throw Error("%m not implemented!");


          /* JS-specific conversions (extension) */
          case /*J*/  74: throw Error("%J not implemented!");
          case /*V*/  86: throw Error("%V not implemented!");
          case /*T*/  84: throw Error("%T not implemented!");
          case /*Y*/  89: throw Error("%Y not implemented!");
          case /*y*/ 121: throw Error("%y not implemented!");
          case /*L*/ 76: break; // L is used to specify string literal part of format
          default: 
              throw Error("unknown specifer " + specifier + " not implemented!");
      }
    }

  return args;
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
    r = r.toString(16).padStart(2, '0');
    g = g.toString(16).padStart(2, '0');
    b = b.toString(16).padStart(2, '0');
    a = a.toString(16).padStart(2, '0');
    return "#"+r+g+b+a;
}

function color_hex(color) {
    const r = ((color>>(0*8))&0xFF).toString(16).padStart(2, '0');
    const g = ((color>>(1*8))&0xFF).toString(16).padStart(2, '0');
    const b = ((color>>(2*8))&0xFF).toString(16).padStart(2, '0');
    const a = ((color>>(3*8))&0xFF).toString(16).padStart(2, '0');
    return "#"+r+g+b+a;
}

function getColorFromMemory(buffer, color_ptr) {
    const [r, g, b, a] = new Uint8Array(buffer, color_ptr, 4);
    return color_hex_unpacked(r, g, b, a);
}
