#ifndef TSODING_SNAKE_H_
#define TSODING_SNAKE_H_

#include <stddef.h>

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef int i32;
typedef int b32;
typedef float f32;

void platform_fill_rect(i32 x, i32 y, i32 w, i32 h, u32 color);
void platform_stroke_rect(i32 x, i32 y, i32 w, i32 h, u32 color);
void platform_fill_text(i32 x, i32 y, const char *text, u32 size, u32 color);
u32 platform_text_width(const char *text, u32 size);
void platform_panic(const char *file_path, i32 line, const char *message);
void platform_log(const char *message);

void game_init(u32 width, u32 height);
void game_resize(u32 width, u32 height);
void game_render(void);
void game_update(f32 dt);
void game_keydown(int key);

#endif // TSODING_SNAKE_H_
