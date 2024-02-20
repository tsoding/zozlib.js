const wasmPaths = {
    tsoding: ["tsoding_ball", "tsoding_snake"],
    core: ["core_basic_window", "core_basic_screen_manager", "core_input_keys", "core_input_mouse_wheel"],
    shapes: ["shapes_colors_palette"],
    text: ["text_writing_anim"],
    textures: ["textures_logo_raylib"],
};
const defaultWasm = Object.values(wasmPaths)[0][0];

const raylibExampleSelect = document.getElementById("raylib-example-select");

for (const exampleCategory in wasmPaths) {
    raylibExampleSelect.innerHTML += `<optgroup label="${exampleCategory}">`;
    for (const example of wasmPaths[exampleCategory]) {
        raylibExampleSelect.innerHTML += `<option>${example}</option>`;
    }
    raylibExampleSelect.innerHTML += "</optgroup>";
}

const { protocol } = window.location;
const isHosted = protocol !== "file:";
let raylibJs = undefined;

function startRaylib(selectedWasm) {
    var queryParams = new URLSearchParams(window.location.search);
    queryParams.set("example", selectedWasm);
    history.pushState(null, null, "?" + queryParams.toString());
    raylibExampleSelect.value = selectedWasm;

    if (isHosted) {
        if (raylibJs !== undefined) {
            raylibJs.stop();
        }
        raylibJs = new RaylibJs();
        raylibJs.start({
            wasmPath: `wasm/${selectedWasm}.wasm`,
            canvasId: "game",
        });
    } else {
        window.addEventListener("load", () => {
            document.body.innerHTML = `
                <div class="not-hosted-msg">
                    <div class="important">
                        <p>Unfortunately, due to CORs restrictions, the wasm assembly cannot be fetched.</p>
                        <p>Please navigate to this location using a web server.</p>
                        <p>If you have Python 3 on your system you can just do:</p>
                    </div>
                    <code>$ python3 -m http.server 6969</code>
                </div>
                `;
        });
    }
}

let queryParams = new URLSearchParams(window.location.search);
const exampleParam = queryParams.get("example") ?? defaultWasm;

if (Object.values(wasmPaths).flat().includes(exampleParam)) startRaylib(exampleParam);
else startRaylib(defaultWasm);
