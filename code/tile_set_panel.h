#ifndef TILE_SET_PANEL_H_
#define TILE_SET_PANEL_H_

#include "denis_meta.h"

void tileSetPanelCreateNew(SDL_Renderer *renderer, uint32 x, uint32 y,
			   uint32 width, uint32 height);

void tileSetPanelDraw();

void tileSetPanelOnMouseMove(Vector2 mousePos);
void tileSetPanelOnMouseDown(Vector2 mousePos, uint8 mouseButton);
void tileSetPanelOnMouseUp(Vector2 mousePos, uint8 mouseButton);

void tileSetPanelInitializeNewTileSet(char *name, SDL_Surface *image, uint32 tileSize);

Tile tileSetPanelGetSelectedTile();
SDL_Texture *tileSetPanelGetCurrentTileSet();
bool tileSetPanelImportTileSetPressed();
int tileSetPanelGetCurrentTileSize();
char* tileSetPanelGetCurrentTileSetFileName();

bool tileSetPanelVisible();

Vector2 tileSetPanelGetPosition();
void tileSetPanelSetPosition(Vector2 newPos);

#endif
