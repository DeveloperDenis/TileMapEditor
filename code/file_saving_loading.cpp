#include "ui_elements.h"
#include "file_saving_loading.h"
#include "main.h"
#include "tile_map_panel.h"
#include "stdio.h"
#include "windows.h"

//TODO(denis): also want to be able to save in different formats
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
	    fileName[i+1] = 't';
	    fileName[i+2] = 'x';
	    fileName[i+3] = 't';
	    fileName[i+4] = '\0';
	}
	else
	    fileName[i] = tileMapName[i];
    }

    //TODO(denis): Windows wants you to use "Common Item Dialog" instead of this
    
    OPENFILENAME openFileName = {};
    openFileName.lStructSize = sizeof(OPENFILENAME);
    char *filter = "Text File\0*.txt\0\0";
    openFileName.lpstrFilter = filter;
    openFileName.lpstrFile = fileName;
    openFileName.nMaxFile = fileNameBufferSize;
    //TODO(denis): dunno if I want this in the save function
    //openFileName.lpstrFileTitle = ;
    openFileName.Flags = OFN_OVERWRITEPROMPT;
    openFileName.nFileExtension = (WORD)charsToExtension;
    openFileName.lpstrDefExt = "txt";
    
    BOOL result =  GetSaveFileName(&openFileName);

    if (result != 0)
    {
	//TODO(denis): change the format of the save depending on what the file
	// extension is
	
	FILE *outputFile = NULL;

	if (tileMapName)
	{
	    if (fopen_s(&outputFile, fileName, "w") == 0)
	    {
		for (int i = 0; i < tileMap->heightInTiles; ++i)
		{
		    for (int j = 0; j < tileMap->widthInTiles; ++j)
		    {
			Tile* tile = (tileMap->tiles + i*tileMap->widthInTiles + j);
			int tileValue;

			if (tile->sheetPos.w != 0 && tile->sheetPos.h != 0)
			{
			    //TODO(denis): dunno if this expression is correct
			    tileValue = tile->sheetPos.x / tileMap->tileSize +
				tile->sheetPos.y/tileMap->tileSize*tileMap->widthInTiles;
			}
			else
			{
			    tileValue = -1;
			}
		    
			fprintf(outputFile, "%d", tileValue);
		    }

		    fprintf(outputFile, "\n");
		}
	    
		fclose(outputFile);
	    }
	}
    }
}

char* getTileSheetFileName()
{
    char *result = 0;
    
    OPENFILENAME openFileName = {};

    //TODO(denis): sizeof(char) should always be 1 right?
    int fileNameSize = 512*sizeof(char);
    char* fileNameBuffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					 fileNameSize);
    
    openFileName.lStructSize = sizeof(OPENFILENAME);
    char *filter = "Tile Sheet file\0*.png\0\0";
    openFileName.lpstrFilter = filter;
    openFileName.lpstrFile = fileNameBuffer;
    openFileName.nMaxFile = fileNameSize/sizeof(char);
    //TODO(denis): dunno if I want this in the save function
    //openFileName.lpstrFileTitle = ;
    openFileName.Flags = OFN_FILEMUSTEXIST;
    openFileName.lpstrDefExt = "png";
    
    BOOL fileNameFound = GetOpenFileName(&openFileName);

    if (fileNameFound != 0)
    {
	result = fileNameBuffer;
    }

    return result;
}
