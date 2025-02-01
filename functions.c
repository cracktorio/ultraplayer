#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./cjson/cJSON.c"

char *LoadFileTextCustom(const char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    
    char *text = (char *)malloc(size + 1);
    if (text)
    {
        fread(text, 1, size, file);
        text[size] = '\0';
    }
    fclose(file);
    return text;
}

bool ParseJSONData(const char *jsonFileName, Level levels[], int *levelCount)
{
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
        
        // Use the JSON key as the level name.
        const char *levelKey = levelEntry->string;
        if (!levelKey) continue;
        strncpy(levels[*levelCount].name, levelKey, sizeof(levels[*levelCount].name) - 1);
        levels[*levelCount].name[sizeof(levels[*levelCount].name) - 1] = '\0';
        
        // Get required properties: folder, thumbnail and segments from the level entry.
        cJSON *folderItem = cJSON_GetObjectItemCaseSensitive(levelEntry, "folder");
        cJSON *thumbItem  = cJSON_GetObjectItemCaseSensitive(levelEntry, "thumbnail");
        cJSON *segments   = cJSON_GetObjectItemCaseSensitive(levelEntry, "segments");
        if (!cJSON_IsString(folderItem) || !cJSON_IsString(thumbItem) || !segments)
        {
            printf("Warning: Skipping level '%s' because of missing fields.\n", levelKey);
            continue;
        }
        
        // Get segment "0" for music information.
        cJSON *segment0 = cJSON_GetObjectItemCaseSensitive(segments, "0");
        if (!segment0)
        {
            printf("Warning: Skipping level '%s' because segment 0 is missing.\n", levelKey);
            continue;
        }
        cJSON *freeMusicItem = cJSON_GetObjectItemCaseSensitive(segment0, "free");
        cJSON *combatMusicItem = cJSON_GetObjectItemCaseSensitive(segment0, "combat");
        if (!cJSON_IsString(freeMusicItem))
        {
            printf("Warning: Skipping level '%s' because free music is missing.\n", levelKey);
            continue;
        }
        
        // Build full paths for the thumbnail and free music
        snprintf(levels[*levelCount].thumbnailPath, sizeof(levels[*levelCount].thumbnailPath),
                 "%s/%s/%s", baseFolder, folderItem->valuestring, thumbItem->valuestring);
        snprintf(levels[*levelCount].musicPath, sizeof(levels[*levelCount].musicPath),
                 "%s/%s/%s", baseFolder, folderItem->valuestring, freeMusicItem->valuestring);
        
        // Load thumbnail image and convert to texture.
        Image img = LoadImage(levels[*levelCount].thumbnailPath);
        if (img.data != NULL)
        {
            levels[*levelCount].thumbnail = LoadTextureFromImage(img);
            UnloadImage(img);
        }
        else
        {
            printf("Warning: Could not load image: %s\n", levels[*levelCount].thumbnailPath);
            levels[*levelCount].thumbnail = (Texture2D){ 0 };
        }
        
        // Load the free music stream.
        levels[*levelCount].music = LoadMusicStream(levels[*levelCount].musicPath);
        
        // Load the combat music stream if provided.
        if (cJSON_IsString(combatMusicItem))
        {
            char combatPath[512];
            snprintf(combatPath, sizeof(combatPath), "%s/%s/%s",
                     baseFolder, folderItem->valuestring, combatMusicItem->valuestring);
            levels[*levelCount].combatMusic = LoadMusicStream(combatPath);
        }
        else
        {
            levels[*levelCount].combatMusic = (Music){ 0 };
        }
        
        (*levelCount)++;
    }
    
    cJSON_Delete(jsonRoot);
    return true;
}

/* DrawTextRec: Draws text inside a rectangle with word-wrapping support.
   This is a simple implementation that splits the text into words and
   draws as many words per line as will fit in the given rectangle.
   Parameters:
     - font: the Font to use.
     - text: the text to draw.
     - rec: the rectangle inside which to draw the text.
     - fontSize: desired font size.
     - spacing: extra spacing between lines.
     - wordWrap: if true, word wrap the text; otherwise, draw as a single line.
     - tint: the color tint for the text.
*/
void DrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint)
{
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
