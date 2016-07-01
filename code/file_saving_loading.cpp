#include "file_saving_loading.h"
#include "main.h"
#include <stdio.h>

//TODO(denis): eventually want to let the user choose where to save the file and the
// name
//TODO(denis): also want to be able to save in different formats
void saveTileMapToFile(TileMap *tileMap, char *tileMapName)
{
    FILE *outputFile = NULL;

    if (tileMapName)
    {
	char fileName[128];
	bool done = false;
	for (int i = 0; i < 128 && !done; ++i)
	{
	    if (tileMapName[i] == '\0')
	    {
		done = true;
		fileName[i] = '.';
		fileName[i+1] = 'j';
		fileName[i+2] = 's';
		fileName[i+3] = '\0';
	    }
	    else
		fileName[i] = tileMapName[i];
	}
	
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
