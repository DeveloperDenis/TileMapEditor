#ifndef IMPORT_TILE_SET_PANEL_H_
#define IMPORT_TILE_SET_PANEL_H_

#include "denis_meta.h"

void importTileSetPanelCreateNew(SDL_Renderer *renderer, int32 x, int32 y,
				 int32 width, int32 height);

void importTileSetPanelOnMouseDown(Vector2 mousePos, uint8 mouseButton);
void importTileSetPanelOnMouseUp(Vector2 mousePos, uint8 mouseButton);

void importTileSetPanelCharInput(char c);
void importTileSetPanelCharDeleted();

void importTileSetPanelDraw();

bool importTileSetPanelVisible();
void importTileSetPanelSetVisible(bool newValue);

void importTileSetPanelSetTileSize(int32 newSize);

#endif
