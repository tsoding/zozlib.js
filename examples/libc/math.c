#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void raylib_js_set_entry(void (*entry)(void));

const double time_per_calc = 2; // in s
double time_since_last_calc = 0;
char buf[BUFSIZ];
double random = 0;
void GameFrame()
{
  BeginDrawing();

  ClearBackground((Color){20, 20, 20, 255});

  const size_t w = GetScreenWidth();
  const size_t h = GetScreenHeight();
  const size_t font_size = 48;

  const int ch = h / 2 - (font_size / 2);

  sprintf(buf, "cos(%.2g) = %.2g%c", random, cos(random), '\0');
  size_t text_size = MeasureText(buf, font_size);
  int cw = w / 2 - (text_size / 2);
  DrawText(buf, cw, ch - font_size, font_size, RED);

  sprintf(buf, "sin(%.2g) = %.2g%c", random, sin(random), '\0');
  text_size = MeasureText(buf, font_size);
  cw = w / 2 - (text_size / 2);
  DrawText(buf, cw, ch + font_size, font_size, RED);

  if (time_since_last_calc >= time_per_calc)
  {
    const int random_int = rand();
    random = ((random_int / (double)RAND_MAX) - 0.5) * 2 * 4;

    time_since_last_calc = 0;
  }
  else
  {
    time_since_last_calc += GetFrameTime();
  }

  EndDrawing();
}

int main()
{
  InitWindow(800, 600, "Hello, with math.h");
  SetTargetFPS(60);

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
