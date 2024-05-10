#include "raylib.h"

void raylib_js_set_entry(void (*entry)(void));

unsigned int framesCounter = 0;
int r = 150;
int g = 150;
int b = 150;

void GameFrame()
{
    BeginDrawing();
    ClearBackground(CLITERAL(Color){r, g, b, 255});
    framesCounter++;

    if (((framesCounter / 120) % 2) == 1)
    {
      r = GetRandomValue(50, 255);
      g = GetRandomValue(50, 255);
      b = GetRandomValue(50, 255);
      framesCounter = 0;
    }

    ClearBackground(CLITERAL(Color){r, g, b, 255});
    DrawText("Every 2 seconds a new random color is generated", 130, 100, 20, BLACK);
    EndDrawing();
}

int main(void)
{
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "Random colors");

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