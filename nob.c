#include <stdbool.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

void log_available_subcommands(const char *program, Nob_Log_Level level) 
{
    nob_log(level, "Usage: %s [subcommand]", program);
    nob_log(level, "Subcommands:");
    nob_log(level, "    build (default)");
    nob_log(level, "    help");
}

bool build_native(const char *examples_dir, const char *example) 
{
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_String_Builder exe_sb = {0};
    Nob_String_Builder example_sb = {0};

    nob_sb_append_cstr(&example_sb, examples_dir);
    nob_sb_append_cstr(&example_sb, "/");
    nob_sb_append_cstr(&example_sb, example);
    nob_sb_append_null(&example_sb);

    Nob_String_View example_sv = nob_sv_from_cstr(example);
    const char *exe = nob_temp_sv_to_cstr(nob_sv_chop_by_delim(&example_sv, '.'));
    Nob_String_View c_ext_sv = nob_sv_from_cstr("c");
   
    // example_sv contains the file extension after choping
    // Do not build stuffs that does not have extension `c`
    if(!nob_sv_eq(example_sv, c_ext_sv)) nob_return_defer(true);
   
    nob_sb_append_cstr(&exe_sb, "./build");
    nob_sb_append_cstr(&exe_sb, "/");
    nob_sb_append_cstr(&exe_sb, exe);
    nob_sb_append_null(&exe_sb);

    nob_cmd_append(&cmd, "clang");
    nob_cmd_append(&cmd, "-o", exe_sb.items, example_sb.items);
    nob_cmd_append(&cmd, "-I./include/");

#ifdef _WIN32
    nob_cmd_append(&cmd, "-L./lib/win64", "-mwindows", "-lraylib", "-lgdi32", "-lwinmm");
#else
    nob_cmd_append(&cmd, "-L./lib/", "-lraylib", "-lm");
#endif //_WIN32

    if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

defer:
    nob_cmd_free(cmd);
    nob_sb_free(example_sb);
    nob_sb_free(exe_sb);
    return result;
}

bool build_wasm(const char *examples_dir, const char *example) 
{
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_String_Builder wasm_sb = {0};
    Nob_String_Builder example_sb = {0};

    nob_sb_append_cstr(&example_sb, examples_dir);
    nob_sb_append_cstr(&example_sb, "/");
    nob_sb_append_cstr(&example_sb, example);
    nob_sb_append_null(&example_sb);

    Nob_String_View example_sv = nob_sv_from_cstr(example);
    const char *wasm = nob_temp_sv_to_cstr(nob_sv_chop_by_delim(&example_sv, '.'));
    Nob_String_View c_ext_sv = nob_sv_from_cstr("c");
   
    // example_sv contains the file extension after choping
    // Do not build stuffs that does not have extension `c`
    if(!nob_sv_eq(example_sv, c_ext_sv)) nob_return_defer(true);

    nob_sb_append_cstr(&wasm_sb, "./wasm");
    nob_sb_append_cstr(&wasm_sb, "/");
    nob_sb_append_cstr(&wasm_sb, wasm);
    nob_sb_append_cstr(&wasm_sb, ".wasm");
    nob_sb_append_null(&wasm_sb);

    nob_cmd_append(&cmd, "clang");
    nob_cmd_append(&cmd, "--target=wasm32");
    nob_cmd_append(&cmd, "-I./include/", "--no-standard-libraries");
    nob_cmd_append(&cmd, "-Wl,--export-table", "-Wl,--no-entry");
    nob_cmd_append(&cmd, "-Wl,--allow-undefined", "-Wl,--export=main");
    nob_cmd_append(&cmd, "-o", wasm_sb.items, example_sb.items);
    nob_cmd_append(&cmd, "-DPLATFORM_WEB");

    if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

defer:
    nob_cmd_free(cmd);
    nob_sb_free(example_sb);
    nob_sb_free(wasm_sb);
    return result;
}

int main(int argc, char **argv) 
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    
    const char *program = nob_shift_args(&argc, &argv);

    const char *subcommand = NULL;
    if (argc <= 0) {
        subcommand = "build";
    } else {
        subcommand = nob_shift_args(&argc, &argv);
    }

    if (strcmp(subcommand, "build") == 0) {

        if (!nob_mkdir_if_not_exists("build")) return 1;
        if (!nob_mkdir_if_not_exists("wasm"))  return 1;

        const char *examples_dir = "./examples";
        Nob_File_Paths examples = {0};
        if (!nob_read_entire_dir(examples_dir, &examples)) return 1;

        nob_log(NOB_INFO, "--- NATIVE ---");

        for (size_t i = 0; i < examples.count; ++i) {
            if (strcmp(examples.items[i], ".") == 0) continue;
            if (strcmp(examples.items[i], "..") == 0) continue;

            if(!build_native(examples_dir, examples.items[i])) return 1;
        }

        nob_log(NOB_INFO, "--- WASM ---");

        for (size_t i = 0; i < examples.count; ++i) {
            if (strcmp(examples.items[i], ".") == 0) continue;
            if (strcmp(examples.items[i], "..") == 0) continue;

            if(!build_wasm(examples_dir, examples.items[i])) return 1;
        }

        nob_temp_reset();
        nob_da_free(examples);
    } else if (strcmp(subcommand, "help") == 0){
        log_available_subcommands(program, NOB_INFO);
    } else {
        nob_log(NOB_ERROR, "Unknown subcommand %s", subcommand);
        log_available_subcommands(program, NOB_ERROR);
    }
    return 0;
}
