#!/bin/sh

set -xe

mkdir -p build/

clang -I./include/ -o build/core_basic_window ./examples/core_basic_window.c -L./lib/ -lraylib -lm
clang -I./include/ -o build/core_basic_screen_manager ./examples/core_basic_screen_manager.c -L./lib/ -lraylib -lm
clang -I./include/ -o build/core_input_keys ./examples/core_input_keys.c -L./lib/ -lraylib -lm
clang -I./include/ -o build/shapes_colors_palette ./examples/shapes_colors_palette.c -L./lib/ -lraylib -lm
clang -I./include/ -o build/game ./game.c -L./lib/ -lraylib -lm
clang -I./include/ -o ./build/core_input_mouse_wheel ./examples/core_input_mouse_wheel.c -L./lib/ -lraylib -lm

clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/core_basic_window.wasm ./examples/core_basic_window.c -DPLATFORM_WEB
clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/core_basic_screen_manager.wasm ./examples/core_basic_screen_manager.c -DPLATFORM_WEB
clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/core_input_keys.wasm ./examples/core_input_keys.c -DPLATFORM_WEB
clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/shapes_colors_palette.wasm ./examples/shapes_colors_palette.c -DPLATFORM_WEB
clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/game.wasm game.c -DPLATFORM_WEB
clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/core_input_mouse_wheel.wasm ./examples/core_input_mouse_wheel.c -DPLATFORM_WEB

# Instrument all wasm files with Asyncify
ls wasm | xargs -I{} wasm-opt --asyncify wasm/{} -o wasm/{}
