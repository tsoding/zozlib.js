/*******************************************************************************************
*
*   raylib [textures] example - Texture loading and drawing
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
Texture2D texture = {0};

void GameFrame() {
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawTexture(texture, screenWidth/2 - texture.width/2, screenHeight/2 - texture.height/2, WHITE);

        DrawText("this IS a texture!", 360, 370, 10, GRAY);

    EndDrawing();
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------

    InitWindow(screenWidth, screenHeight, "raylib [textures] example - texture loading and drawing");

    // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    texture = LoadTexture("resources/raylib_logo.png");        // Texture loading
    //---------------------------------------------------------------------------------------

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
