/*******************************************************************************************
*
*   raylib [text] example - Text Writing Animation
*
*   Example originally created with raylib 1.4, last time updated with raylib 1.4
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2016-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

void raylib_js_set_entry(void (*entry)(void));

const int screenWidth = 800;
const int screenHeight = 450;

const char message[128] = "This sample illustrates a text writing\nanimation effect! Check it out! ;)";
int framesCounter = 0;

void GameFrame() {
    // Update
    //----------------------------------------------------------------------------------
    if (IsKeyDown(KEY_SPACE)) framesCounter += 8;
    else framesCounter++;

    if (IsKeyPressed(KEY_ENTER)) framesCounter = 0;
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText(TextSubtext(message, 0, framesCounter/10), 210, 160, 20, MAROON);

        DrawText("PRESS [ENTER] to RESTART!", 240, 260, 20, LIGHTGRAY);
        DrawText("PRESS [SPACE] to SPEED UP!", 239, 300, 20, LIGHTGRAY);

    EndDrawing();
    //----------------------------------------------------------------------------------
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    InitWindow(screenWidth, screenHeight, "raylib [text] example - text writing anim");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

#ifdef PLATFORM_WEB
    raylib_js_set_entry(GameFrame);
#else
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        GameFrame();
    }
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
#endif

    return 0;
}
