#include "file_saving_loading.h"
#include "main.h"

#define WINDOWS_API

#if defined(C_STD_LIB)
#include <stdio.h>
#elif defined(WINDOWS_API)
#include "windows.h"
#endif

//TODO(denis): eventually want to let the user choose where to save the file and the
// name
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
	    fileName[i+1] = 'j';
	    fileName[i+2] = 's';
	    fileName[i+3] = '\0';
	}
	else
	    fileName[i] = tileMapName[i];
    }
    
#if defined(C_STD_LIB)
    FILE *outputFile = NULL;

    if (tileMapName)
    {
        if (fopen_s(&outputFile, fileName, "w") == 0)
	{
	    fprintf(outputFile, "var %s = [", tileMapName);

	    for (int i = 0; i < tileMap->heightInTiles; ++i)
	    {
		fprintf(outputFile, "[");
		
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

		    if (j < tileMap->widthInTiles-1)
			fprintf(outputFile, ", ");
		}

		fprintf(outputFile, "]");

		if (i < tileMap->heightInTiles-1)
		    fprintf(outputFile, ",\n");
		else
		    fprintf(outputFile, "\n");
	    }
	    
	    fprintf(outputFile, "];");
	    
	    fclose(outputFile);
	}
    }
#elif defined(WINDOWS_API)

    //TODO(denis): Windows wants you to use "Common Item Dialog" instead of this
    
    OPENFILENAME openFileName = {};
    openFileName.lStructSize = sizeof(OPENFILENAME);
//TODO(denis): if required can use SDL_GetWindowWMInfo()
//openFileName.hwndOwner = ;
    char *filter = "JavaScript File\0*.js\0Text File\0*.txt\0\0";
    openFileName.lpstrFilter = filter;
    openFileName.lpstrFile = fileName;
    openFileName.nMaxFile = fileNameBufferSize;
    //TODO(denis): dunno if I want this in the save function
    //openFileName.lpstrFileTitle = ;
    openFileName.Flags = OFN_OVERWRITEPROMPT;
    openFileName.nFileExtension = (WORD)charsToExtension;
    openFileName.lpstrDefExt = "js";
    
    BOOL result =  GetSaveFileName(&openFileName);

    if (result != 0)
    {
	OutputDebugStringA("holy moly it works\n");
    }
    
#endif
}
