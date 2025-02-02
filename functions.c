#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./cjson/cJSON.c"

char *LoadFileTextCustom(const char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    
    char *text = (char *)malloc(size + 1);
    if (text) {
        fread(text, 1, size, file);
        text[size] = '\0';
    }
    fclose(file);
    return text;
}

bool ParseJSONData(const char *jsonFileName, Level levels[], int *levelCount) {
char *jsonText = LoadFileTextCustom(jsonFileName);
    if (jsonText == NULL)
    {
        printf("Error: Unable to open %s\n", jsonFileName);
        return false;
    }
    
    cJSON *jsonRoot = cJSON_Parse(jsonText);
    free(jsonText);
    if (!jsonRoot)
    {
        printf("Error: Could not parse JSON\n");
        return false;
    }
    
    // Get the base folder from JSON
    cJSON *baseFolderItem = cJSON_GetObjectItemCaseSensitive(jsonRoot, "base-folder");
    if (!cJSON_IsString(baseFolderItem))
    {
        printf("Error: JSON missing base-folder string\n");
        cJSON_Delete(jsonRoot);
        return false;
    }
    const char *baseFolder = baseFolderItem->valuestring;
    
    // Get the levels object (an object with level keys)
    cJSON *levelsObj = cJSON_GetObjectItemCaseSensitive(jsonRoot, "levels");
    if (!levelsObj)
    {
        printf("Error: JSON missing levels object\n");
        cJSON_Delete(jsonRoot);
        return false;
    }
    
    *levelCount = 0;
    cJSON *levelEntry = NULL;
    cJSON_ArrayForEach(levelEntry, levelsObj)
    {
        if (*levelCount >= MAX_LEVELS) break;
        
        // Level name
        const char *levelKey = levelEntry->string;
        if (!levelKey) continue;
        strncpy(levels[*levelCount].name, levelKey, sizeof(levels[*levelCount].name) - 1);
        
        // Get required properties
        cJSON *folderItem = cJSON_GetObjectItemCaseSensitive(levelEntry, "folder");
        cJSON *thumbItem = cJSON_GetObjectItemCaseSensitive(levelEntry, "thumbnail");
        cJSON *segments = cJSON_GetObjectItemCaseSensitive(levelEntry, "segments");
        
        if (!cJSON_IsString(folderItem) || !cJSON_IsString(thumbItem) || !segments)
        {
            printf("Warning: Skipping level '%s' because of missing fields.\n", levelKey);
            continue;
        }

        // Load thumbnail
        snprintf(levels[*levelCount].thumbnailPath, sizeof(levels[*levelCount].thumbnailPath),
                 "%s/%s/%s", baseFolder, folderItem->valuestring, thumbItem->valuestring);
        Image img = LoadImage(levels[*levelCount].thumbnailPath);
        if (img.data != NULL)
        {
            levels[*levelCount].thumbnail = LoadTextureFromImage(img);
            UnloadImage(img);
        }
        
        // Initialize segments
        levels[*levelCount].segmentCount = 0;
        levels[*levelCount].currentSegment = 0;
        
        // Parse segments
        cJSON *segmentEntry = NULL;
        cJSON_ArrayForEach(segmentEntry, segments) {
            if (levels[*levelCount].segmentCount >= MAX_SEGMENTS) break;
            
            cJSON *nameItem = cJSON_GetObjectItemCaseSensitive(segmentEntry, "name");
            cJSON *freeMusicItem = cJSON_GetObjectItemCaseSensitive(segmentEntry, "free");
            cJSON *combatMusicItem = cJSON_GetObjectItemCaseSensitive(segmentEntry, "combat");
            
            if (!nameItem || !freeMusicItem) continue;
            
            int segIdx = levels[*levelCount].segmentCount;
            Segment *seg = &levels[*levelCount].segments[segIdx];
            
            strncpy(seg->name, nameItem->valuestring, sizeof(seg->name) - 1);
            
            char musicPath[512];
            snprintf(musicPath, sizeof(musicPath), "%s/%s/%s",
                     baseFolder, folderItem->valuestring, freeMusicItem->valuestring);
            seg->free = LoadMusicStream(musicPath);
            if (seg->free.ctxData == NULL) {
                printf("Failed to load music: %s\n", musicPath);
            }
            if (cJSON_IsString(combatMusicItem)) {
                snprintf(musicPath, sizeof(musicPath), "%s/%s/%s",
                        baseFolder, folderItem->valuestring, combatMusicItem->valuestring);
                seg->combat = LoadMusicStream(musicPath);
                seg->hasCombat = true;
                
                if (seg->combat.ctxData == NULL) {
                    printf("Failed to load combat music: %s\n", musicPath);
                }
            }
            
            levels[*levelCount].segmentCount++;
        }
        
        (*levelCount)++;
    }
    
    cJSON_Delete(jsonRoot);
    return *levelCount > 0;
    }

void DrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint) {
    // If wordWrap is false, simply draw the text and return.
    if (!wordWrap)
    {
        DrawTextEx(font, text, (Vector2){ rec.x, rec.y }, fontSize, spacing, tint);
        return;
    }
    
    float lineHeight = fontSize + spacing;
    float posY = rec.y;
    char lineBuffer[1024] = { 0 };
    lineBuffer[0] = '\0';
    
    const char *ptr = text;
    while (*ptr)
    {
        // Find the next space or newline
        const char *next = ptr;
        while (*next && *next != ' ' && *next != '\n') next++;
        int wordLen = next - ptr;
        char word[256] = { 0 };
        if (wordLen < sizeof(word))
        {
            strncpy(word, ptr, wordLen);
            word[wordLen] = '\0';
        }
        
        // Create a test line from the current lineBuffer plus this word
        char testLine[1024] = { 0 };
        if (lineBuffer[0] != '\0')
            snprintf(testLine, sizeof(testLine), "%s %s", lineBuffer, word);
        else
            snprintf(testLine, sizeof(testLine), "%s", word);
        
        // Measure the width of the test line
        Vector2 size = MeasureTextEx(font, testLine, fontSize, spacing);
        if (size.x > rec.width && lineBuffer[0] != '\0')
        {
            // The word does not fit in the current line.
            DrawTextEx(font, lineBuffer, (Vector2){ rec.x, posY }, fontSize, spacing, tint);
            posY += lineHeight;
            lineBuffer[0] = '\0';
            // Continue without appending a space.
            snprintf(lineBuffer, sizeof(lineBuffer), "%s", word);
        }
        else
        {
            // Append the word to the current line.
            if (lineBuffer[0] != '\0')
            {
                strcat(lineBuffer, " ");
                strcat(lineBuffer, word);
            }
            else
            {
                strcpy(lineBuffer, word);
            }
        }
        
        // If a newline character was encountered, force a break.
        if (*next == '\n')
        {
            DrawTextEx(font, lineBuffer, (Vector2){ rec.x, posY }, fontSize, spacing, tint);
            posY += lineHeight;
            lineBuffer[0] = '\0';
        }
        
        // Skip spaces and newlines.
        ptr = next;
        while (*ptr == ' ' || *ptr == '\n')
        {
            if (*ptr == '\n')
            {
                DrawTextEx(font, lineBuffer, (Vector2){ rec.x, posY }, fontSize, spacing, tint);
                posY += lineHeight;
                lineBuffer[0] = '\0';
            }
            ptr++;
        }
    }
    // Draw any remaining text.
    if (lineBuffer[0] != '\0')
    {
        DrawTextEx(font, lineBuffer, (Vector2){ rec.x, posY }, fontSize, spacing, tint);
    }
    }

// Button implementation
Button CreateTextButton(Rectangle bounds, const char *text, Color color, Color hoverColor, Color borderColor, Font font, float fontSize, float spacing, bool wordWrap) {
    return (Button){
        .bounds = bounds,
        .text = text,
        .color = color,
        .hoverColor = hoverColor,
        .borderColor = borderColor,
        .font = font,
        .fontSize = fontSize,
        .spacing = spacing,
        .wordWrap = wordWrap,
        .texture.id = 0
    };
}

Button CreateImageButton(Rectangle bounds, Texture2D texture, float textureScale, const char *text, Color color, Color hoverColor, Color borderColor, Font font, float fontSize, float spacing, bool wordWrap, Rectangle textRec) {
    return (Button){
        .bounds = bounds,
        .texture = texture,
        .textureScale = textureScale,
        .text = text,
        .color = color,
        .hoverColor = hoverColor,
        .borderColor = borderColor,
        .font = font,
        .fontSize = fontSize,
        .spacing = spacing,
        .wordWrap = wordWrap,
        .textRec = textRec
    };
}

void DrawButton(Button *button) {
    // Draw background
    DrawRectangleRec(button->bounds, button->isHovered ? button->hoverColor : button->color);
    
    // Draw border
    DrawRectangleLinesEx(button->bounds, 1, button->borderColor);
    
    // Draw texture
    if (button->texture.id != 0) {
        Vector2 pos = {
            button->bounds.x + (button->bounds.width - button->texture.width*button->textureScale)/2,
            button->bounds.y + 10
        };
        DrawTextureEx(button->texture, pos, 0.0f, button->textureScale, WHITE);
    }
    
    // Draw text
    if (button->text != NULL) {
        if (button->wordWrap) {
            DrawTextRec(button->font, button->text, button->textRec, button->fontSize, button->spacing, true, WHITE);
        }
        else {
            Vector2 textPos = {
                button->bounds.x + (button->bounds.width - MeasureText(button->text, button->fontSize))/2,
                button->bounds.y + (button->bounds.height - button->fontSize)/2
            };
            DrawTextEx(button->font, button->text, textPos, button->fontSize, button->spacing, WHITE);
        }
    }
}

bool IsButtonHovered(Button button, Vector2 mousePoint) {
    return CheckCollisionPointRec(mousePoint, button.bounds);
}

bool IsButtonClicked(Button button, Vector2 mousePoint) {
    return IsButtonHovered(button, mousePoint) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

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