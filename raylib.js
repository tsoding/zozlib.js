function make_environment(...envs) {
    return new Proxy(envs, {
        get(target, prop, receiver) {
            for (let env of envs) {
                if (env.hasOwnProperty(prop)) {
                    return env[prop];
                }
            }
            return (...args) => {
                throw new Error(`NOT IMPLEMENTED: ${prop} ${args}`);
            }
        }
    });
}

let previous = undefined;
let wasm = undefined;
let ctx = undefined;
let dt = undefined;
let targetFPS = 60;
let entryFunction = undefined;
let pressedKeys = new Set();
const glfwKeyMapping = {
    "Enter": 257,
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

WebAssembly.instantiateStreaming(fetch('wasm/core_basic_screen_manager.wasm'), {
    env: make_environment({
        InitWindow: (width, height, title_ptr) => {
            ctx.canvas.width = width;
            ctx.canvas.height = height;
            const buffer = wasm.instance.exports.memory.buffer;
            document.title = cstr_by_ptr(buffer, title_ptr);
        },
        SetTargetFPS: (fps) => {
            console.log(`The game wants to run at ${fps} FPS, but in Web we gonna just ignore it.`);
            targetFPS = fps;
        },
        GetScreenWidth: () => {
            return ctx.canvas.width;
        },
        GetScreenHeight: () => {
            return ctx.canvas.height;
        },
        GetFrameTime: () => {
            return Math.min(dt, 1.0/targetFPS);
        },
        BeginDrawing: () => {},
        EndDrawing: () => {
            pressedKeys.clear();
        },
        DrawCircleV: (center_ptr, radius, color_ptr) => {
            const buffer = wasm.instance.exports.memory.buffer;
            const [x, y] = new Float32Array(buffer, center_ptr, 2);
            const [r, g, b, a] = new Uint8Array(buffer, color_ptr, 4);
            const color = color_hex_unpacked(r, g, b, a);
            ctx.beginPath();
            ctx.arc(x, y, radius, 0, 2*Math.PI, false);
            ctx.fillStyle = color;
            ctx.fill();
        },
        ClearBackground: (color_ptr) => {
            ctx.fillStyle = getColorFromMemory(wasm.instance.exports.memory.buffer, color_ptr);
            ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        },
        // RLAPI void DrawText(const char *text, int posX, int posY, int fontSize, Color color);       // Draw text (using default font)
        DrawText: (text_ptr, posX, posY, fontSize, color_ptr) => {
            const buffer = wasm.instance.exports.memory.buffer;
            const text = cstr_by_ptr(buffer, text_ptr);
            const color = getColorFromMemory(buffer, color_ptr);
            // TODO: We stole the font from the website
            // (https://raylib.com/) and it's slightly different than
            // the one that is "baked" into Raylib library itself. To
            // account for the differences we scale the size with a
            // magical factor.
            //
            // It would be nice to have a better approach...
            fontSize *= 0.65;
            ctx.fillStyle = color;
            ctx.font = `${fontSize}px grixel`;
            ctx.fillText(text, posX, posY + fontSize);
        },
        // RLAPI void DrawRectangle(int posX, int posY, int width, int height, Color color);                        // Draw a color-filled rectangle
        DrawRectangle: (posX, posY, width, height, color_ptr) => {
            const buffer = wasm.instance.exports.memory.buffer;
            const color = getColorFromMemory(buffer, color_ptr);
            ctx.fillStyle = color;
            ctx.fillRect(posX, posY, width, height);
        },
        IsKeyPressed: (key) => {
            return pressedKeys.has(key);
        },
        IsGestureDetected: () => {
            return false;
        },
        raylib_js_set_entry: (entry) => {
            entryFunction = entry;
            console.log(`Entry function was set to ${entryFunction}`);
        },
    })
})
.then((w) => {
    wasm = w;

    const canvas = document.getElementById("game");
    ctx = canvas.getContext("2d");

    window.addEventListener("keydown", (e) => {
        pressedKeys.add(glfwKeyMapping[e.code]);
    });

    w.instance.exports.main();
    let entry = w.instance.exports.__indirect_function_table.get(entryFunction);

    function first(timestamp) {
        previous = timestamp;
        window.requestAnimationFrame(next)
    }
    function next(timestamp) {
        dt = (timestamp - previous)/1000.0;
        previous = timestamp;
        entry();
        window.requestAnimationFrame(next);
    }
    window.requestAnimationFrame(first);
})
.catch((err) => console.log(err));
