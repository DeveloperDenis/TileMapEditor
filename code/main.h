#ifndef MAIN_H_
#define MAIN_H_

#include "SDL_rect.h"
#undef max
#include "denis_meta.h"
#include "denis_math.h"

struct SDL_Texture;

//TODO(denis): probably a temporary header file that will be replaced

struct Tile
{
    SDL_Rect pos;
    SDL_Rect sheetPos;
};

struct TileMap
{
    Tile *tiles;
    char *name;
    int tileSize;
    int widthInTiles;
    int heightInTiles;
    
    Vector2 offset;
    Vector2 drawOffset;
    
    SDL_Rect visibleArea;
    
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

struct TileSet
{
    char *name;

    //TODO(denis): should this just be a width + height property?
    SDL_Rect imageSize;
    
    SDL_Texture *image;
    uint32 tileSize;

    SDL_Rect collisionBox;
};

//TODO(denis): not sure where to put this
struct TexturedRect
{
    SDL_Texture* image;
    SDL_Rect pos;

    int getWidth() { return this->pos.w; };
    int getHeight() { return this->pos.h; };
    void setPosition(Vector2 newPos)
    {
	this->pos.x = newPos.x;
	this->pos.y = newPos.y;
    }
};

#endif
