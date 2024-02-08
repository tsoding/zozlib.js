/*******************************************************************************************
*
*   raylib [core] example - Keyboard input
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

void raylib_js_set_entry(void (*entry)(void));

const int screenWidth = 800;
const int screenHeight = 450;
int boxPositionY = screenHeight/2 - 40;
int scrollSpeed = 4;            // Scrolling speed in pixels
Vector2 ballPosition = { 0 };

void GameFrame()
{
    // Update
    //----------------------------------------------------------------------------------
    boxPositionY -= (GetMouseWheelMove()*scrollSpeed);
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawRectangle(screenWidth/2 - 40, boxPositionY, 80, 80, MAROON);

        DrawText("Use mouse wheel to move the cube up and down!", 10, 10, 20, GRAY);
        // TODO: implement TextFormat
        // DrawText(TextFormat("Box position Y: %03i", boxPositionY), 10, 40, 20, LIGHTGRAY);

    EndDrawing();
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "raylib [core] example - input mouse wheel");

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
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
#endif
    return 0;
}