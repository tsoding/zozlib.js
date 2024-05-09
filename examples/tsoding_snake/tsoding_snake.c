#include "./tsoding_snake.h"
#include <raylib.h>

// #define FEATURE_DYNAMIC_CAMERA
// #define FEATURE_DEV

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define TRUE 1
#define FALSE 0

static char logf_buf[4096] = {0};
#define LOGF(...) \
    do { \
        stbsp_snprintf(logf_buf, sizeof(logf_buf), __VA_ARGS__); \
        platform_log(logf_buf); \
    } while(0)

static void platform_assert(const char *file, i32 line, b32 cond, const char *message)
{
    if (!cond) {
        TraceLog(LOG_FATAL, "%s:%d: GAME ASSERTION FAILED: %s\n", file, line, message);
    }
}
#define ASSERT(cond, message) platform_assert(__FILE__, __LINE__, cond, message)
#define UNREACHABLE() platform_assert(__FILE__, __LINE__, false, "unreachable")

// NOTE: This implies that the platform has to carefully choose the resolution so the cells fit into the screen
#define CELL_SIZE 100
#define COLS 16
#define ROWS 9

#define BACKGROUND_COLOR 0xFF181818
#define CELL1_COLOR BACKGROUND_COLOR
#define CELL2_COLOR 0xFF183018
#define SNAKE_BODY_COLOR 0xFF189018
#define SNAKE_SPINE_COLOR 0xFF185018
#define SNAKE_SPINE_THICCNESS_PERCENT 0.05f
#define EGG_BODY_COLOR 0xFF31A6FF
#define EGG_SPINE_COLOR 0xFF3166BB
#define DEBUG_COLOR 0xFF0000FF

#define SNAKE_INIT_SIZE 3

#define STEP_INTEVAL 0.125f

#define RAND_A 6364136223846793005ULL
#define RAND_C 1442695040888963407ULL

typedef enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
} Align;

static void fill_text_aligned(i32 x, i32 y, const char *text, u32 size, u32 color, Align align)
{
    u32 width = platform_text_width(text, size);
    switch (align) {
    case ALIGN_LEFT:                 break;
    case ALIGN_CENTER: x -= width/2; break;
    case ALIGN_RIGHT:  x -= width;   break;
    }
    platform_fill_text(x, y, text, size, color);
}

static u32 my_rand(void)
{
    static u64 rand_state = 0;
    rand_state = rand_state*RAND_A + RAND_C;
    return (rand_state >> 32)&0xFFFFFFFF;
}

static void *memset(void *mem, u32 c, u32 n)
{
    void *result = mem;
    u8 *bytes = mem;
    while (n-- > 0) *bytes++ = c;
    return result;
}

typedef enum {
    DIR_RIGHT = 0,
    DIR_UP,
    DIR_LEFT,
    DIR_DOWN,
    COUNT_DIRS,
} Dir;

static Dir dir_opposite(Dir dir)
{
    ASSERT(0 <= dir && dir < COUNT_DIRS, "Invalid direction");
    return (dir + 2)%COUNT_DIRS;
}

typedef struct {
    f32 x, y, w, h;
} Rect;

typedef struct {
    f32 lens[COUNT_DIRS];
} Sides;

static Sides rect_sides(Rect rect)
{
    Sides sides = {
        .lens = {
            [DIR_LEFT]  = rect.x,
            [DIR_RIGHT] = rect.x + rect.w,
            [DIR_UP]    = rect.y,
            [DIR_DOWN]  = rect.y + rect.h,
        }
    };
    return sides;
}

static Rect sides_rect(Sides sides)
{
    Rect rect = {
        .x = sides.lens[DIR_LEFT],
        .y = sides.lens[DIR_UP],
        .w = sides.lens[DIR_RIGHT] - sides.lens[DIR_LEFT],
        .h = sides.lens[DIR_DOWN] - sides.lens[DIR_UP],
    };
    return rect;
}

typedef struct {
    i32 x, y;
} Cell;

