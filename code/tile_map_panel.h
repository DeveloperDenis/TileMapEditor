#ifndef TILE_MAP_PANEL_H_
#define TILE_MAP_PANEL_H_

#include "denis_meta.h"
#include "SDL_keycode.h"

struct TileMapTile
{
    union
    {
	Tile tile;
	
	struct
	{
	    SDL_Rect pos;
	    SDL_Rect sheetPos;
	};
    };

    bool initialized;
};

struct TileMap
{
    TileMapTile *tiles;
    char *name;
    int tileSize;
    int widthInTiles;
    int heightInTiles;
    
    Vector2 offset;
    Vector2 drawOffset;
    
    SDL_Rect visibleArea;

    ScrollBar horizontalBar;
    ScrollBar verticalBar;
    
    //TODO(denis): create a setPosition function that will reposition all of the
    // tiles contained within as well
    
    SDL_Rect getRect()
    {
	SDL_Rect result = {};
	
	result.x = offset.x;
	result.y = offset.y;
	result.w = tileSize*widthInTiles;
	result.h = tileSize*heightInTiles;
	
	return result;
    }
};

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
