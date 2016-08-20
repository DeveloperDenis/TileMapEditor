/*
 * Written by Denis Levesque
 */

#include "tile_map_file.h"
#include "windows.h"
#include "assert.h"

#define HEAP_ALLOC(bytes) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
#define HEAP_FREE(ptr) HeapFree(GetProcessHeap(), 0, ptr);

static char* duplicateString(char *string)
{
    char *result = 0;
    
    if (string)
    {
	uint32 numChars = 0;
	for (uint32 i = 0; string[i] != 0; ++i)
	{
	    ++numChars;
	}

	result = (char*)HEAP_ALLOC(numChars+1);

	for (uint32 i = 0; i < numChars; ++i)
	{
	    result[i] = string[i];
	}
    }

    return result;
}

LoadTileMapResult loadTileMap(char *fileName)
{
    char *tileMapName = 0;
    uint32 tileMapWidth = 0;
    uint32 tileMapHeight = 0;
    uint32 tileSize = 0;
    char *tileSheetFileName = 0;
    LoadedTile *tiles = 0;
    
    HANDLE fileHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
				   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
	LARGE_INTEGER bytesToRead = {};
	DWORD bytesRead = 0;
	GetFileSizeEx(fileHandle, &bytesToRead);

        void *buffer = HEAP_ALLOC(bytesToRead.QuadPart);

	//NOTE(denis): could be a problem if we ever read files that are
	// gigabytes in size (shouldn't happen though)
	assert(bytesToRead.HighPart == 0);
	if (ReadFile(fileHandle, buffer, bytesToRead.LowPart, &bytesRead, NULL))
	{
	    assert(bytesToRead.LowPart == bytesRead);

	    MapFileHeader *fileHeader = (MapFileHeader*)buffer;
	    tileMapWidth = fileHeader->tileMapWidth;
	    tileMapHeight = fileHeader->tileMapHeight;
	    tileSize = fileHeader->tileSize;
	    tileSheetFileName = duplicateString(fileHeader->tileSheetFileName);
	    tileMapName = duplicateString(fileHeader->tileMapName);
	    
	    uint32 mapSizeInBytes = tileMapWidth*tileMapHeight*sizeof(LoadedTile);
	    tiles = (LoadedTile*)HEAP_ALLOC(mapSizeInBytes);

	    uint8 *bufferTiles = (uint8*)buffer + sizeof(MapFileHeader);
	    for (uint32 i = 0; i < mapSizeInBytes; ++i)
	    {
		*((uint8*)tiles + i) = *(bufferTiles + i);
	    }
	}
	
	HEAP_FREE(buffer);
	CloseHandle(fileHandle);
    }

    LoadTileMapResult result = {};
    result.tileMapName = tileMapName;
    result.tileMapWidth = tileMapWidth;
    result.tileMapHeight = tileMapHeight;
    result.tileSize = tileSize;
    result.tileSheetFileName = tileSheetFileName;
    result.tiles = tiles;

    return result;
}