typedef struct {
    f32 x, y;
} Vec;

#define SNAKE_CAP (ROWS*COLS)
typedef struct {
    Cell items[SNAKE_CAP];
    u32 begin;
    u32 size;
} Snake;

typedef struct {
    Rect items[SNAKE_CAP];
    Vec vels[SNAKE_CAP];
    u8 masks[SNAKE_CAP];
    u32 size;
} Dead_Snake;

typedef enum {
    STATE_GAMEPLAY = 0,
    STATE_PAUSE,
    STATE_GAMEOVER,
} State;

#define DIR_QUEUE_CAP 3
typedef struct {
    u32 begin;
    u32 size;
    Dir items[DIR_QUEUE_CAP];
} Dir_Queue;

typedef struct {
    u32 width;
    u32 height;

    Vec camera_pos;
    Vec camera_vel;

    State state;
    Snake snake;
    Dead_Snake dead_snake;
    Cell egg;
    b32 eating_egg;

    Dir dir;
    Dir_Queue next_dirs;
    b32 dir_keys[COUNT_DIRS];

    f32 step_cooldown;

#ifdef FEATURE_DEV
    f32 dt_scale;
#endif

    b32 infinite_field;

    u32 score;
    char score_buffer[256];
} Game;

static Game game = {0};

static Rect cell_rect(Cell cell)
{
    Rect result = {
        .x = cell.x*CELL_SIZE,
        .y = cell.y*CELL_SIZE,
        .w = CELL_SIZE,
        .h = CELL_SIZE,
    };
    return result;
}

#define ring_empty(ring) ((ring)->size == 0)

#define ring_cap(ring) (sizeof((ring)->items)/sizeof((ring)->items[0]))

#define ring_push_back(ring, item) \
    do { \
        ASSERT((ring)->size < ring_cap(ring), "Ring buffer overflow"); \
        u32 index = ((ring)->begin + (ring)->size)%ring_cap(ring); \
        (ring)->items[index] = (item); \
        (ring)->size += 1; \
    } while (0)

#define ring_displace_back(ring, item) \
    do { \
        u32 index = ((ring)->begin + (ring)->size)%ring_cap(ring); \
        (ring)->items[index] = (item); \
        if ((ring)->size < ring_cap(ring)) { \
            (ring)->size += 1; \
        } else { \
            (ring)->begin = ((ring)->begin + 1)%ring_cap(ring); \
        } \
    } while (0)

#define ring_pop_front(ring) \
    do { \
        ASSERT((ring)->size > 0, "Ring buffer underflow"); \
        (ring)->begin = ((ring)->begin + 1)%ring_cap(ring); \
        (ring)->size -= 1; \
    } while (0)

#define ring_back(ring) \
    (ASSERT((ring)->size > 0, "Ring buffer is empty"), \
     &(ring)->items[((ring)->begin + (ring)->size - 1)%ring_cap(ring)])
#define ring_front(ring) \
    (ASSERT((ring)->size > 0, "Ring buffer is empty"), \
     &(ring)->items[(ring)->begin])
#define ring_get(ring, index) \
    (ASSERT((ring)->size > 0, "Ring buffer is empty"), \
     &(ring)->items[((ring)->begin + (index))%ring_cap(ring)])

static b32 cell_eq(Cell a, Cell b)
{
    return a.x == b.x && a.y == b.y;
}

static i32 is_cell_snake_body(Cell cell)
{
    // TODO: ignoring the tail feel hacky @tail-ignore
    for (u32 index = 1; index < game.snake.size; ++index) {
        if (cell_eq(*ring_get(&game.snake, index), cell)) {
            return index;
        }
    }
    return -1;
}

static i32 emod(i32 a, i32 b)
{
    return (a%b + b)%b;
}

static Cell cell_wrap(Cell cell)
{
    cell.x = emod(cell.x, COLS);
    cell.y = emod(cell.y, ROWS);
    return cell;
}

