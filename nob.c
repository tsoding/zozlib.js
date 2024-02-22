#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct {
    const char *src_path;
    const char *bin_path;
    const char *wasm_path;
} Example;

Example examples[] = {
    {
        .src_path   = "./examples/core_basic_window.c",
        .bin_path   = "./build/core_basic_window",
        .wasm_path  = "./wasm/core_basic_window.wasm",
    },
    {
        .src_path   = "./examples/core_basic_screen_manager.c",
        .bin_path   = "./build/core_basic_screen_manager",
        .wasm_path  = "./wasm/core_basic_screen_manager.wasm",
    },
    {
        .src_path   = "./examples/core_input_keys.c",
        .bin_path   = "./build/core_input_keys",
        .wasm_path  = "./wasm/core_input_keys.wasm",
    },
    {
        .src_path   = "./examples/shapes_colors_palette.c",
        .bin_path   = "./build/shapes_colors_palette",
        .wasm_path  = "./wasm/shapes_colors_palette.wasm",
    },
    {
        .src_path   = "./examples/tsoding_ball.c",
        .bin_path = "./build/tsoding_ball",
        .wasm_path  = "./wasm/tsoding_ball.wasm",
    },
    {
        .src_path   = "./examples/tsoding_snake/tsoding_snake.c",
        .bin_path = "./build/tsoding_snake",
        .wasm_path  = "./wasm/tsoding_snake.wasm",
    },
    {
        .src_path   = "./examples/core_input_mouse_wheel.c",
        .bin_path   = "./build/core_input_mouse_wheel",
        .wasm_path  = "./wasm/core_input_mouse_wheel.wasm",
    },
    {
        .src_path   = "./examples/text_writing_anim.c",
        .bin_path   = "./build/text_writing_anim",
        .wasm_path  = "./wasm/text_writing_anim.wasm",
    },
    {
        .src_path   = "./examples/libc/math.c",
        .bin_path   = "./build/libc_math",
        .wasm_path  = "./wasm/libc_math.wasm",
    },
    {
        .src_path   = "./examples/libc/exit.c",
        .bin_path   = "./build/libc_exit",
        .wasm_path  = "./wasm/libc_exit.wasm",
    },
    {
        .src_path   = "./examples/libc/malloc.c",
        .bin_path   = "./build/libc_malloc",
        .wasm_path  = "./wasm/libc_malloc.wasm",
    },
    {
        .src_path   = "./examples/libc/time.c",
        .bin_path   = "./build/libc_time",
        .wasm_path  = "./wasm/libc_time.wasm",
    },
    {
        .src_path   = "./examples/libc/file.c",
        .bin_path   = "./build/libc_file",
        .wasm_path  = "./wasm/libc_file.wasm",
    },
};

bool build_native(void)
{
    Nob_Cmd cmd = {0};
    for (size_t i = 0; i < NOB_ARRAY_LEN(examples); ++i) {
        cmd.count = 0;
        nob_cmd_append(&cmd, "clang", "-I./include/");
        nob_cmd_append(&cmd, "-includelibc/libc.h"); // for stdio compatibility
        nob_cmd_append(&cmd, "-o", examples[i].bin_path, examples[i].src_path);
        nob_cmd_append(&cmd, "-L./lib/", "-lraylib", "-lm");
        if (!nob_cmd_run_sync(cmd)) return 1;
    }
}

bool build_wasm(void)
{
    Nob_Cmd cmd = {0};
    for (size_t i = 0; i < NOB_ARRAY_LEN(examples); ++i) {
        cmd.count = 0;
        nob_cmd_append(&cmd, "clang");
        nob_cmd_append(&cmd, "--target=wasm32");
        nob_cmd_append(&cmd, "-I./include");
        nob_cmd_append(&cmd, "-I./libc/include");
        nob_cmd_append(&cmd, "-includelibc/libc.h");
        nob_cmd_append(&cmd, "--no-standard-libraries");
        nob_cmd_append(&cmd, "-Wl,--export-table");
        nob_cmd_append(&cmd, "-Wl,--no-entry");
        nob_cmd_append(&cmd, "-Wl,--allow-undefined");
        nob_cmd_append(&cmd, "-Wl,--export=main");
        nob_cmd_append(&cmd, "-Wl,--export=__heap_base", "-Wl,--export=feraiseexcept");                            // libc
        nob_cmd_append(&cmd, "-Wl,--export=malloc");                                                               // libc
        nob_cmd_append(&cmd, "-Wl,--export=_create_file", "-Wl,--export=setvbuf", "-Wl,--export=_set_file_ready"); // libc
        nob_cmd_append(&cmd, "-Wl,--export=_add_errno", "-Wl,--export=_set_errno");                                // libc
        nob_cmd_append(&cmd, "-o");
        nob_cmd_append(&cmd, examples[i].wasm_path);
        nob_cmd_append(&cmd, examples[i].src_path);
        nob_cmd_append(&cmd, "-DPLATFORM_WEB");
        if (!nob_cmd_run_sync(cmd)) return 1;
    }
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    if (!nob_mkdir_if_not_exists("build/")) return 1;
    build_native();
    build_wasm();
    return 0;
}
