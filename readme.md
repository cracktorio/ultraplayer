
# UltraPlayer

This is a music player that aims to mimic the way games play music.

It functions by reading data from `data.json`, and loading the appropriate files from the assets folder.
You can find some examples and game OSTs [here](https://github.com/cracktorio/ultraplayer-music).

Main Interface:

![Main](https://raw.githubusercontent.com/cracktorio/ultraplayer/refs/heads/main/screenshots/main.png)

Support for segments:

![Segments](https://raw.githubusercontent.com/cracktorio/ultraplayer/refs/heads/main/screenshots/segments.png)

## Features

- Seamless loop (provided you don't use mp3)
- Levels with name and thumbnail
- Each level has segments
- Each segment has a base loop and an optional combat loop
- Repeat song or play in chronological order
- Keyboard controls
- Built in Raylib

### Controls
pause: `space`  
switch combat/peaceful: `c`  
repeat: `r`  
next: `arrow right`  
previous: `arrow left`  

## Building
**There is no need to recompile if you just want to change the `data.json`!**

this project is built in raylib with cJSON, and written in C.
To compile it, you will only need raylib as the cJSON library comes with the program.
On windows, it should be built with SDL.
## Known Issues
- Playback pauses when moving the window