static Cell dir_cell_data[COUNT_DIRS] = {
    [DIR_LEFT]  = {.x = -1},
    [DIR_RIGHT] = {.x =  1},
    [DIR_UP]    = {.y = -1},
    [DIR_DOWN]  = {.y =  1},
};

static Cell cell_add(Cell a, Cell b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

#define dir_cell(dir) (ASSERT((u32) dir < COUNT_DIRS, "Invalid direction"), dir_cell_data[dir])
#define dir_vec(dir) cell_vec(dir_cell(dir))

static Cell step_cell(Cell head, Dir dir)
{
    if (game.infinite_field) {
        return cell_add(head, dir_cell(dir));
    } else {
        return cell_wrap(cell_add(head, dir_cell(dir)));
    }
}

#define SNAKE_INIT_ROW (ROWS/2)

static void random_egg(b32 first)
{
    i32 col1 = 0;
    i32 col2 = COLS - 1;
    i32 row1 = 0;
    i32 row2 = ROWS - 1;

    // TODO: make a single formula that works for any mode
    if (game.infinite_field) {
        col1 = (i32)(game.camera_pos.x - game.width*0.5f + CELL_SIZE)/CELL_SIZE;
        col2 = (i32)(game.camera_pos.x + game.width*0.5f - CELL_SIZE)/CELL_SIZE;
        row1 = (i32)(game.camera_pos.y - game.height*0.5f + CELL_SIZE)/CELL_SIZE;
        row2 = (i32)(game.camera_pos.y + game.height*0.5f - CELL_SIZE)/CELL_SIZE;
    }

#define RANDOM_EGG_MAX_ATTEMPTS 1000
    u32 attempt = 0;
    do {
        game.egg.x = my_rand()%(col2 - col1 + 1) + col1;
        game.egg.y = my_rand()%(row2 - row1 + 1) + row1;
        attempt += 1;
    } while ((is_cell_snake_body(game.egg) >= 0 || (first && game.egg.y == SNAKE_INIT_ROW)) && attempt < RANDOM_EGG_MAX_ATTEMPTS);

    ASSERT(attempt <= RANDOM_EGG_MAX_ATTEMPTS, "TODO: make sure we have always at least one free visible cell");
}

// TODO: animation on restart
static void game_restart(u32 width, u32 height)
{
    memset(&game, 0, sizeof(game));

#ifdef FEATURE_DEV
    game.dt_scale = 1.0f;
#endif

    game.width        = width;
    game.height       = height;
    game.camera_pos.x = width/2;
    game.camera_pos.y = height/2;

    for (u32 i = 0; i < SNAKE_INIT_SIZE; ++i) {
        Cell head = {.x = i, .y = SNAKE_INIT_ROW};
        ring_push_back(&game.snake, head);
    }
    random_egg(TRUE);
    game.dir = DIR_RIGHT;
    // TODO: Using snprintf to render Score is an overkill
    // I believe snprintf should be only used for LOGF and in the "release" build stbsp_snprintf should not be included at all
    stbsp_snprintf(game.score_buffer, sizeof(game.score_buffer), "Score: %u", game.score);
}

static f32 lerpf(f32 a, f32 b, f32 t)
{
    return (b - a)*t + a;
}

static f32 ilerpf(f32 a, f32 b, f32 v)
{
    return (v - a)/(b - a);
}

static void fill_rect(Rect rect, u32 color)
{
    platform_fill_rect(
        rect.x - game.camera_pos.x + game.width/2,
        rect.y - game.camera_pos.y + game.height/2,
        rect.w, rect.h, color);
}

#ifdef FEATURE_DEV
static void stroke_rect(Rect rect, u32 color)
{
    platform_stroke_rect(
        rect.x - game.camera_pos.x + game.width/2,
        rect.y - game.camera_pos.y + game.height/2,
        rect.w, rect.h, color);
}
#endif

static Rect scale_rect(Rect r, float a)
{
    r.x = lerpf(r.x, r.x + r.w*0.5f, 1.0f - a);
    r.y = lerpf(r.y, r.y + r.h*0.5f, 1.0f - a);
    r.w = lerpf(0.0f, r.w, a);
    r.h = lerpf(0.0f, r.h, a);
    return r;
}

static void fill_cell(Cell cell, u32 color, f32 a)
{
    fill_rect(scale_rect(cell_rect(cell), a), color);
}

static void fill_sides(Sides sides, u32 color)
{
    fill_rect(sides_rect(sides), color);
}

static Dir cells_dir(Cell a, Cell b)
{
    for (Dir dir = 0; dir < COUNT_DIRS; ++dir) {
        if (cell_eq(step_cell(a, dir), b)) return dir;
    }
    UNREACHABLE();
    return 0;
}

static Vec cell_center(Cell a)
{
    return (Vec) {
        .x = a.x*CELL_SIZE + CELL_SIZE/2,
        .y = a.y*CELL_SIZE + CELL_SIZE/2,
    };
}

static Sides slide_sides(Sides sides, Dir dir, f32 a)
{
    f32 d = sides.lens[dir] - sides.lens[dir_opposite(dir)];
    sides.lens[dir]               += lerpf(0, d, a);
    sides.lens[dir_opposite(dir)] += lerpf(0, d, a);
    return sides;
}

Vec sides_center(Sides sides)
{
    return (Vec) {
        .x = sides.lens[DIR_LEFT] + (sides.lens[DIR_RIGHT] - sides.lens[DIR_LEFT])*0.5f,
        .y = sides.lens[DIR_UP] + (sides.lens[DIR_DOWN] - sides.lens[DIR_UP])*0.5f,
    };
}

static void fill_spine(Vec center, Dir dir, float len)
{
    f32 thicc = CELL_SIZE*SNAKE_SPINE_THICCNESS_PERCENT;
    Sides sides = {
        .lens = {
            [DIR_LEFT]   = center.x - thicc,
            [DIR_RIGHT]  = center.x + thicc,
            [DIR_UP]     = center.y - thicc,
            [DIR_DOWN]   = center.y + thicc,
        }
    };
    if (dir == DIR_RIGHT || dir == DIR_DOWN) sides.lens[dir] += len;
    if (dir == DIR_LEFT  || dir == DIR_UP)   sides.lens[dir] -= len;
    fill_sides(sides, SNAKE_SPINE_COLOR);
}

static void fill_fractured_spine(Sides sides, u8 mask)
{
    f32 thicc = CELL_SIZE*SNAKE_SPINE_THICCNESS_PERCENT;
    Vec center = sides_center(sides);
    for (Dir dir = 0; dir < COUNT_DIRS; ++dir) {
        if (mask&(1<<dir)) {
            Sides arm = {
                .lens = {
                    [DIR_LEFT]   = center.x - thicc,
                    [DIR_RIGHT]  = center.x + thicc,
                    [DIR_UP]     = center.y - thicc,
                    [DIR_DOWN]   = center.y + thicc,
                }
            };
            arm.lens[dir] = sides.lens[dir];
            fill_sides(arm, SNAKE_SPINE_COLOR);
        }
    }
}

static void snake_render(void)
{
    f32 t = game.step_cooldown / STEP_INTEVAL;

    Cell  head_cell         = *ring_back(&game.snake);
    Sides head_sides        = rect_sides(cell_rect(head_cell));
    Dir   head_dir          = game.dir;
    Sides head_slided_sides = slide_sides(head_sides, dir_opposite(head_dir), t);

    Cell  tail_cell         = *ring_front(&game.snake);
    Sides tail_sides        = rect_sides(cell_rect(tail_cell));
    Dir   tail_dir          = cells_dir(*ring_get(&game.snake, 0), *ring_get(&game.snake, 1));
    Sides tail_slided_sides = slide_sides(tail_sides, tail_dir, game.eating_egg ? 1.0f : 1.0f - t);

    if (game.eating_egg) {
        fill_cell(head_cell, EGG_BODY_COLOR, 1.0f);
        fill_cell(head_cell, EGG_SPINE_COLOR, SNAKE_SPINE_THICCNESS_PERCENT*2.0f);
    }

    fill_sides(head_slided_sides, SNAKE_BODY_COLOR);
    fill_sides(tail_slided_sides, SNAKE_BODY_COLOR);

    for (u32 index = 1; index < game.snake.size - 1; ++index) {
        fill_cell(*ring_get(&game.snake, index), SNAKE_BODY_COLOR, 1.0f);
    }

    for (u32 index = 1; index < game.snake.size - 2; ++index) {
        Cell cell1 = *ring_get(&game.snake, index);
        Cell cell2 = *ring_get(&game.snake, index + 1);
        // TODO: can we cache that direction in the snake itself?
        fill_spine(cell_center(cell1), cells_dir(cell1, cell2), CELL_SIZE);
        fill_spine(cell_center(cell2), cells_dir(cell2, cell1), CELL_SIZE);
    }

    // Head
    {
        Cell cell1 = *ring_get(&game.snake, game.snake.size - 2);
        Cell cell2 = *ring_get(&game.snake, game.snake.size - 1);
        f32 len = lerpf(0.0f, CELL_SIZE, 1.0f - t);
        fill_spine(cell_center(cell1), cells_dir(cell1, cell2), len);
        fill_spine(cell_center(cell_add(cell2, dir_cell(dir_opposite(head_dir)))), head_dir, len);
    }

    // Tail
    {
        Cell cell1 = *ring_get(&game.snake, 1);
        Cell cell2 = *ring_get(&game.snake, 0);
        f32 len = lerpf(0.0f, CELL_SIZE, game.eating_egg ? 0.0f : t);
        fill_spine(cell_center(cell1), cells_dir(cell1, cell2), len);
        fill_spine(cell_center(cell_add(cell2, dir_cell(tail_dir))), dir_opposite(tail_dir), len);
    }

#ifdef FEATURE_DEV
    for (u32 i = 0; i < game.snake.size; ++i) {
        stroke_rect(cell_rect(*ring_get(&game.snake, i)), 0xFF0000FF);
    }
#endif
}

static void background_render(void)
{
    i32 col1 = (i32)(game.camera_pos.x - game.width*0.5f - CELL_SIZE)/CELL_SIZE;
    i32 col2 = (i32)(game.camera_pos.x + game.width*0.5f + CELL_SIZE)/CELL_SIZE;
    i32 row1 = (i32)(game.camera_pos.y - game.height*0.5f - CELL_SIZE)/CELL_SIZE;
    i32 row2 = (i32)(game.camera_pos.y + game.height*0.5f + CELL_SIZE)/CELL_SIZE;

    for (i32 col = col1; col <= col2; ++col) {
        for (i32 row = row1; row <= row2; ++row) {
            u32 color = (row + col)%2 == 0 ? CELL1_COLOR : CELL2_COLOR;
            Cell cell = { .x = col, .y = row, };
            fill_cell(cell, color, 1.0f);
        }
    }
}

// TODO: controls tutorial
void game_init(u32 width, u32 height)
{
    game_restart(width, height);
    LOGF("Game initialized");
}

#define SCORE_PADDING 100
// TODO: font size relative to the resolution
#define SCORE_FONT_SIZE 48
#define SCORE_FONT_COLOR 0xFFFFFFFF
#define PAUSE_FONT_COLOR SCORE_FONT_COLOR
#define PAUSE_FONT_SIZE SCORE_FONT_SIZE
#define GAMEOVER_FONT_COLOR SCORE_FONT_COLOR
#define GAMEOVER_FONT_SIZE SCORE_FONT_SIZE

static u32 color_alpha(u32 color, f32 a)
{
    return (color&0x00FFFFFF)|((u32)(a*0xFF)<<(3*8));
}

static void egg_render(void)
{
    if (game.eating_egg) {
        f32 t = 1.0f - game.step_cooldown/STEP_INTEVAL;
        f32 a = lerpf(1.5f, 1.0f, t*t);
        fill_cell(game.egg, color_alpha(EGG_BODY_COLOR, t*t), a);
        fill_cell(game.egg, color_alpha(EGG_SPINE_COLOR, t*t), a*(SNAKE_SPINE_THICCNESS_PERCENT*2.0f));
    } else {
        fill_cell(game.egg, EGG_BODY_COLOR, 1.0f);
        fill_cell(game.egg, EGG_SPINE_COLOR, SNAKE_SPINE_THICCNESS_PERCENT*2.0f);
    }
}

static void dead_snake_render(void)
{
    // @tail-ignore
    for (u32 i = 1; i < game.dead_snake.size; ++i) {
        fill_rect(game.dead_snake.items[i], SNAKE_BODY_COLOR);
        fill_fractured_spine(rect_sides(game.dead_snake.items[i]), game.dead_snake.masks[i]);
    }
}

void game_render(void)
{
    switch (game.state) {
    case STATE_GAMEPLAY: {
        background_render();
        egg_render();
        snake_render();
        fill_text_aligned(SCORE_PADDING, SCORE_PADDING, game.score_buffer, SCORE_FONT_SIZE, SCORE_FONT_COLOR, ALIGN_LEFT);
    }
    break;

    case STATE_PAUSE: {
        background_render();
        egg_render();
        snake_render();
        fill_text_aligned(SCORE_PADDING, SCORE_PADDING, game.score_buffer, SCORE_FONT_SIZE, SCORE_FONT_COLOR, ALIGN_LEFT);
        // TODO: "Pause", "Game Over" are not centered vertically
        fill_text_aligned(game.width/2, game.height/2, "Pause", PAUSE_FONT_SIZE, PAUSE_FONT_COLOR, ALIGN_CENTER);
    }
    break;

    case STATE_GAMEOVER: {
        background_render();
        egg_render();
        dead_snake_render();
        fill_text_aligned(SCORE_PADDING, SCORE_PADDING, game.score_buffer, SCORE_FONT_SIZE, SCORE_FONT_COLOR, ALIGN_LEFT);
        fill_text_aligned(game.width/2, game.height/2, "Game Over", GAMEOVER_FONT_SIZE, GAMEOVER_FONT_COLOR, ALIGN_CENTER);
    }
    break;

    default: {
        UNREACHABLE();
    }
    }

#ifdef FEATURE_DEV
    fill_text_aligned(game.width - SCORE_PADDING, SCORE_PADDING, "Dev", SCORE_FONT_SIZE, SCORE_FONT_COLOR, ALIGN_RIGHT);
    Rect rect = { .w = COLS*CELL_SIZE, .h = ROWS*CELL_SIZE };
    stroke_rect(rect, 0xFF0000FF);
#endif
}

static Vec vec_sub(Vec a, Vec b)
{
    return (Vec) {
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
}

static f32 fabsf(f32 x)
{
    if (x < 0.0f) x = -x;
    return x;
}

static f32 sqrtf(f32 a)
{
    float x = a;
    for (u32 i = 0; i < 1000 && fabsf(x*x - a) > 1e-6; ++i) {
        x -= (x*x - a)/(2*x);
    }
    return x;
}

static f32 vec_len(Vec a)
{
    return sqrtf(a.x*a.x + a.y*a.y);
}

void game_resize(u32 width, u32 height)
{
    game.width = width;
    game.height = height;
}

void game_update(f32 dt)
{
#ifdef FEATURE_DEV
    dt *= game.dt_scale;

    #define DEV_DT_SCALE_STEP 0.05f
    if (IsKeyPressed(KEY_Z)) {
        game.dt_scale -= DEV_DT_SCALE_STEP;
        if (game.dt_scale < 0.0f) game.dt_scale = 0.0f;
        LOGF("dt scale = %f", game.dt_scale);
    }
    if (IsKeyPressed(KEY_X)) {
        game.dt_scale += DEV_DT_SCALE_STEP;
        LOGF("dt scale = %f", game.dt_scale);
    }
    if (IsKeyPressed(KEY_C)) {
        game.dt_scale = 1.0f;
        LOGF("dt scale = %f", game.dt_scale);
    }
#endif

#define CAMERA_VELOCITY_FACTOR 0.80f
    if (game.infinite_field) {
        game.camera_pos.x += game.camera_vel.x*CAMERA_VELOCITY_FACTOR*dt;
        game.camera_pos.y += game.camera_vel.y*CAMERA_VELOCITY_FACTOR*dt;
        game.camera_vel = vec_sub(
                              cell_center(*ring_back(&game.snake)),
                              game.camera_pos);
    }

    switch (game.state) {
    case STATE_GAMEPLAY: {
        if (IsKeyPressed(KEY_W)) {
            ring_displace_back(&game.next_dirs, DIR_UP);
        }
        if (IsKeyPressed(KEY_S)) {
            ring_displace_back(&game.next_dirs, DIR_DOWN);
        }
        if (IsKeyPressed(KEY_A)) {
            ring_displace_back(&game.next_dirs, DIR_LEFT);
        }
        if (IsKeyPressed(KEY_D)) {
            ring_displace_back(&game.next_dirs, DIR_RIGHT);
        }
        if (IsKeyPressed(KEY_SPACE)) {
            game.state = STATE_PAUSE;
        }
        if (IsKeyPressed(KEY_R)) {
            game_restart(game.width, game.height);
        }
        if ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER)) {
            ToggleFullscreen();
        }

        game.step_cooldown -= dt;
        if (game.step_cooldown <= 0.0f) {
            if (!ring_empty(&game.next_dirs)) {
                if (dir_opposite(game.dir) != *ring_front(&game.next_dirs)) {
                    game.dir = *ring_front(&game.next_dirs);
                }
                ring_pop_front(&game.next_dirs);
            }

            Cell next_head = step_cell(*ring_back(&game.snake), game.dir);

            if (cell_eq(game.egg, next_head)) {
                ring_push_back(&game.snake, next_head);
                random_egg(FALSE);
                game.eating_egg = TRUE;
#ifdef FEATURE_DYNAMIC_CAMERA
                game.infinite_field = TRUE;
#endif
                game.score += 1;
                stbsp_snprintf(game.score_buffer, sizeof(game.score_buffer), "Score: %u", game.score);
            } else {
                i32 next_head_index = is_cell_snake_body(next_head);
                if (next_head_index >= 0) {
                    // NOTE: reseting step_cooldown to 0 is important bcause the whole smooth movement is based on it.
                    // Without this reset the head of the snake "detaches" from the snake on the Game Over, when
                    // step_cooldown < 0.0f
                    game.step_cooldown = 0.0f;
                    game.state = STATE_GAMEOVER;

                    game.dead_snake.size = game.snake.size;
                    Vec head_center = cell_center(next_head);
                    for (u32 i = 0; i < game.snake.size; ++i) {
#define GAMEOVER_EXPLOSION_RADIUS 1000.0f
#define GAMEOVER_EXPLOSION_MAX_VEL 200.0f
                        Cell cell = *ring_get(&game.snake, i);
                        game.dead_snake.items[i] = cell_rect(cell);
                        if (!cell_eq(cell, next_head)) {
                            Vec vel_vec = vec_sub(cell_center(cell), head_center);
                            f32 vel_len = vec_len(vel_vec);
                            f32 t = ilerpf(0.0f, GAMEOVER_EXPLOSION_RADIUS, vel_len);
                            if (t > 1.0f) t = 1.0f;
                            t = 1.0f - t;
                            f32 noise_x = (my_rand()%1000)*0.01;
                            f32 noise_y = (my_rand()%1000)*0.01;
                            vel_vec.x = vel_vec.x/vel_len*GAMEOVER_EXPLOSION_MAX_VEL*t + noise_x;
                            vel_vec.y = vel_vec.y/vel_len*GAMEOVER_EXPLOSION_MAX_VEL*t + noise_y;
                            game.dead_snake.vels[i] = vel_vec;
                            // TODO: additional velocities along the body of the dead snake
                        } else {
                            game.dead_snake.vels[i].x = 0;
                            game.dead_snake.vels[i].y = 0;
                        }

                        // @tail-ignore
                        if (i > 0) {
                            game.dead_snake.masks[i] = 0;
                            if (i > 1) {
                                game.dead_snake.masks[i] |= 1 << cells_dir(cell, *ring_get(&game.snake, i - 1));
                            }
                            if (i < game.snake.size - 1) {
                                game.dead_snake.masks[i] |= 1 << cells_dir(cell, *ring_get(&game.snake, i + 1));
                            }
                        }
                        if (i == game.snake.size - 1) {
                            game.dead_snake.masks[i] |= 1 << game.dir;
                        }
                    }

                    game.dead_snake.masks[next_head_index] |= 1 << cells_dir(
                                *ring_get(&game.snake, next_head_index),
                                *ring_get(&game.snake, game.snake.size - 1));

                    return;
                } else {
                    ring_push_back(&game.snake, next_head);
                    ring_pop_front(&game.snake);
                    game.eating_egg = FALSE;
                }
            }

            game.step_cooldown = STEP_INTEVAL;
        }
    }
    break;

    case STATE_PAUSE: {
        if (IsKeyPressed(KEY_SPACE)) {
            game.state = STATE_GAMEPLAY;
        }
        if (IsKeyPressed(KEY_R)) {
            game_restart(game.width, game.height);
        }
    } break;

    case STATE_GAMEOVER: {
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) {
            game_restart(game.width, game.height);
        }

        // @tail-ignore
        for (u32 i = 1; i < game.dead_snake.size; ++i) {
            game.dead_snake.vels[i].x *= 0.99f;
            game.dead_snake.vels[i].y *= 0.99f;
            game.dead_snake.items[i].x += game.dead_snake.vels[i].x*dt;
            game.dead_snake.items[i].y += game.dead_snake.vels[i].y*dt;
        }
    }
    break;

    default: {
        UNREACHABLE();
    }
    }
}

