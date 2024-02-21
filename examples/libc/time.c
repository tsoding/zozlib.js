#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

void raylib_js_set_entry(void (*entry)(void));

char buf[BUFSIZ];

void GameFrame()
{
  BeginDrawing();

  ClearBackground((Color){20, 20, 20, 255});

  const size_t w = GetScreenWidth();
  const size_t h = GetScreenHeight();
  const size_t font_size = 48;

  const int ch = h / 2 - (font_size / 2);

  time_t t = time(NULL);
  printf("%lli\n", t);

  struct tm *ts = localtime(&t);
  printf("%i\n", ts->tm_mday);

  char *time_text = asctime(ts); // ctime(&t);
  sprintf(buf, "%s%c", time_text, '\0');

  size_t text_size = MeasureText(buf, font_size);
  int cw = w / 2 - (text_size / 2);
  DrawText(buf, cw, ch, font_size, RED);

  EndDrawing();
}

int main()
{
  InitWindow(800, 600, "Hello, with math.h");
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