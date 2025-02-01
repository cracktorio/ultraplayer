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

// Define button rectangles
Rectangle pauseButton = { 20, 0, 100, 30 }; // Y will be set later
Rectangle combatButton = { 0, 0, 100, 30 }; // Position will be set later
Rectangle segmentButton = { 0, 0, 100, 30 }; // Position will be set later
bool persistentCombat = false; // New global variable for combat state
bool showSegmentMenu = false; // Add at top with other globals

// Add after initial variable declarations
float scrollY = 0.0f;
const float scrollSpeed = 30.0f;

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

    // Update button positions after window creation
    const int controlPanelHeight = 50;
    pauseButton.y = screenHeight - controlPanelHeight + 10;
    combatButton.x = pauseButton.x + pauseButton.width + 20;
    combatButton.y = pauseButton.y;
    segmentButton.x = screenWidth - 120; // New position from right edge
    segmentButton.y = pauseButton.y;

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

    // After button layout configuration
    int totalRows = (levelCount + buttonsPerRow - 1) / buttonsPerRow;
    int contentHeight = totalRows * (buttonHeight + padding) + padding;
    int startX = (screenWidth - (buttonsPerRow * (buttonWidth + padding) - padding)) / 2;

    // --- Define control panel dimensions (bottom of the window) ---
    // Pause/Resume button inside the control panel
    // Combat mode button (Initially hidden)
    // Progress bar (always visible)
    Rectangle progressBar = { 250, screenHeight - controlPanelHeight + 20, 400, 10 };

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update currently playing music stream if one is active
        if (currentPlaying != -1)
        {
            Segment *currentSeg = &levels[currentPlaying].segments[levels[currentPlaying].currentSegment];
            UpdateMusicStream(currentSeg->free);
            if (currentSeg->hasCombat) {
                UpdateMusicStream(currentSeg->combat);
            }
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

                if (CheckCollisionPointRec(mousePoint, buttonRec) && 
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                    mousePoint.y < screenHeight - controlPanelHeight &&
                    !showSegmentMenu) {  // Only allow selection if menu is closed
                    // Stop any currently playing music
                    if (currentPlaying != -1)
                    {
                        Segment *oldSeg = &levels[currentPlaying].segments[levels[currentPlaying].currentSegment];
                        StopMusicStream(oldSeg->free);
                        if (oldSeg->hasCombat) {
                            StopMusicStream(oldSeg->combat);
                        }
                    }

                    // Start playing the new level's first segment
                    currentPlaying = i;
                    levels[currentPlaying].currentSegment = 0;
                    Segment *newSeg = &levels[currentPlaying].segments[0];
                    
                    // Start both streams if combat music exists
                    PlayMusicStream(newSeg->free);
                    if (newSeg->hasCombat) {
                        PlayMusicStream(newSeg->combat);
                        // Set initial volumes based on persistent combat state
                        if (persistentCombat) {
                            SetMusicVolume(newSeg->free, 0.0f);
                            SetMusicVolume(newSeg->combat, 1.0f);
                        } else {
                            SetMusicVolume(newSeg->free, 1.0f);
                            SetMusicVolume(newSeg->combat, 0.0f);
                        }
                    }
                    isPaused = false;
                    showSegmentMenu = false;
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
                    PauseMusicStream(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].free);
                    if (levels[currentPlaying].segments[levels[currentPlaying].currentSegment].hasCombat) {
                        PauseMusicStream(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].combat);
                    }
                }
                else
                {
                    ResumeMusicStream(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].free);
                    if (levels[currentPlaying].segments[levels[currentPlaying].currentSegment].hasCombat) {
                        ResumeMusicStream(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].combat);
                    }
                }
            }
        }

        // Check Combat button click – switch to combat mode if not already active
        if (CheckCollisionPointRec(mousePoint, combatButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            persistentCombat = !persistentCombat;
            if (currentPlaying != -1)
            {
                Segment *currentSeg = &levels[currentPlaying].segments[levels[currentPlaying].currentSegment];
                if (currentSeg->hasCombat)
                {
                    if (persistentCombat)
                    {
                        SetMusicVolume(currentSeg->free, 0.0f);
                        SetMusicVolume(currentSeg->combat, 1.0f);
                    }
                    else
                    {
                        SetMusicVolume(currentSeg->free, 1.0f);
                        SetMusicVolume(currentSeg->combat, 0.0f);
                    }
                }
            }
        }

        // Handle segment button click
        if (currentPlaying != -1 && 
            CheckCollisionPointRec(mousePoint, segmentButton) && 
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            showSegmentMenu = !showSegmentMenu;
        }

        // Handle segment selection if menu is shown
        if (showSegmentMenu && currentPlaying != -1) {
            // Calculate maximum segment name width
            float maxWidth = segmentButton.width;
            for (int i = 0; i < levels[currentPlaying].segmentCount; i++) {
                float nameWidth = MeasureText(levels[currentPlaying].segments[i].name, 10);
                maxWidth = fmaxf(maxWidth, nameWidth + 20); // Add padding
            }

            // Menu background - align right edge with button
            Rectangle menuBg = {
                segmentButton.x + segmentButton.width - maxWidth - 10,
                segmentButton.y - (levels[currentPlaying].segmentCount * 40),
                maxWidth + 10, // Width based on longest name
                levels[currentPlaying].segmentCount * 40 + 5
            };
            DrawRectangleRec(menuBg, DARKGRAY);

            // Draw segments
            for (int i = 0; i < levels[currentPlaying].segmentCount; i++) {
                Rectangle segmentRec = {
                    menuBg.x + 5,
                    segmentButton.y - ((i + 1) * 40) + 5,
                    maxWidth,
                    30
                };

                // Check for mouse hover
                Vector2 mousePoint = GetMousePosition();
                bool isHovered = CheckCollisionPointRec(mousePoint, segmentRec);
                bool isSelected = (i == levels[currentPlaying].currentSegment);

                // Draw segment background
                if (isSelected) {
                    DrawRectangleRec(segmentRec, WHITE);       // White background for selected
                    DrawRectangleLinesEx(segmentRec, 1, GRAY); // Gray border
                } else {
                    DrawRectangleRec(segmentRec, isHovered ? LIGHTGRAY : GRAY);
                }

                // Draw segment name with consistent left padding
                DrawText(levels[currentPlaying].segments[i].name,
                        segmentRec.x + 10,
                        segmentRec.y + (segmentRec.height - 10) / 2,
                        10,
                        isSelected ? BLACK : WHITE);

                if (CheckCollisionPointRec(mousePoint, segmentRec) && 
                    IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    // Stop current segment
                    Segment *oldSeg = &levels[currentPlaying].segments[levels[currentPlaying].currentSegment];
                    StopMusicStream(oldSeg->free);
                    if (oldSeg->hasCombat) {
                        StopMusicStream(oldSeg->combat);
                    }

                    // Switch to new segment
                    levels[currentPlaying].currentSegment = i;
                    Segment *newSeg = &levels[currentPlaying].segments[i];
                    PlayMusicStream(newSeg->free);
                    if (newSeg->hasCombat) {
                        PlayMusicStream(newSeg->combat);
                        if (persistentCombat) {
                            SetMusicVolume(newSeg->free, 0.0f);
                            SetMusicVolume(newSeg->combat, 1.0f);
                        } else {
                            SetMusicVolume(newSeg->free, 1.0f);
                            SetMusicVolume(newSeg->combat, 0.0f);
                        }
                    }
                    showSegmentMenu = false;
                }
            }
        }

        // Add click-outside handler after segment menu code
        if (showSegmentMenu && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // Calculate menu bounds
            float maxWidth = segmentButton.width;
            for (int i = 0; i < levels[currentPlaying].segmentCount; i++) {
                float nameWidth = MeasureText(levels[currentPlaying].segments[i].name, 10);
                maxWidth = fmaxf(maxWidth, nameWidth + 20);
            }
            
            Rectangle menuBounds = {
                segmentButton.x + segmentButton.width - maxWidth - 10,
                segmentButton.y - (levels[currentPlaying].segmentCount * 40),
                maxWidth + 10,
                levels[currentPlaying].segmentCount * 40 + 5
            };
            
            // Close menu if clicked outside
            if (!CheckCollisionPointRec(mousePoint, menuBounds) &&
                !CheckCollisionPointRec(mousePoint, segmentButton)) {
                showSegmentMenu = false;
            }
        }

        // In main game loop, before drawing
        // Handle scrolling
        if (GetMouseWheelMove() != 0) {
            scrollY += GetMouseWheelMove() * scrollSpeed;
            // Limit scrolling
            float maxScroll = 0.0f;
            float minScroll = -(contentHeight - (screenHeight - controlPanelHeight));
            if (minScroll > 0) minScroll = 0;
            if (scrollY < minScroll) scrollY = minScroll;
            if (scrollY > maxScroll) scrollY = maxScroll;
        }

        // --- Draw ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Update level button drawing code
        for (int i = 0; i < levelCount; i++)
        {
            int row = i / buttonsPerRow;
            int col = i % buttonsPerRow;
            int x = startX + col * (buttonWidth + padding);
            int y = padding + row * (buttonHeight + padding) + scrollY;
            
            // Skip if button is outside visible area
            if (y + buttonHeight < 0 || y > screenHeight - controlPanelHeight) {
                continue;
            }
            
            Rectangle buttonRec = { (float)x, (float)y, (float)buttonWidth, (float)buttonHeight };
            
            // Update click detection to account for scroll position
            if (CheckCollisionPointRec(mousePoint, buttonRec) && 
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                mousePoint.y < screenHeight - controlPanelHeight &&
                !showSegmentMenu) {  // Only allow selection if menu is closed
                // Stop any currently playing music
                if (currentPlaying != -1)
                {
                    Segment *oldSeg = &levels[currentPlaying].segments[levels[currentPlaying].currentSegment];
                    StopMusicStream(oldSeg->free);
                    if (oldSeg->hasCombat) {
                        StopMusicStream(oldSeg->combat);
                    }
                }

                // Start playing the new level's first segment
                currentPlaying = i;
                levels[currentPlaying].currentSegment = 0;
                Segment *newSeg = &levels[currentPlaying].segments[0];
                
                // Start both streams if combat music exists
                PlayMusicStream(newSeg->free);
                if (newSeg->hasCombat) {
                    PlayMusicStream(newSeg->combat);
                    // Set initial volumes based on persistent combat state
                    if (persistentCombat) {
                        SetMusicVolume(newSeg->free, 0.0f);
                        SetMusicVolume(newSeg->combat, 1.0f);
                    } else {
                        SetMusicVolume(newSeg->free, 1.0f);
                        SetMusicVolume(newSeg->combat, 0.0f);
                    }
                }
                isPaused = false;
                showSegmentMenu = false;
            }
            
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

        // Always draw combat button when a level is playing
        if (currentPlaying != -1) {
            Segment *currentSeg = &levels[currentPlaying].segments[levels[currentPlaying].currentSegment];
            Color buttonColor = GRAY;
            
            if (currentSeg->hasCombat) {
                buttonColor = persistentCombat ? RED : GRAY;
            } else {
                buttonColor = DARKGRAY; // Grayed out when no combat available
            }
            
            DrawRectangleRec(combatButton, buttonColor);
            const char* combatText = "Combat";
            int combatTextWidth = MeasureText(combatText, 10);
            DrawText(combatText, 
                    combatButton.x + (combatButton.width - combatTextWidth) / 2,
                    combatButton.y + (combatButton.height - 10) / 2, 
                    10, WHITE);
        }

        // Draw segment button and menu
        if (currentPlaying != -1 && levels[currentPlaying].segmentCount > 1) {
            // Calculate maximum segment name width first
            float maxWidth = segmentButton.width;
            for (int i = 0; i < levels[currentPlaying].segmentCount; i++) {
                float nameWidth = MeasureText(levels[currentPlaying].segments[i].name, 10);
                maxWidth = fmaxf(maxWidth, nameWidth + 20);
            }

            // Main segment button
            DrawRectangleRec(segmentButton, GRAY);
            const char* segText = "Segments";
            int segTextWidth = MeasureText(segText, 10);
            DrawText(segText, 
                    segmentButton.x + (segmentButton.width - segTextWidth) / 2,
                    segmentButton.y + (segmentButton.height - 10) / 2, 
                    10, WHITE);

            if (showSegmentMenu) {
                Rectangle menuBg = {
                    segmentButton.x + segmentButton.width - maxWidth - 10,
                    segmentButton.y - (levels[currentPlaying].segmentCount * 40),
                    maxWidth + 10,
                    levels[currentPlaying].segmentCount * 40 + 5
                };
                DrawRectangleRec(menuBg, DARKGRAY);

                // Update segment selection handler to use same coordinates
                for (int i = 0; i < levels[currentPlaying].segmentCount; i++) {
                    Rectangle segmentRec = {
                        menuBg.x + 5,
                        segmentButton.y - ((i + 1) * 40) + 5,
                        maxWidth,
                        30
                    };

                    // Check for mouse hover and draw segment
                    Vector2 mousePoint = GetMousePosition();
                    bool isHovered = CheckCollisionPointRec(mousePoint, segmentRec);
                    bool isSelected = (i == levels[currentPlaying].currentSegment);

                    if (isSelected) {
                        DrawRectangleRec(segmentRec, WHITE);
                        DrawRectangleLinesEx(segmentRec, 1, GRAY);
                    } else {
                        DrawRectangleRec(segmentRec, isHovered ? LIGHTGRAY : GRAY);
                    }

                    DrawText(levels[currentPlaying].segments[i].name,
                            segmentRec.x + 10,
                            segmentRec.y + (segmentRec.height - 10) / 2,
                            10,
                            isSelected ? BLACK : WHITE);

                    if (CheckCollisionPointRec(mousePoint, segmentRec) && 
                        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        // Stop current segment
                        Segment *oldSeg = &levels[currentPlaying].segments[levels[currentPlaying].currentSegment];
                        StopMusicStream(oldSeg->free);
                        if (oldSeg->hasCombat) {
                            StopMusicStream(oldSeg->combat);
                        }

                        // Switch to new segment
                        levels[currentPlaying].currentSegment = i;
                        Segment *newSeg = &levels[currentPlaying].segments[i];
                        PlayMusicStream(newSeg->free);
                        if (newSeg->hasCombat) {
                            PlayMusicStream(newSeg->combat);
                            if (persistentCombat) {
                                SetMusicVolume(newSeg->free, 0.0f);
                                SetMusicVolume(newSeg->combat, 1.0f);
                            } else {
                                SetMusicVolume(newSeg->free, 1.0f);
                                SetMusicVolume(newSeg->combat, 0.0f);
                            }
                        }
                        showSegmentMenu = false;
                    }
                }
            }
        }

        // If a track is playing, draw its progress
        if (currentPlaying != -1)
        {
            DrawRectangleRec(progressBar, LIGHTGRAY);

            float musicLength = inCombat ? GetMusicTimeLength(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].combat)
                                         : GetMusicTimeLength(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].free);
            float musicTime = inCombat ? GetMusicTimePlayed(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].combat)
                                       : GetMusicTimePlayed(levels[currentPlaying].segments[levels[currentPlaying].currentSegment].free);
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
