import { DataSchema } from "./SharedData.js";

function setListeners(/** @type {Worker} */ worker, handlers) {
    worker.onmessage = (ev) => {
        if ("topic" in ev.data && "data" in ev.data) {
            if (ev.data.topic in handlers) {
                handlers[ev.data.topic](...ev.data.data);
            } else {
                throw new TypeError(`No handler for ${ev.data.topic}!`);
            }
        } else {
            throw new TypeError("Invalid message received!");
        }
    };
}

export default class RaylibJs {
    async start({ wasmPath, canvasId }) {
        /** @type {HTMLCanvasElement} */
        const canvas = document.getElementById(canvasId);
        if (canvas == null) {
            throw new Error(
                `raylib.js could not find canvas with ID ${canvasId}`,
            );
        }
        this.ctx = canvas.getContext("bitmaprenderer");
        if (this.worker === undefined) {
            this.worker = new Worker("./raylib.js", { type: "module" });
        } else {
            throw new Error("raylib.js worker already exists!");
        }
        const shared = DataSchema.build({
            asyncFlag: "i32",
            time: "f64",
            stop: "u8",
            keys: ["u8", GLFW_KEY_LAST + 1],
            mouse: ["f32", 3],
            boundingRect: ["f32", 4],
        });
        this.views = DataSchema.view(shared);
        // bind listeners
        setListeners(this.worker, {
            frame: this.#onFrame.bind(this),
            window: this.#onWindow.bind(this),
            requestAnimationFrame: this.#onRequestAnimationFrame.bind(this),
        });
        window.addEventListener("keydown", (e) => {
            const key = glfwKeyMapping[e.code];
            this.views.keys[~~(key / 8)] |= 1 << key % 8;
        });
        window.addEventListener("keyup", (e) => {
            const key = glfwKeyMapping[e.code];
            this.views.keys[~~(key / 8)] &= ~(1 << key % 8);
        });
        window.addEventListener("mousemove", (e) => {
            this.views.mouse[0] = e.clientX;
            this.views.mouse[1] = e.clientY;
        });
        window.addEventListener("wheel", (e) => {
            this.views.mouse[2] = e.deltaY;
        });

        // Initialize raylib.js worker
        this.#setBoundingRect();
        this.worker.postMessage({ wasmPath, shared });
    }

    stop() {
        this.views.stop.set([1]);
        // TODO: gracefully shut down
        this.worker.terminate();
    }

    #onFrame(frame) {
        this.ctx.transferFromImageBitmap(frame);
    }

    #onWindow(width, height, title) {
        this.ctx.canvas.width = width;
        this.ctx.canvas.height = height;
        // TODO: listen to bounding client rect changes
        this.#setBoundingRect();
        document.title = title;
    }

    #onRequestAnimationFrame() {
        requestAnimationFrame((timestamp) => {
            this.views.time.set([timestamp]);
            this.#wake();
        });
    }

    #wake() {
        Atomics.store(this.views.asyncFlag, 0, 1);
        Atomics.notify(this.views.asyncFlag, 0);
    }

    #setBoundingRect() {
        const rect = this.ctx.canvas.getBoundingClientRect();
        this.views.boundingRect.set([
            rect.left,
            rect.top,
            rect.width,
            rect.height,
        ]);
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
};

const GLFW_KEY_LAST = 348;
