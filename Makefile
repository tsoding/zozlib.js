all: wasm/game.wasm

wasm/game.wasm: game.c
	clang --target=wasm32 -I./include --no-standard-libraries -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=game_init -Wl,--export=game_frame -Wl,--export=game_over -o wasm/game.wasm game.c -DPLATFORM_WEB
