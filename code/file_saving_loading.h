#ifndef FILE_SAVING_LOADING_H_
#define FILE_SAVING_LOADING_H_

struct TileMap;
void saveTileMapToFile(TileMap *tileMap, char *tileMapName);
char* getTileSheetFileName();

#endif
