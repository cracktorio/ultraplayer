#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "raylib.h"
#include "./cjson/cJSON.h"

// Common defines
#define CONTROL_PANEL_HEIGHT 50
#define SCROLL_SPEED 30.0f

#define FONT_SIZE_SMALL 11
#define FONT_SIZE_MEDIUM 12
#define FONT_SIZE_LARGE 14
#define FONT_SIZE_XLARGE 16

#define DARKRED (Color){ 84, 8, 8, 255 }
#define DARKERRED (Color){ 54, 8, 8, 255 }
#define MIDRED (Color){ 130, 8, 8, 255 }

#define MAX_LEVELS 50
#define MAX_SEGMENTS 10

typedef struct Segment {
    char name[256];
    Music free;
    Music combat;
    bool hasCombat;
} Segment;

typedef struct Level {
    char name[256];
    char thumbnailPath[512];
    Texture2D thumbnail;
    Segment segments[MAX_SEGMENTS];
    int segmentCount;
    int currentSegment;
} Level;

typedef struct Button {
    Rectangle bounds;
    const char *text;
    Texture2D texture;
    Color color;
    Color hoverColor;
    Color borderColor;
    bool isHovered;
    bool wordWrap;
    Font font;
    float fontSize;
    float spacing;
    Rectangle textRec;
    float textureScale;
} Button;

typedef struct AppState {
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

char *LoadFileTextCustom(const char *fileName);
bool ParseJSONData(const char *jsonFileName, Level levels[], int *levelCount);
void DrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);

// Button functions
Button CreateTextButton(Rectangle bounds, const char *text, Color color, Color hoverColor, Color borderColor, Font font, float fontSize, float spacing, bool wordWrap);
Button CreateImageButton(Rectangle bounds, Texture2D texture, float textureScale, const char *text, Color color, Color hoverColor, Color borderColor, Font font, float fontSize, float spacing, bool wordWrap, Rectangle textRec);
void DrawButton(Button *button);
bool IsButtonHovered(Button button, Vector2 mousePoint);
bool IsButtonClicked(Button button, Vector2 mousePoint);

void InitializeButtons(AppState *state, int screenWidth, int screenHeight);
void HandleSegmentMenu(AppState *state);

#endif