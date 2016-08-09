#ifndef TILE_MAP_PANEL_H_
#define TILE_MAP_PANEL_H_

#include "denis_meta.h"


void tileMapPanelCreateNew(SDL_Renderer *renderer, uint32 x, uint32 y,
		      uint32 width, uint32 height);

void tileMapPanelDraw();

void tileMapPanelOnMouseMove(Vector2 mousePos, int32 leftClickFlag);
void tileMapPanelOnMouseDown(Vector2 mousePos, uint8 mouseButton);
void tileMapPanelOnMouseUp(Vector2 mousePos, uint8 mouseButton);

void tileMapPanelOnKeyPressed(SDL_Keycode key);
void tileMapPanelOnKeyReleased(SDL_Keycode key);

TileMap* tileMapPanelAddNewTileMap();

bool tileMapPanelVisible();
void tileMapPanelSetVisible(bool newValue);

bool tileMapPanelTileMapIsValid();
TileMap* tileMapPanelGetCurrentTileMap();
void tileMapPanelSelectTileMap(uint32 newSelection);

#endif
