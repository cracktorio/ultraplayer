#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "./cjson/cJSON.h"

// Include our header for helper functions and types.
#include "functions.h"

// Include the functions source file so that all functions are compiled together.
#include "functions.c"

//------------------------------------------------------------------------------------
// Main Entry Point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization of raylib window and audio device
    const int screenWidth = 800;
    const int screenHeight = 450;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "ultraplayer");
    InitAudioDevice();
    SetTargetFPS(60);

    // Variables for level selection and controls
    Level levels[MAX_LEVELS];
    int levelCount = 0;
    int currentPlaying = -1;  // Index of currently playing music, -1 = none
    bool isPaused = false;    // Whether the current track is paused
    bool inCombat = false;    // Whether combat mode is active
    bool hasCombatMusic = false; // Whether the current level has combat music

    // --- Load and Parse JSON Data ---
    if (!ParseJSONData("data.json", levels, &levelCount))
    {
        // Error message is printed inside ParseJSONData; exit if failed.
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    // --- Button layout configuration for level selection ---
    const int buttonWidth = 150;
    const int buttonHeight = 150;
    const int padding = 20;
    int buttonsPerRow = screenWidth / (buttonWidth + padding);

    // --- Define control panel dimensions (bottom of the window) ---
    const int controlPanelHeight = 50;
    // Pause/Resume button inside the control panel
    Rectangle pauseButton = { 20, screenHeight - controlPanelHeight + 10, 100, 30 };
    // Combat mode button (Initially hidden)
    Rectangle combatButton = { pauseButton.x + pauseButton.width + 20, pauseButton.y, 100, 30 };
    // Progress bar (always visible)
    Rectangle progressBar = { 250, screenHeight - controlPanelHeight + 20, 400, 10 };

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update currently playing music stream if one is active
        if (currentPlaying != -1)
        {
            UpdateMusicStream(levels[currentPlaying].combatMusic);
            UpdateMusicStream(levels[currentPlaying].music);
        }

        // --- Handle Input for Level Buttons ---
        Vector2 mousePoint = GetMousePosition();
        if (mousePoint.y < screenHeight - controlPanelHeight)  // Ignore clicks in the control panel
        {
            for (int i = 0; i < levelCount; i++)
            {
                int row = i / buttonsPerRow;
                int col = i % buttonsPerRow;
                int x = padding + col * (buttonWidth + padding);
                int y = padding + row * (buttonHeight + padding);
                Rectangle buttonRec = { (float)x, (float)y, (float)buttonWidth, (float)buttonHeight };

                if (CheckCollisionPointRec(mousePoint, buttonRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    // Stop any currently playing music
                    if (currentPlaying != -1)
                    {
                        StopMusicStream(levels[currentPlaying].combatMusic);
                        StopMusicStream(levels[currentPlaying].music);
                    }

                    // Play the selected level's music while retaining combat mode
                    if (inCombat)
                    {
                        SetMusicVolume(levels[i].combatMusic, 1.0);
                        SetMusicVolume(levels[i].music, 0.0);
                    }
                    else
                    {
                        SetMusicVolume(levels[i].combatMusic, 0.0);
                        SetMusicVolume(levels[i].music, 1.0);
                    }

                    PlayMusicStream(levels[i].music);
                    PlayMusicStream(levels[i].combatMusic);
                    currentPlaying = i;
                    isPaused = false;

                    // **Check if this level has a combat track**
                    hasCombatMusic = (levels[i].combatMusic.stream.buffer != NULL);
                }
            }
        }

        // --- Handle Input for Control Panel ---
        // Check Pause/Resume button click
        if (CheckCollisionPointRec(mousePoint, pauseButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            isPaused = !isPaused;
            if (currentPlaying != -1)
            {
                if (isPaused)
                {
                    PauseMusicStream(levels[currentPlaying].combatMusic);
                    PauseMusicStream(levels[currentPlaying].music);
                }
                else
                {
                    ResumeMusicStream(levels[currentPlaying].combatMusic);
                    ResumeMusicStream(levels[currentPlaying].music);
                }
            }
        }

        // Check Combat button click – switch to combat mode if not already active
        if (hasCombatMusic && CheckCollisionPointRec(mousePoint, combatButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (currentPlaying != -1)
            {
                if (inCombat)
                {   // Switch to free music if in combat
                    SetMusicVolume(levels[currentPlaying].combatMusic, 0.0);
                    SetMusicVolume(levels[currentPlaying].music, 1.0);
                    inCombat = false;
                }
                else
                {
                    SetMusicVolume(levels[currentPlaying].combatMusic, 1.0);
                    SetMusicVolume(levels[currentPlaying].music, 0.0);
                    inCombat = true;
                }
            }
        }

        // --- Draw ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Draw level buttons (only in the upper part of the window)
        for (int i = 0; i < levelCount; i++)
        {
            int row = i / buttonsPerRow;
            int col = i % buttonsPerRow;
            int x = padding + col * (buttonWidth + padding);
            int y = padding + row * (buttonHeight + padding);
            Rectangle buttonRec = { (float)x, (float)y, (float)buttonWidth, (float)buttonHeight };

            // Draw a border (highlighted if this level is playing)
            Color borderColor = (i == currentPlaying) ? RED : BLACK;
            DrawRectangleLines(x, y, buttonWidth, buttonHeight, borderColor);

            // Draw the thumbnail image if valid, scaled to fit
            if (levels[i].thumbnail.id != 0)
            {
                float scale = fminf((buttonWidth - 20) / (float)levels[i].thumbnail.width,
                                    (buttonHeight - 40) / (float)levels[i].thumbnail.height);
                int drawWidth = levels[i].thumbnail.width * scale;
                int drawHeight = levels[i].thumbnail.height * scale;
                int drawX = x + (buttonWidth - drawWidth) / 2;
                int drawY = y + 10;
                DrawTextureEx(levels[i].thumbnail, (Vector2){ drawX, drawY }, 0.0f, scale, WHITE);
            }

            // Draw the level name in white with auto-wrap.
            Rectangle textRec = { (float)x+padding/2, (float)(y + buttonHeight - 30), (float)buttonWidth-padding, 30 };
            DrawTextRec(GetFontDefault(), levels[i].name, textRec, 10, 1.0f, true, WHITE);
        }

        // --- Draw Control Panel ---
        DrawRectangle(0, screenHeight - controlPanelHeight, screenWidth, controlPanelHeight, RAYWHITE);

        // Draw the Pause/Resume button
        DrawRectangleRec(pauseButton, GRAY);
        const char *pauseText = isPaused ? "Resume" : "Pause";
        int pauseTextWidth = MeasureText(pauseText, 10);
        DrawText(pauseText, pauseButton.x + (pauseButton.width - pauseTextWidth) / 2,
                 pauseButton.y + (pauseButton.height - 10) / 2, 10, WHITE);

        // **Only draw combat button if the level has combat music**
        if (hasCombatMusic)
        {
            DrawRectangleRec(combatButton, GRAY);
            int combatTextWidth = MeasureText("Toggle Combat", 10);
            DrawText("Toggle Combat", combatButton.x + (combatButton.width - combatTextWidth) / 2,
                     combatButton.y + (combatButton.height - 10) / 2, 10, WHITE);
        }

        // If a track is playing, draw its progress
        if (currentPlaying != -1)
        {
            DrawRectangleRec(progressBar, LIGHTGRAY);

            float musicLength = inCombat ? GetMusicTimeLength(levels[currentPlaying].combatMusic)
                                         : GetMusicTimeLength(levels[currentPlaying].music);
            float musicTime = inCombat ? GetMusicTimePlayed(levels[currentPlaying].combatMusic)
                                       : GetMusicTimePlayed(levels[currentPlaying].music);
            float progress = (musicLength > 0.0f) ? (musicTime / musicLength) : 0.0f;
            Rectangle progressFill = progressBar;
            progressFill.width = progress * progressBar.width;
            DrawRectangleRec(progressFill, RED);
            
            // Optionally draw the elapsed time as text.
            char timeText[64];
            snprintf(timeText, sizeof(timeText), "%.0f / %.0f sec", musicTime, musicLength);
            int timeTextWidth = MeasureText(timeText, 10);
            DrawText(timeText, progressBar.x + (progressBar.width - timeTextWidth) / 2,
                     progressBar.y - 12, 10, BLACK);
        }

        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
