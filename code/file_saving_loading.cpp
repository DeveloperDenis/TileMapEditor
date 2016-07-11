#include "file_saving_loading.h"
#include "main.h"
#include "stdio.h"
#include "windows.h"

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

    //TODO(denis): Windows wants you to use "Common Item Dialog" instead of this
    
    OPENFILENAME openFileName = {};
    openFileName.lStructSize = sizeof(OPENFILENAME);
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
	//TODO(denis): change the format of the save depending on what the file
	// extension is
	
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
    }
}

void loadTileSheet()
{
    OPENFILENAME openFileName = {};

    char fileNameBuffer[256] = {};
    
    openFileName.lStructSize = sizeof(OPENFILENAME);
    char *filter = "Tile Sheet file\0*.png\0\0";
    openFileName.lpstrFilter = filter;
    openFileName.lpstrFile = fileNameBuffer;
    openFileName.nMaxFile = 256;
    //TODO(denis): dunno if I want this in the save function
    //openFileName.lpstrFileTitle = ;
    openFileName.Flags = OFN_FILEMUSTEXIST;
    openFileName.lpstrDefExt = "png";
    
    BOOL result = GetOpenFileName(&openFileName);

    if (result != 0)
    {
	//TODO(denis): do the tile sheet processing
	OutputDebugStringA("you are trying to click the import button\n");
    }
}
