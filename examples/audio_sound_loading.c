/*******************************************************************************************
*
*   raylib [audio] example - Sound loading and playing
*
*   Example originally created with raylib 1.1, last time updated with raylib 3.5
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

void raylib_js_set_entry(void (*entry)(void));

Sound fxWav;
Sound fxOgg;

void GameFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    if (IsKeyPressed(KEY_SPACE)) PlaySound(fxWav);      // Play WAV sound
    if (IsKeyPressed(KEY_ENTER)) PlaySound(fxOgg);      // Play OGG sound
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText("Press SPACE to PLAY the WAV sound!", 200, 180, 20, LIGHTGRAY);
        DrawText("Press ENTER to PLAY the OGG sound!", 200, 220, 20, LIGHTGRAY);

    EndDrawing();
    //----------------------------------------------------------------------------------
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [audio] example - sound loading and playing");

    InitAudioDevice();      // Initialize audio device

    fxWav = LoadSound("resources/sound.wav");         // Load WAV audio file
    fxOgg = LoadSound("resources/target.ogg");        // Load OGG audio file

    SetTargetFPS(60);               // Set desired framerate (frames-per-second)
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
    UnloadSound(fxWav);     // Unload sound data
    UnloadSound(fxOgg);     // Unload sound data

    CloseAudioDevice();     // Close audio device

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
#endif

    return 0;
}
