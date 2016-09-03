#include "ui_elements.h"
#include "tile_map_file.h"
#include "file_saving_loading.h"
#include "tile_map_panel.h"
#include "tile_set_panel.h"
#include "windows.h"
#include "assert.h"

void saveTileMapToFile(TileMap *tileMap, char *tileMapName)
{
    //TODO(denis): maybe make this bigger?
    const int fileNameBufferSize = 256;
    int charsToExtension = 0;

    char fileName[fileNameBufferSize];
    bool done = false;
    for (int i = 0; i < fileNameBufferSize && !done; ++i)
    {
	if (tileMapName[i] == '\0')
	{
	    charsToExtension = i+1;
	    done = true;
	    fileName[i] = '.';
	    fileName[i+1] = 'm';
	    fileName[i+2] = 'a';
	    fileName[i+3] = 'p';
	    fileName[i+4] = '\0';
	}
	else
	    fileName[i] = tileMapName[i];
    }
    
    OPENFILENAME openFileName = {};
    openFileName.lStructSize = sizeof(OPENFILENAME);
    char *filter = "Map File\0*.map\0\0";
    openFileName.lpstrFilter = filter;
    openFileName.lpstrFile = fileName;
    openFileName.nMaxFile = fileNameBufferSize;
    openFileName.Flags = OFN_OVERWRITEPROMPT;
    openFileName.nFileExtension = (WORD)charsToExtension;
    openFileName.lpstrDefExt = "map";
    
    BOOL result =  GetSaveFileName(&openFileName);
    
    if (result != 0)
    {
	HANDLE fileHandle = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ,
				       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
				       NULL);

	if (fileHandle != INVALID_HANDLE_VALUE)
	{
	    MapFileHeader fileHeader = {};
	    fileHeader.tileMapWidth = tileMap->widthInTiles;
	    fileHeader.tileMapHeight = tileMap->heightInTiles;
	    fileHeader.tileSize = tileMap->tileSize;

	    char *tempString = tileSetPanelGetCurrentTileSetFileName();
	    copyIntoString(fileHeader.tileSheetFileName, tempString);

	    copyIntoString(fileHeader.tileMapName, tileMap->name);
	    
	    uint32 tileMapSizeInBytes = tileMap->widthInTiles*tileMap->heightInTiles*sizeof(Tile);
	    uint32 bytesToWrite = sizeof(MapFileHeader) + tileMapSizeInBytes;
	    DWORD bytesWritten = 0;

	    void *bufferToWrite = HEAP_ALLOC(bytesToWrite);

	    for (uint32 i = 0; i < bytesToWrite; ++i)
	    {
		uint8 *nextBufferByte = (uint8*)bufferToWrite + i;
		uint8 *nextCopiedByte = 0;
		
		if (i < sizeof(fileHeader))
		{
		    nextCopiedByte = (uint8*)(&fileHeader) + i;
		}
		else
		{
		    nextCopiedByte = (uint8*)(&(tileMap->tiles + (i-sizeof(fileHeader))/sizeof(Tile))->tile) + (i-sizeof(fileHeader))%sizeof(Tile);
		}

		*nextBufferByte = *nextCopiedByte;
	    }
	    
	    WriteFile(fileHandle, bufferToWrite, bytesToWrite, &bytesWritten, NULL);

	    assert(bytesToWrite == bytesWritten);
	    
	    HEAP_FREE(bufferToWrite);
	    
	    CloseHandle(fileHandle);
	}
    }
}

static char* showOpenFileDialog(char *descriptionOfFile, char *fileExtension)
{
    char *result = 0;

    const uint32 fileNameSize = 512;
    char *fileNameBuffer = (char*)HEAP_ALLOC(fileNameSize);
    
    OPENFILENAME openFileName = {};
    openFileName.lStructSize = sizeof(OPENFILENAME);

    char filter[fileNameSize] = {};
    uint32 stringIndex;
    for (stringIndex = 0; descriptionOfFile[stringIndex] != 0; ++stringIndex)
    {
	filter[stringIndex] = descriptionOfFile[stringIndex];
    }

    ++stringIndex;
    filter[stringIndex++] = '*';
    filter[stringIndex++] = '.';
    for (uint32 i = 0; fileExtension[i] != 0; ++i)
    {
	filter[stringIndex++] = fileExtension[i];
    }
    
    openFileName.lpstrFilter = filter;
    openFileName.lpstrFile = fileNameBuffer;
    openFileName.nMaxFile = fileNameSize;
    openFileName.Flags = OFN_FILEMUSTEXIST;
    openFileName.lpstrDefExt = fileExtension;

    if (GetOpenFileName(&openFileName) != 0)
    {
	result = fileNameBuffer;
    }
    
    return result;
}

LoadTileMapResult loadTileMapFromFile()
{    
    char *fileDescription = "Tile Map file";
    char *fileExtension = "map";

    char *fileName = showOpenFileDialog(fileDescription, fileExtension);

    return loadTileMap(fileName);
}

char* getTileSheetFileName()
{
    char *fileDescription = "Tile Sheet file";
    char *fileExtension = "png;*.bmp";
    
    return showOpenFileDialog(fileDescription, fileExtension);
}
