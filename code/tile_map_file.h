#ifndef TILE_MAP_FILE_H_
#define TILE_MAP_FILE_H_

#if !defined(DENIS_INTERNAL)

#include "stdint.h"

typedef int32_t int32;
typedef uint32_t uint32;

struct Point2
{
    int32 x, y;
};

struct Tile
{
    uint32 size;
    
    Point2 pos;
    Point2 sheetPos;
};

#endif

struct MapFileHeader
{
    char tileMapName[256];
    
    uint32 tileMapWidth;
    uint32 tileMapHeight;
    uint32 tileSize;

    char tileSheetFileName[256];
};

struct LoadTileMapResult
{
    char *tileMapName;
    uint32 tileMapWidth;
    uint32 tileMapHeight;
    uint32 tileSize;
    
    char *tileSheetFileName;

    Tile *tiles;
};

//NOTE(denis): you want to call this function with the full path name
LoadTileMapResult loadTileMap(char *fileName);

#endif
