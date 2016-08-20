#ifndef FILE_SAVING_LOADING_H_
#define FILE_SAVING_LOADING_H_

#include "tile_map_file.h"

struct TileMap;

void saveTileMapToFile(TileMap *tileMap, char *tileMapName);
LoadTileMapResult loadTileMapFromFile();

char* getTileSheetFileName();

#endif
