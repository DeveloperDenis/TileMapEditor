# Tile-Map-Editor

A simple, fast, and small Windows tile map editor app for games.
Programmed in C/C++ using the SDL2 library.

### Current Features

- create arbitrarily large tile maps that can be scrolled if they don't fit on the screen
  - draw tiles onto the tile map using paint or fill tools
  - move around bigger maps by using the scroll bars or the move tool
  - access each tool quickly by pressing the shortcut keys (p, f, or m respectively) or hold down the space bar to temporarily use the move tool
  
- import image files as tile sheets
  - tile sheets are automatically cropped and any empty tiles are removed from drawing
  - open as many as 15 different tile sets at a time and switch between them easily using a dropdown menu
  
- save tile maps in a simple custom file format to use directly in your game, or to load back into the program later for further editing  
### Known Issues/Planned Additions

- if you switch tile sets while editing a tile map, every tile in the tile map is drawn using whichever tile set is currently active, no matter which tile set you used to create it
- if you load a tile map without any tile sets open, nothing will draw until you load a tile set, and it will only draw the way it was created if you have loaded the exact tile set used to create it
