#!/bin/sh

set -xe

mkdir -p build/

clang -I./include/ -o build/core_basic_window ./examples/core_basic_window.c -L./lib/ -lraylib -lm
clang -I./include/ -o build/game ./game.c -L./lib/ -lraylib -lm

clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/core_basic_window.wasm ./examples/core_basic_window.c -DPLATFORM_WEB
clang --target=wasm32 -I./include --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=main -o wasm/game.wasm game.c -DPLATFORM_WEB
