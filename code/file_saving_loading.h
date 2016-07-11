#ifndef FILE_SAVING_LOADING_H_
#define FILE_SAVING_LOADING_H_

struct TileMap;
void saveTileMapToFile(TileMap *tileMap, char *tileMapName);

//TODO(denis): return some sort of tile sheet struct?
void loadTileSheet();

#endif
