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

export class RaylibJs {
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
        this.dt = undefined;
        this.targetFPS = 60;
        this.entryFunction = undefined;
        this.prevPressedKeyState = new Set();
        this.currentPressedKeyState = new Set();
        this.currentMouseWheelMoveState = 0;
        this.currentMousePosition = {x: 0, y: 0};
        this.frameId = undefined
    }
    
    constructor(canvas, platform) {
        this.ctx = canvas.getContext("2d");
        if (this.ctx === null) {
            throw new Error("Could not create 2d canvas context");
        }
        this.platform = platform
        this.#reset();
    }

    handleKeyDown(keyCode) {
        this.currentPressedKeyState.add(keyCode);
    }

    handleKeyUp(keyCode) {
        this.currentPressedKeyState.delete(keyCode);
    }

    handleWheelMove(direction) {
        this.currentMouseWheelMoveState = direction
    }

    handleMouseMove(position) {
        this.currentMousePosition = position
    }

    next = (timestamp) => {
        this.dt = (timestamp - this.previous)/1000.0;
        this.previous = timestamp;
        this.entryFunction();
        this.frameId = requestAnimationFrame(this.next);
    }

    async start({ wasmPath }) {
        if (this.wasm !== undefined) {
            throw new Error("The game is already running. Please stop() it first.");
        }
        this.wasm = await WebAssembly.instantiateStreaming(fetch(wasmPath), {
            env: make_environment(this)
        });
        this.wasm.instance.exports.main();
        this.frameId = requestAnimationFrame((timestamp) => {
            this.previous = timestamp
            this.frameId = requestAnimationFrame(this.next)
        });
    }
    
    stop() {
        cancelAnimationFrame(this.frameId);
        this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
        this.#reset()
    }

    InitWindow(width, height, title_ptr) {
        this.ctx.canvas.width = width;
        this.ctx.canvas.height = height;
        const buffer = this.wasm.instance.exports.memory.buffer;
        this.platform.updateTitle(cstr_by_ptr(buffer, title_ptr))
    }

    WindowShouldClose() {
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

    TextFormat(... args) {
        // TODO: Implement printf style formatting for TextFormat
        return args[0];
    }

    GetMousePosition(result_ptr) {
        const buffer = this.wasm.instance.exports.memory.buffer;
        new Float32Array(buffer, result_ptr, 2).set([
            this.currentMousePosition.x,
            this.currentMousePosition.y,
        ]);
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

const REQUEST_MESSAGE_TYPE = {
    INIT: 0,
    START: 1,
    STOP: 2,
    KEY_DOWN: 3,
    KEY_UP: 4,
    WHEEL_MOVE: 5,
    MOUSE_MOVE: 6,
}

const RESPONSE_MESSAGE_TYPE = {
    START_SUCCESS: 0,
    START_FAIL: 1,
    UPDATE_TITLE: 2,
}

export function makeMessagesHandler(self) {
    let raylibJs = undefined
    const platform = {
        updateTitle: (title) => {
            self.postMessage({
                type: RESPONSE_MESSAGE_TYPE.UPDATE_TITLE,
                title
            })
        },
    }
    const handlers = new Array(Object.keys(REQUEST_MESSAGE_TYPE).length)
    handlers[REQUEST_MESSAGE_TYPE.INIT] = ({ canvas }) => {
        if (raylibJs) {
            raylibJs.stop()
        }
        raylibJs = new RaylibJs(canvas, platform)
    }
    handlers[REQUEST_MESSAGE_TYPE.START] = async ({ params }) => {
        try {
            await self.fonts.ready
            await raylibJs.start(params)
            self.postMessage({
                type: RESPONSE_MESSAGE_TYPE.START_SUCCESS
            })
        } catch (error) {
            console.log(error)
            self.postMessage({
                type: RESPONSE_MESSAGE_TYPE.START_FAIL,
                reason: String(error)
            })
        }
    }
    handlers[REQUEST_MESSAGE_TYPE.STOP] = () => {
        raylibJs.stop()
    }
    handlers[REQUEST_MESSAGE_TYPE.KEY_DOWN] = ({ keyCode }) => {
        raylibJs.handleKeyDown(keyCode)
    }
    handlers[REQUEST_MESSAGE_TYPE.KEY_UP] = ({ keyCode }) => {
        raylibJs.handleKeyUp(keyCode)
    }
    handlers[REQUEST_MESSAGE_TYPE.WHEEL_MOVE] = ({ direction }) => {
        raylibJs.handleWheelMove(direction)
    }
    handlers[REQUEST_MESSAGE_TYPE.MOUSE_MOVE] = ({ position }) => {
        raylibJs.handleMouseMove(position)
    }
    return (event) => {
        if (handlers[event.data.type]) {
            handlers[event.data.type](event.data)
        } else {
            console.error("Unhandled message", event)
        }
    }
}

export class RaylibJsWorker {

    handleMessage = (event) => {
        switch (event.data.type) {
        case RESPONSE_MESSAGE_TYPE.START_SUCCESS: {
            if (this.onStartSuccess) {
                this.onStartSuccess()
                return
            }
        }
        case RESPONSE_MESSAGE_TYPE.START_FAIL: {
            if (this.onStartFail) {
                this.onStartFail(new Error(event.data.reason))
                return
            }
        }
        case RESPONSE_MESSAGE_TYPE.UPDATE_TITLE: {
            this.platform.updateTitle(event.data.title)
            break
        }
        default:
            console.error("Unhandled worker message", event)
        }
    }

    constructor(worker, canvas, platform) {
        this.worker = worker
        this.platform = platform
        this.startPromise = undefined
        this.onStartSuccess = undefined
        this.onStartFail = undefined
        this.worker.addEventListener("message", this.handleMessage)
        // https://developer.mozilla.org/en-US/docs/Web/API/OffscreenCanvas
        const offscreen = canvas.transferControlToOffscreen()
        this.worker.postMessage({
            type: REQUEST_MESSAGE_TYPE.INIT,
            canvas: offscreen,
        }, [offscreen])
    }

    async start(params) {
        if (this.startPromise) {
            return this.startPromise
        }
        this.startPromise = new Promise((resolve, reject) => {
            this.onStartSuccess = resolve
            this.onStartFail = reject
        }).then(() => {
            this.startPromise = undefined
            this.onStartSuccess = undefined
            this.onStartFail = undefined
        })
        this.worker.postMessage({
            type: REQUEST_MESSAGE_TYPE.START,
            params
        })
        return this.startPromise
    }

    stop() {
        this.worker.postMessage({
            type: REQUEST_MESSAGE_TYPE.STOP
        })
        this.worker.removeEventListener("message", this.handleMessage)
    }

    handleKeyDown(keyCode) {
        this.worker.postMessage({
            type: REQUEST_MESSAGE_TYPE.KEY_DOWN,
            keyCode
        })
    }

    handleKeyUp(keyCode) {
        this.worker.postMessage({
            type: REQUEST_MESSAGE_TYPE.KEY_UP,
            keyCode
        })
    }

    handleWheelMove(direction) {
        this.worker.postMessage({
            type: REQUEST_MESSAGE_TYPE.WHEEL_MOVE,
            direction
        })
    }

    handleMouseMove(position) {
        this.worker.postMessage({
            type: REQUEST_MESSAGE_TYPE.MOUSE_MOVE,
            position
        })
    }

}

export const glfwKeyMapping = {
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
