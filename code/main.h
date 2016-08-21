#ifndef MAIN_H_
#define MAIN_H_

#define DENIS_INTERNAL

#include "SDL_rect.h"
#include "SDL_render.h"
#undef max
#include "denis_meta.h"
#include "denis_math.h"

struct SDL_Surface;

//TODO(denis): probably a temporary header file that will be replaced

struct Tile
{
    uint32 size;
    
    Point2 pos;
    Point2 sheetPos;
};

struct TileSet
{
    char *name;

    SDL_Rect imageSize;

    SDL_Surface *surface;
    SDL_Texture *image;
    uint32 tileSize;

    Tile *tiles;
    uint32 numTiles;

    Tile selectedTile;
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

static inline void drawTile(SDL_Renderer *renderer, SDL_Texture *tileSheet,
			    Point2 sheetPos, Point2 screenPos, uint32 size)
{
    SDL_Rect sheetRect = {sheetPos.x, sheetPos.y, size, size};
    SDL_Rect screenRect = {screenPos.x, screenPos.y, size, size};

    SDL_RenderCopy(renderer, tileSheet, &sheetRect, &screenRect);
}

#endif
