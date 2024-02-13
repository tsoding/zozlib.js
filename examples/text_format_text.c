/*******************************************************************************************
*
*   raylib [text] example - Text formatting
*
*   Example originally created with raylib 1.1, last time updated with raylib 3.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2014-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

void raylib_js_set_entry(void (*entry)(void));

int score = 100020;
int hiscore = 200450;
int lives = 5;
int inc = 0;

void GameFrame()
{
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------

    int inc_changed = 0;

    if (IsKeyDown(KEY_SPACE)){
        inc_changed = 1;
        inc++;
    } 
    if (IsKeyDown(KEY_Q)) {
        TraceLog(LOG_INFO, "raylib [%s] example - %s is shutting down...", "text", "text formatting");
        TraceLog(LOG_FATAL, "This is a fatal message: %d", inc);
    }

    SetTraceLogLevel(LOG_DEBUG);

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText(TextFormat("Score: %08i", score), 200, 80, 20, RED);

        DrawText(TextFormat("HiScore: %08i", hiscore), 200, 120, 20, GREEN);

        DrawText(TextFormat("Lives: %02i", lives), 200, 160, 40, BLUE);

        DrawText(TextFormat("Elapsed Time: %02.02f ms", GetFrameTime()*1000), 200, 220, 20, BLACK);
        
        DrawText(TextFormat("Hold Space To Increment: 0x%02X", inc % 255), 200, 280, 20, DARKPURPLE);

        DrawText(TextFormat("Press '%c' To Log Fatal and Terminate the Program", 'Q'), 200, 340, 20, RED);

        const char* text = "Hello :)";
        DrawText(TextFormat("'%s' is stored at address %p", text, text), 200, 400, 20, GOLD);
        
        if(inc_changed){
            TraceLog(LOG_TRACE, "This is an trace message and should not be printed");
            TraceLog(LOG_DEBUG, "This is a debug message: %d", inc);
        }

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

    InitWindow(screenWidth, screenHeight, "raylib [text] example - text formatting");

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