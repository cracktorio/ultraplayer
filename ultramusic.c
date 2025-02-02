#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "./cjson/cJSON.h"
#include "functions.h"
#include "functions.c"

#define CONTROL_PANEL_HEIGHT 50
#define SCROLL_SPEED 30.0f

#define FONT_SIZE_SMALL 11
#define FONT_SIZE_MEDIUM 12
#define FONT_SIZE_LARGE 14
#define FONT_SIZE_XLARGE 16

#define DARKRED (Color){ 84, 8, 8, 255 }
#define DARKERRED (Color){ 54, 8, 8, 255 }
#define MIDRED (Color){ 130, 8, 8, 255 }

typedef struct {
    Button pauseBtn;
    Button combatBtn;
    Button segmentBtn;
    Level levels[MAX_LEVELS];
    int levelCount;
    int currentPlaying;
    bool isPaused;
    bool persistentCombat;
    bool showSegmentMenu;
    float scrollY;
    int buttonsPerRow;
    int startX;
    Rectangle progressBar;
} AppState;

void InitializeButtons(AppState *state, int screenWidth, int screenHeight) {
    // Control buttons
    state->pauseBtn = CreateTextButton(
        (Rectangle){20, screenHeight - CONTROL_PANEL_HEIGHT + FONT_SIZE_SMALL, 100, 30},
        "Pause", GRAY, LIGHTGRAY, BLACK, GetFontDefault(), FONT_SIZE_SMALL, 1.0f, false
    );
    
    state->combatBtn = CreateTextButton(
        (Rectangle){140, screenHeight - CONTROL_PANEL_HEIGHT + FONT_SIZE_SMALL, 100, 30},
        "Combat", GRAY, LIGHTGRAY, BLACK, GetFontDefault(), FONT_SIZE_SMALL, 1.0f, false
    );
    
    state->segmentBtn = CreateTextButton(
        (Rectangle){screenWidth - 120, screenHeight - CONTROL_PANEL_HEIGHT + FONT_SIZE_SMALL, 100, 30},
        "Segments", GRAY, LIGHTGRAY, BLACK, GetFontDefault(), FONT_SIZE_SMALL, 1.0f, false
    );

    // Progress bar
    state->progressBar = (Rectangle){250, screenHeight - CONTROL_PANEL_HEIGHT + 20, 400, FONT_SIZE_SMALL};
}

void HandleSegmentMenu(AppState *state) {
    if (!state->showSegmentMenu || state->currentPlaying == -1) return;

    Vector2 mousePoint = GetMousePosition();
    Level *currentLevel = &state->levels[state->currentPlaying];
    
    // Calculate menu dimensions
    float maxWidth = state->segmentBtn.bounds.width;
    for (int i = 0; i < currentLevel->segmentCount; i++) {
        float nameWidth = MeasureText(currentLevel->segments[i].name, 10);
        maxWidth = fmaxf(maxWidth, nameWidth + 20);
    }

    // Menu background
    Rectangle menuBg = {
        state->segmentBtn.bounds.x + state->segmentBtn.bounds.width - maxWidth - 10,
        state->segmentBtn.bounds.y - (currentLevel->segmentCount * 40),
        maxWidth + 10,
        currentLevel->segmentCount * 40 + 5
    };

    // Check click outside
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && 
        !CheckCollisionPointRec(mousePoint, menuBg) &&
        !CheckCollisionPointRec(mousePoint, state->segmentBtn.bounds)) {
        state->showSegmentMenu = false;
    }

    // Draw segments
    for (int i = 0; i < currentLevel->segmentCount; i++) {
        Rectangle segmentRec = {
            menuBg.x + 5,
            state->segmentBtn.bounds.y - ((i + 1) * 40) + 5,
            maxWidth,
            30
        };

        bool isHovered = CheckCollisionPointRec(mousePoint, segmentRec);
        bool isSelected = (i == currentLevel->currentSegment);

        // Draw segment background
        DrawRectangleRec(segmentRec, isSelected ? WHITE : (isHovered ? LIGHTGRAY : GRAY));
        if (isSelected) DrawRectangleLinesEx(segmentRec, 1, GRAY);

        // Draw text
        DrawText(currentLevel->segments[i].name,
                segmentRec.x + 10,
                segmentRec.y + (segmentRec.height - 10) / 2,
                10,
                isSelected ? BLACK : WHITE);

        // Handle click
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Segment *oldSeg = &currentLevel->segments[currentLevel->currentSegment];
            StopMusicStream(oldSeg->free);
            if (oldSeg->hasCombat) StopMusicStream(oldSeg->combat);

            currentLevel->currentSegment = i;
            Segment *newSeg = &currentLevel->segments[i];
            PlayMusicStream(newSeg->free);
            if (newSeg->hasCombat) {
                PlayMusicStream(newSeg->combat);
                SetMusicVolume(newSeg->free, state->persistentCombat ? 0.0f : 1.0f);
                SetMusicVolume(newSeg->combat, state->persistentCombat ? 1.0f : 0.0f);
            }
            state->showSegmentMenu = false;
        }
    }
}

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

        // Handle pause button
        if (IsButtonClicked(state.pauseBtn, mousePoint)) {
            state.isPaused = !state.isPaused;
            if (state.currentPlaying != -1) {
                Segment *currentSeg = &state.levels[state.currentPlaying].segments[state.levels[state.currentPlaying].currentSegment];
                if (state.isPaused) {
                    PauseMusicStream(currentSeg->free);
                    if (currentSeg->hasCombat) PauseMusicStream(currentSeg->combat);
                } else {
                    ResumeMusicStream(currentSeg->free);
                    if (currentSeg->hasCombat) ResumeMusicStream(currentSeg->combat);
                }
            }
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

        // Handle scrolling
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0) {
            state.scrollY += wheelMove * SCROLL_SPEED;
            int totalRows = (state.levelCount + state.buttonsPerRow - 1) / state.buttonsPerRow;
            float minScroll = -(totalRows * (buttonHeight + padding) - (screenHeight - CONTROL_PANEL_HEIGHT));
            if (minScroll > 0) minScroll = 0;
            
            state.scrollY = Clamp(state.scrollY, minScroll, 0.0f);
        }

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