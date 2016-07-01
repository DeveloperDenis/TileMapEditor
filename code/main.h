#ifndef MAIN_H_
#define MAIN_H_

#include "denis_math.h"
#include "SDL_rect.h"


//TODO(denis): probably a temporary header file that will be replaced
struct Tile
{
    SDL_Rect pos;
    SDL_Rect sheetPos;
};

struct TileMap
{
    Tile *tiles;
    int tileSize;
    int widthInTiles;
    int heightInTiles;

    //TODO(denis): should this be in here?
    Vector2 offset;

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

#endif