#define FACTOR 100
#define WIDTH (16*FACTOR)
#define HEIGHT (9*FACTOR)

static Font font = {0};

void platform_fill_rect(i32 x, i32 y, i32 w, i32 h, u32 color)
{
    DrawRectangle(x, y, w, h, *(Color*)&color);
}

void platform_stroke_rect(i32 x, i32 y, i32 w, i32 h, u32 color)
{
    DrawRectangleLines(x, y, w, h, *(Color*)&color);
}

u32 platform_text_width(const char *text, u32 size)
{
    return MeasureText(text, size);
}

void platform_fill_text(i32 x, i32 y, const char *text, u32 fontSize, u32 color)
{
    Vector2 size = MeasureTextEx(font, text, fontSize, 0);
    Vector2 position = {.x = x, .y = y - size.y};
    DrawTextEx(font, text, position, fontSize, 0.0, *(Color*)&color);
}

void platform_log(const char *message)
{
    TraceLog(LOG_INFO, "%s", message);
}

void GameFrame(void)
{
    BeginDrawing();
    game_update(GetFrameTime());
    game_render();
    EndDrawing();
}

void raylib_js_set_entry(void (*entry)(void));

int main(void)
{
    InitWindow(WIDTH, HEIGHT, "Snake");
    game_init(WIDTH, HEIGHT);

    font = LoadFontEx("fonts/AnekLatin-Light.ttf", 48, NULL, 0);
    GenTextureMipmaps(&font.texture);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

#ifdef PLATFORM_WEB
    raylib_js_set_entry(GameFrame);
#else
    while (!WindowShouldClose()) {
        GameFrame();
    }

    CloseWindow();
#endif

    return 0;
}

// TODO: inifinite field mechanics
// TODO: starvation mechanics
// TODO: bug on wrapping around when eating the first egg
