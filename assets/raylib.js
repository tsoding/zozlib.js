function make_environment(...envs) {
    return new Proxy(envs, {
        get(target, prop, receiver) {
            for (let env of envs) {
                if (env.hasOwnProperty(prop)) {
                    return env[prop];
                }
            }
            return (...args) => {console.error("NOT IMPLEMENTED: "+prop, args)}
        }
    });
}

let wasm = undefined;
let ctx = undefined;
let targetFPS = 60;
let calculatedFPS = targetFPS;
let dt = undefined;

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

/**
 * @param {number} num
 */
const toHex = (num) => num.toString(16).padStart(2, '0');

function color_hex_unpacked(r, g, b, a) {
    return "#"+toHex(r)+toHex(g)+toHex(b)+toHex(a);
}

const NO_OP = () => {};

WebAssembly.instantiateStreaming(fetch('wasm/game.wasm'), {
    env: make_environment({
        /**
         * @param {number} width
         * @param {number} height
         * @param {IntPtr} title_ptr
         */
        InitWindow: (width, height, title_ptr) => {
            Object.assign(ctx.canvas, { width, height });

            const buffer = wasm.instance.exports.memory.buffer;
            document.title = cstr_by_ptr(buffer, title_ptr);
        },
        /**
         * @param {number} fps
         */
        SetTargetFPS: (fps) => {
            targetFPS = fps;
            if (targetFPS < 1 || targetFPS > 120) targetFPS = 60;

            console.log(`The game requested for ${fps}fps, setting it to ${targetFPS}fps`);
        },
        GetScreenWidth: () => ctx.canvas.width,
        GetScreenHeight: () => ctx.canvas.height,
        GetFrameTime: () => dt/1000,
        GetFPS: () => calculatedFPS,
        BeginDrawing: NO_OP,
        EndDrawing: NO_OP,
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
            const buffer = wasm.instance.exports.memory.buffer;
            const [r, g, b, a] = new Uint8Array(buffer, color_ptr, 4);
            ctx.fillStyle = color_hex_unpacked(r, g, b, a);
            ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        },
    })
})
.then((w) => {
    wasm = w;

    const canvas = document.getElementById("game");
    ctx = canvas.getContext("2d");

    let start = performance.now();
    w.instance.exports.game_init();
    dt = (performance.now() - start);

    let startTime = performance.now();
    let lastFrameTime = startTime;
    let frameCount = 0;

    const frame = () => {
        frameCount++;

        const currentTime = performance.now();
        dt = currentTime - lastFrameTime;
        const elapsedTime = currentTime - startTime;

        if (elapsedTime >= 1000) {
            calculatedFPS = Math.floor((frameCount / elapsedTime) * 500);

            // Reset for the next second
            startTime = currentTime;
            frameCount = 0;
        }

        w.instance.exports.game_frame();

        ctx.font = "24px JetBrainsMono";
        ctx.textBaseline = "hanging";
        ctx.fillText(`${calculatedFPS} fps`, 50, 50);

        lastFrameTime = currentTime;
        setTimeout(frame, Math.max(0, (1000 / targetFPS) - dt));
    };

    frame();
})
.catch((err) => console.log(err));


// == TypeDefs ==

/** @typedef {number} IntPtr */
