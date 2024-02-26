#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdbool.h>

void raylib_js_set_entry(void (*entry)(void));

bool once = true;
FILE *read_me = NULL;

char buf[BUFSIZ];
const double time_per_calc = 2; // in s
double time_since_last_calc = 0;

void GameFrame()
{
  BeginDrawing();

  ClearBackground((Color){20, 20, 20, 255});

  const size_t w = GetScreenWidth();
  const size_t h = GetScreenHeight();
  const size_t font_size = 48;

  if (time_since_last_calc >= time_per_calc)
  {
    // Get new line
    if (is_file_ready(read_me))
    {
      if (feof(read_me))
      {
        rewind(read_me);
      }

      fgets(buf, BUFSIZ, read_me);
    }

    time_since_last_calc = 0;
  }
  else
  {
    time_since_last_calc += GetFrameTime();
  }

  // display text somewhat sensibly
  size_t current_font_size = font_size;
  size_t text_size = MeasureText(buf, font_size);
  while (text_size > w && current_font_size > font_size / 2)
  {
    text_size = MeasureText(buf, --current_font_size);
  }
  if (text_size <= w)
  {
    DrawText(buf, w / 2 - (text_size / 2), h / 2 - (current_font_size / 2), current_font_size, RED);
  }
  else
  {
    DrawText(buf, 10, h / 2 - (current_font_size / 2), current_font_size, RED);
  }

  EndDrawing();
}

int main()
{
  InitWindow(800, 600, "Hello, with loaded README.md");
  SetTargetFPS(60);

  read_me = fopen("README.md", "rb");

  int w = GetScreenWidth();
  int h = GetScreenHeight();

#ifdef PLATFORM_WEB
  raylib_js_set_entry(GameFrame);
#else
  while (!WindowShouldClose())
  {
    GameFrame();
  }
  CloseWindow();
#endif
  return 0;
}
