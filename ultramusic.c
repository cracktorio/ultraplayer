#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "./cjson/cJSON.h"
#include "functions.h"
#include "functions.c"

int main(void) {
    const int screenWidth = 900;
    const int screenHeight = 600;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(screenWidth, screenHeight, "ultraplayer");
    InitAudioDevice();
    SetTargetFPS(60);

    AppState state = {0};
    state.currentPlaying = -1;
    InitializeButtons(&state, screenWidth, screenHeight);

    // Load levels
    if (!ParseJSONData("data.json", state.levels, &state.levelCount)) {
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    // Level buttons layout
    const int buttonWidth = 150;
    const int buttonHeight = 150;
    const int padding = 20;
    state.buttonsPerRow = screenWidth / (buttonWidth + padding);
    state.startX = (screenWidth - (state.buttonsPerRow * (buttonWidth + padding) - padding)) / 2;

    while (!WindowShouldClose()) {
        Vector2 mousePoint = GetMousePosition();
        
        if (state.currentPlaying != -1) {
            // Check for music end and handle repeat/continue
            HandleMusicEnd(&state);
            
            // Add keyboard controls after mousePoint definition
            Level *currentLevel = &state.levels[state.currentPlaying];
            Segment *currentSeg = &currentLevel->segments[currentLevel->currentSegment];
            
            // Space to toggle pause
            if (IsKeyPressed(KEY_SPACE)) {
                state.isPaused = !state.isPaused;
                HandleMusicPause(&state);
            }
            
            // C to toggle combat music
            if (IsKeyPressed(KEY_C) && currentSeg->hasCombat) {
                state.persistentCombat = !state.persistentCombat;
                SetMusicVolume(currentSeg->free, state.persistentCombat ? 0.0f : 1.0f);
                SetMusicVolume(currentSeg->combat, state.persistentCombat ? 1.0f : 0.0f);
            }

            // Right arrow - next segment/level
            if (IsKeyPressed(KEY_RIGHT)) {
                bool levelChanged = false;
                GetNextSegment(&state, currentLevel, &levelChanged);
                
                if (levelChanged) {
                    HandleMusicTransition(&state, currentLevel, 
                                       &state.levels[state.currentPlaying], 0);
                } else if (currentLevel->currentSegment < currentLevel->segmentCount) {
                    HandleMusicTransition(&state, currentLevel, 
                                       currentLevel, currentLevel->currentSegment);
                }
            }
            
            // Left arrow - previous segment/level
            if (IsKeyPressed(KEY_LEFT)) {
                if (currentLevel->currentSegment > 0) {
                    // Previous segment in current level
                    HandleMusicTransition(&state, currentLevel, 
                                       currentLevel, currentLevel->currentSegment - 1);
                } else if (state.currentPlaying > 0) {
                    // Last segment of previous level
                    Level *prevLevel = &state.levels[state.currentPlaying - 1];
                    state.currentPlaying--;
                    HandleMusicTransition(&state, currentLevel, 
                                       prevLevel, prevLevel->segmentCount - 1);
                }
            }

            // Add to keyboard input section where other key checks are
            if (IsKeyPressed(KEY_R)) {
                state.repeatSegment = !state.repeatSegment;
                state.repeatBtn.color = state.repeatSegment ? RED : GRAY;
            }
        }

        if (state.currentPlaying != -1) {
            Segment *currentSeg = &state.levels[state.currentPlaying].segments[state.levels[state.currentPlaying].currentSegment];
            UpdateMusicStream(currentSeg->free);
            if (currentSeg->hasCombat) {
                UpdateMusicStream(currentSeg->combat);
            }
        }
        // Update button states
        state.pauseBtn.isHovered = IsButtonHovered(state.pauseBtn, mousePoint);
        state.combatBtn.isHovered = IsButtonHovered(state.combatBtn, mousePoint);
        state.segmentBtn.isHovered = IsButtonHovered(state.segmentBtn, mousePoint);
        state.repeatBtn.isHovered = IsButtonHovered(state.repeatBtn, mousePoint);

        // Handle pause button
        if (IsButtonClicked(state.pauseBtn, mousePoint)) {
            state.isPaused = !state.isPaused;
            HandleMusicPause(&state);
        }

        // Handle combat button
        if (IsButtonClicked(state.combatBtn, mousePoint) && state.currentPlaying != -1) {
            state.persistentCombat = !state.persistentCombat;
            Segment *currentSeg = &state.levels[state.currentPlaying].segments[state.levels[state.currentPlaying].currentSegment];
            if (currentSeg->hasCombat) {
                SetMusicVolume(currentSeg->free, state.persistentCombat ? 0.0f : 1.0f);
                SetMusicVolume(currentSeg->combat, state.persistentCombat ? 1.0f : 0.0f);
            }
        }

        // Handle segment button
        if (IsButtonClicked(state.segmentBtn, mousePoint) && state.currentPlaying != -1) {
            state.showSegmentMenu = !state.showSegmentMenu;
        }

        // Handle repeat button
        if (IsButtonClicked(state.repeatBtn, mousePoint)) {
            state.repeatSegment = !state.repeatSegment;
            state.repeatBtn.color = state.repeatSegment ? RED : GRAY;
        }

        // Handle scrolling
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0) {
            state.scrollY += wheelMove * SCROLL_SPEED;
            int totalRows = (state.levelCount + state.buttonsPerRow - 1) / state.buttonsPerRow;
            float minScroll = -(totalRows * (buttonHeight + padding) - (screenHeight - CONTROL_PANEL_HEIGHT));
            if (minScroll > 0) minScroll = 0;
            
            state.scrollY = Clamp(state.scrollY, minScroll, 0.0f);
        }

        // After updating button states and before drawing
        // Update pause button color to reflect state
        state.pauseBtn.color = state.isPaused ? RED : GRAY;
        state.pauseBtn.hoverColor = state.isPaused ? MAROON : LIGHTGRAY;

        BeginDrawing();
        ClearBackground(DARKERRED);

        // Draw level buttons
        for (int i = 0; i < state.levelCount; i++) {
            int row = i / state.buttonsPerRow;
            int col = i % state.buttonsPerRow;
            float x = state.startX + col * (buttonWidth + padding);
            float y = padding + row * (buttonHeight + padding) + state.scrollY;

            // Create image button
            Button levelBtn = CreateImageButton(
                (Rectangle){x, y, buttonWidth, buttonHeight},
                state.levels[i].thumbnail,
                fminf((buttonWidth - 20) / (float)state.levels[i].thumbnail.width, 
                     (buttonHeight - 40) / (float)state.levels[i].thumbnail.height),
                state.levels[i].name,
                (i == state.currentPlaying) ? RED : DARKRED,
                MIDRED, LIGHTGRAY,
                GetFontDefault(),
                10, 1.0f, true,
                (Rectangle){x + padding/2, y + buttonHeight - 30, buttonWidth - padding, 30}
            );

            levelBtn.isHovered = IsButtonHovered(levelBtn, mousePoint);
            if (IsButtonClicked(levelBtn, mousePoint) && 
                !state.showSegmentMenu &&
                mousePoint.y < (screenHeight - CONTROL_PANEL_HEIGHT)) {
                if (state.currentPlaying != -1) {
                    Segment *oldSeg = &state.levels[state.currentPlaying].segments[state.levels[state.currentPlaying].currentSegment];
                    StopMusicStream(oldSeg->free);
                    if (oldSeg->hasCombat) StopMusicStream(oldSeg->combat);
                }

                state.currentPlaying = i;
                state.levels[i].currentSegment = 0;
                Segment *newSeg = &state.levels[i].segments[0];
                PlayMusicStream(newSeg->free);
                if (newSeg->hasCombat) {
                    PlayMusicStream(newSeg->combat);
                    SetMusicVolume(newSeg->free, state.persistentCombat ? 0.0f : 1.0f);
                    SetMusicVolume(newSeg->combat, state.persistentCombat ? 1.0f : 0.0f);
                }
                state.isPaused = false;
                state.showSegmentMenu = false;
            }
            DrawButton(&levelBtn);
        }

        // Draw control panel
        DrawRectangle(0, screenHeight - CONTROL_PANEL_HEIGHT, screenWidth, CONTROL_PANEL_HEIGHT, MIDRED);
        DrawButton(&state.pauseBtn);
        DrawButton(&state.combatBtn);
        DrawButton(&state.segmentBtn);
        DrawButton(&state.repeatBtn);

        // Update combat button color
        if (state.currentPlaying != -1) {
            bool hasCombat = state.levels[state.currentPlaying].segments[state.levels[state.currentPlaying].currentSegment].hasCombat;
            state.combatBtn.color = hasCombat ? (state.persistentCombat ? RED : GRAY) : DARKGRAY;
        }
        
        if (state.currentPlaying != -1) {
            Level *currentLevel = &state.levels[state.currentPlaying];
            
            // Update segment button color
            if (currentLevel->segmentCount <= 1) {
                state.segmentBtn.color = DARKGRAY;
                state.segmentBtn.hoverColor = DARKGRAY;
            } else {
                state.segmentBtn.color = GRAY;
                state.segmentBtn.hoverColor = LIGHTGRAY;
            }
        }

        // Draw progress bar
        if (state.currentPlaying != -1) {
            Segment *currentSeg = &state.levels[state.currentPlaying].segments[state.levels[state.currentPlaying].currentSegment];
            float musicTime = GetMusicTimePlayed(currentSeg->free);
            float musicLength = GetMusicTimeLength(currentSeg->free);

            // Draw outline
            DrawRectangleRec(state.progressBar, DARKERRED);
            
            // Draw progress fill
            Rectangle progressFill = state.progressBar;
            progressFill.width = state.progressBar.width * (musicLength > 0 ? musicTime/musicLength : 0);
            DrawRectangleRec(progressFill, RAYWHITE);

            // Format time text
            char timeText[32];
            int currentMin = (int)musicTime / 60;
            int currentSec = (int)musicTime % 60;
            int totalMin = (int)musicLength / 60;
            int totalSec = (int)musicLength % 60;
            snprintf(timeText, sizeof(timeText), "%02d:%02d / %02d:%02d", 
                    currentMin, currentSec, totalMin, totalSec);

            // Draw time text
            Vector2 textSize = MeasureTextEx(GetFontDefault(), timeText, 20, 1);
            DrawTextEx(GetFontDefault(), timeText,
                      (Vector2){state.progressBar.x + state.progressBar.width/2 - textSize.x/2,
                                state.progressBar.y - 20},
                      20, 1, RAYWHITE);
        }

        // Draw segment menu
        if (state.showSegmentMenu) {
            HandleSegmentMenu(&state);
        }

        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}