#include "raylib.h"
#include "raqm_raylib.h"

int main(void)
{
    InitWindow(800, 450, "Raqm Raylib - Text Shaping");

    BeginDrawing();
        ClearBackground(RAYWHITE);
        const Vector2 textSize = MeasureTextEx(GetFontDefault(), "loading ....", 20, 0);
        const Vector2 pos = {
            .x = 800 / 2 - textSize.x / 2,
            .y = 450 / 2 - textSize.y / 2};
        DrawTextEx(GetFontDefault(), "loading ....", pos, 20, 0, DARKGRAY);
    EndDrawing();

    RaqmRayFont font = {0};

    if (!RaqmRayFont_Load(&font, "resources/fonts/BalooTamma2-Regular.ttf", 100))
    {
        CloseWindow();
        return -1;
    }

    const char *kannadaText = "ಕಾಯಕವೇ ಕೈಲಾಸ";

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // RaqmRaylib text with shaping.
        RaqmRay_DrawTextPro(&font, kannadaText, (Vector2){50, 0}, (Vector2){0, 0}, 0, 70, 10, ORANGE);
        RaqmRay_DrawTextEx(&font, kannadaText, (Vector2){50, 100}, 70, 0, RED);
        RaqmRay_DrawText(&font, kannadaText, 50, 200, 70, BLUE);
        
        // Raylib 
        DrawText("This uses text shaping to render the above texts !", 50, 400, 20, DARKGRAY);

        EndDrawing();
    }

    RaqmRayFont_Unload(&font);
    CloseWindow();
    return 0;
}