#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void raylib_js_set_entry(void (*entry)(void));

const double time_per_calc = 1; // in s
double time_since_last_calc = 0;

char buf[128];

char *buf_dym = NULL;
int counter = 1;

void GameFrame()
{
  BeginDrawing();

  ClearBackground((Color){20, 20, 20, 255});

  const size_t w = GetScreenWidth();
  const size_t h = GetScreenHeight();
  const size_t font_size = 72;

  const int ch = h / 2 - (font_size / 2);

  buf_dym = realloc(buf_dym, counter + 1);

  memset(buf_dym, 'a', counter);
  buf_dym[counter] = '\0';
  size_t text_size = MeasureText(buf_dym, font_size);
  int cw = w / 2 - (text_size / 2);
  DrawText(buf_dym, cw, ch - font_size, font_size, RED);

  sprintf(buf, "malloc size: %d%c", counter + 1, '\0');
  size_t m_text_size = MeasureText(buf, font_size);
  cw = w / 2 - (m_text_size / 2);
  DrawText(buf, cw, ch + font_size, font_size, RED);

  if (text_size > w)
  {
    counter = 1;
    free(buf_dym);
    buf_dym = NULL;
  }

  if (time_since_last_calc >= time_per_calc)
  {
    counter++;
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
  InitWindow(800, 600, "Hello, with malloc");
  SetTargetFPS(60);

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
