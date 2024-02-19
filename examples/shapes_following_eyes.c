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
#include "math.h"

void raylib_js_set_entry(void (*entry)(void));

Vector2 scleraLeftPosition;
Vector2 scleraRightPosition;
float scleraRadius;

Vector2 irisLeftPosition;
Vector2 irisRightPosition;
float irisRadius;

float angle;
float dx, dy, dxx, dyy;

void GameFrame() {
    // Update
    //----------------------------------------------------------------------------------
    irisLeftPosition = GetMousePosition();
    irisRightPosition = GetMousePosition();

    // Check not inside the left eye sclera
    if (!CheckCollisionPointCircle(irisLeftPosition, scleraLeftPosition, scleraRadius - irisRadius))
    {
        dx = irisLeftPosition.x - scleraLeftPosition.x;
        dy = irisLeftPosition.y - scleraLeftPosition.y;

        angle = atan2f(dy, dx);

        dxx = (scleraRadius - irisRadius)*cosf(angle);
        dyy = (scleraRadius - irisRadius)*sinf(angle);

        irisLeftPosition.x = scleraLeftPosition.x + dxx;
        irisLeftPosition.y = scleraLeftPosition.y + dyy;
    }

    // Check not inside the right eye sclera
    if (!CheckCollisionPointCircle(irisRightPosition, scleraRightPosition, scleraRadius - irisRadius))
    {
        dx = irisRightPosition.x - scleraRightPosition.x;
        dy = irisRightPosition.y - scleraRightPosition.y;

        angle = atan2f(dy, dx);

        dxx = (scleraRadius - irisRadius)*cosf(angle);
        dyy = (scleraRadius - irisRadius)*sinf(angle);

        irisRightPosition.x = scleraRightPosition.x + dxx;
        irisRightPosition.y = scleraRightPosition.y + dyy;
    }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawCircleV(scleraLeftPosition, scleraRadius, LIGHTGRAY);
        DrawCircleV(irisLeftPosition, irisRadius, BROWN);
        DrawCircleV(irisLeftPosition, 10, BLACK);

        DrawCircleV(scleraRightPosition, scleraRadius, LIGHTGRAY);
        DrawCircleV(irisRightPosition, irisRadius, DARKGREEN);
        DrawCircleV(irisRightPosition, 10, BLACK);

        DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [shapes] example - following eyes");

    scleraLeftPosition = CLITERAL(Vector2){ GetScreenWidth()/2.0f - 100.0f, GetScreenHeight()/2.0f };
    scleraRightPosition = CLITERAL(Vector2){ GetScreenWidth()/2.0f + 100.0f, GetScreenHeight()/2.0f };
    scleraRadius = 80;

    irisLeftPosition = CLITERAL(Vector2){ GetScreenWidth()/2.0f - 100.0f, GetScreenHeight()/2.0f };
    irisRightPosition = CLITERAL(Vector2){ GetScreenWidth()/2.0f + 100.0f, GetScreenHeight()/2.0f };
    irisRadius = 24;

    angle = 0.0f;
    float dx = 0.0f;
    dy = 0.0f;
    dxx = 0.0f;
    dyy = 0.0f;

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
