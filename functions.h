#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "raylib.h"
#include "./cjson/cJSON.h"  // Adjust path as needed

#define MAX_LEVELS 20  // maximum number of levels supported

//--------------------------------------------------------------------
// Level structure
//--------------------------------------------------------------------
typedef struct Level {
    char name[256];          // Level name (taken from the JSON key)
    char thumbnailPath[512]; // Full path to the thumbnail image
    char musicPath[512];     // Full path to the free music track
    Texture2D thumbnail;     // Loaded texture for thumbnail
    Music music;             // Loaded music stream for free segment
    Music combatMusic;       // Loaded music stream for combat segment
} Level;

//--------------------------------------------------------------------
// Function declarations
//--------------------------------------------------------------------
/* Load the entire contents of a file into a dynamically allocated string.
   Caller is responsible for freeing the returned string. */
char *LoadFileTextCustom(const char *fileName);

/* Parse the JSON data from a file and populate the levels array.
   Returns true if successful; false otherwise.
   levelCount is updated to the number of levels loaded. */
bool ParseJSONData(const char *jsonFileName, Level levels[], int *levelCount);

/* Draw text inside a rectangle with word-wrapping support.
   - font: the Font to use
   - text: the text to draw
   - rec: the rectangle inside which to draw the text
   - fontSize: the desired font size
   - spacing: additional spacing between lines
   - wordWrap: if true, auto-wrap words; if false, no wrapping
   - tint: the color tint of the text */
void DrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);

#endif // FUNCTIONS_H
