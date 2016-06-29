/* TODO(denis):
 * in the beginning, check the resolution of the user's monitor, and don't allow them
 * to make tile maps that would be too big!

 * Additionally, I think it would be cool if when you pressed TAB, your selected 
 * EditText changed to the next logical EditText
 * same with hitting Shift+TAB, it should make you select the previous text box
 *
 * have a "paint tool" and "fill tool" for drawing the tile maps
 */

#include <SDL.h>
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "windows.h"
#include <math.h>
#include "UIElements.h"
#include "TEMP_GeneralFunctions.cpp"

#define TITLE "Tile Map Editor"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

struct Tile
{
    SDL_Rect pos;
    SDL_Rect sheetPos;
};

struct TileMap
{
    Tile *tiles;
    Uint32 tileSize;
    Uint32 widthInTiles;
    Uint32 heightInTiles;
};

static bool moveSelectionBox(TexturedRect *selectionBox, Sint32 mouseX, Sint32 mouseY,
			     TileMap *tileMap, TexturedRect *otherArea)
{
    bool shouldBeDrawn = true;
    
    if (selectionBox->image != NULL)
    {
	Uint32 tileMapStartX = tileMap->tiles->pos.x;
	Uint32 tileMapStartY = tileMap->tiles->pos.y;
	Uint32 tileSize = tileMap->tileSize;

	if (pointInRect(mouseX, mouseY, {tileMapStartX, tileMapStartY, tileMap->widthInTiles*tileSize, tileMap->heightInTiles*tileSize}))
	{
	    selectionBox->pos.x = ((mouseX-tileMapStartX)/tileSize) * tileSize + tileMapStartX;
	    selectionBox->pos.y = ((mouseY-tileMapStartY)/tileSize) * tileSize + tileMapStartY;
	}
	else if (pointInRect(mouseX, mouseY, otherArea->pos))
	{
	    selectionBox->pos.x = ((mouseX-otherArea->pos.x)/tileMap->tileSize) * tileMap->tileSize + otherArea->pos.x;
	    selectionBox->pos.y = ((mouseY-otherArea->pos.y)/tileMap->tileSize) * tileMap->tileSize + otherArea->pos.y;
	}
	else
	    shouldBeDrawn = false;
    }

    return shouldBeDrawn;
}

static void reorientEditingArea(SDL_Window *window, int tileSize, int padding,
				int mapWidthInTiles, int mapHeightInTiles,
				int *mapStartingX, int *mapStartingY)
{
    int windowHeight = 0;
    int windowWidth = 0;

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    int widthSpace = windowWidth - tileSize*mapWidthInTiles;
    int heightSpace = windowHeight - tileSize*mapHeightInTiles;

    if (widthSpace > heightSpace)
    {
	*mapStartingX = padding;
        *mapStartingY = windowHeight/2 - tileSize*mapHeightInTiles/2;
    }
    else
    {
	*mapStartingY = padding;
        *mapStartingX = windowWidth/2 - tileSize*mapWidthInTiles/2;
    }
}

static TexturedRect createFilledTexturedRect(SDL_Renderer *renderer,
					     int width, int height, Uint32 colour)
{
    TexturedRect result = {};

    Uint32 rmask, gmask, bmask, amask;
    amask = 0xFF000000;
    bmask = 0x00FF0000;
    gmask = 0x0000FF00;
    rmask = 0x000000FF;
    
    SDL_Surface *rectangle = SDL_CreateRGBSurface(0, width, height, 32,
						  rmask, gmask, bmask, amask);
    SDL_FillRect(rectangle, NULL, colour);
    result.image = SDL_CreateTextureFromSurface(renderer, rectangle);
    SDL_GetClipRect(rectangle, &result.pos);
    SDL_FreeSurface(rectangle);

    return result;
}

static void fillBWithAConverted(float conversionFactor, EditText *A, EditText *B)
{
    int numberA = convertStringToInt(A->text, A->letterCount);
    int numberB = (int)(numberA*conversionFactor);
    
    while (B->letterCount > 0)
	editTextEraseLetter(B);

    int digitsB = 0;
    int tempB = numberB;
    while (tempB > 0)
    {
	tempB /= 10;
	++digitsB;
    }

    for (int i = 0; i < digitsB; ++i)
    {
	//TODO(denis): this calculation is probably pretty inefficient
	char letter = (char)(numberB % exponent(10, digitsB-i) / exponent(10, digitsB-i-1)) + '0';
	editTextAddLetter(B, letter, COLOUR_BLACK);
    }
}

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_Window *window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED,
					  SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
					  WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
        
    if (window)
    {
	//TODO(denis): maybe add renderer flags?
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer && initUI(renderer, "LiberationMono-Regular.ttf", 16)
	    && IMG_Init(IMG_INIT_PNG) != 0)
	{
	    bool running = true;
	    
	    int tileMapStartX = 5;
	    int tileMapStartY = 5;

	    int tileMapPadding = 15;
	    
	    char *tileSetFileName = "some_tiles.png";
	    TexturedRect currentTileSet = {};
	    
	    char *tileMapName = NULL;

	    TileMap tileMap = {};

	    bool selectionVisible = false;
	    TexturedRect selectionBox = {};
	    TexturedRect selectedTile = {};
	    
	    const int MAP_NAME_TEXT = 0;
	    const int TILE_SIZE_TEXT = 1;
	    const int WIDTH_TEXT_PX = 2;
	    const int HEIGHT_TEXT_PX = 3;
	    const int WIDTH_TEXT_TILES = 4;
	    const int HEIGHT_TEXT_TILES = 5;

	    addNewTextField("Tile Map Name: ", 100, 100, COLOUR_WHITE);
	    addNewTextField("Tile Size: ", 100, 150, COLOUR_WHITE);
	    addNewTextField("Width: ", 100, 200, COLOUR_WHITE);
	    addNewTextField("pixels", 455, 200, COLOUR_WHITE);
	    addNewTextField("tiles", 760, 200, COLOUR_WHITE);
	    addNewTextField("Height: ", 100, 250, COLOUR_WHITE);
	    addNewTextField("pixels", 455, 250, COLOUR_WHITE);
	    addNewTextField("tiles", 760, 250, COLOUR_WHITE);

	    addNewEditText(250, 100, 200, 20, COLOUR_WHITE, 2, MAP_NAME_TEXT);
	    addNewEditText(250, 150, 200, 20, COLOUR_WHITE,  2, TILE_SIZE_TEXT);
	    addNewEditText(250, 200, 200, 20, COLOUR_WHITE, 2, WIDTH_TEXT_PX);
	    addNewEditText(550, 200, 200, 20, COLOUR_WHITE, 2, WIDTH_TEXT_TILES);
	    addNewEditText(250, 250, 200, 20, COLOUR_WHITE, 2, HEIGHT_TEXT_PX);
	    addNewEditText(550, 250, 200, 20, COLOUR_WHITE, 2, HEIGHT_TEXT_TILES);
	    
	    char numberChars[] =  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0};
	    
	    EditText *tileSizeText = getEditTextByID(TILE_SIZE_TEXT);
	    tileSizeText->allowedCharacters = numberChars;

	    EditText *widthTextPx = getEditTextByID(WIDTH_TEXT_PX);
	    widthTextPx->allowedCharacters = numberChars;

	    EditText *widthTextTiles = getEditTextByID(WIDTH_TEXT_TILES);
	    widthTextTiles->allowedCharacters = numberChars;

	    EditText *heightTextPx = getEditTextByID(HEIGHT_TEXT_PX);
	    heightTextPx->allowedCharacters = numberChars;
	    
	    EditText *heightTextTiles = getEditTextByID(HEIGHT_TEXT_TILES);
	    heightTextTiles->allowedCharacters = numberChars;

	    bool buttonVisible = true;
	    TexturedRect createNewButton =
		createFilledTexturedRect(renderer, 200, 100, 0xFFFF0000);
	    createNewButton.pos.x = 500;
	    createNewButton.pos.y = 500;
	    addNewTextField("Create New Map", createNewButton.pos.x + 20,
			    createNewButton.pos.y + createNewButton.pos.h/2,
			    COLOUR_WHITE);
	    
	    
	    while (running)
	    {
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
		    switch(event.type)
		    {
			case SDL_QUIT:
			{
			    running = false;
			} break;

			case SDL_WINDOWEVENT:
			{
			    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
			    {
				if (tileMap.tiles)
				{
				    //TODO(denis): make this method take a TileMap as a parameter?
				    reorientEditingArea(window, tileMap.tileSize, tileMapPadding,
							tileMap.widthInTiles, tileMap.heightInTiles, &tileMapStartX, &tileMapStartY);
				    
				    Tile *row = tileMap.tiles;
				    for (Uint32 i = 0; i < tileMap.heightInTiles; ++i)
				    {
					Tile *element = row;
					for (Uint32 j = 0; j < tileMap.widthInTiles; ++j)
					{
					    element->pos.x = tileMapStartX + j*tileMap.tileSize;
					    element->pos.y = tileMapStartY + i*tileMap.tileSize;
					    ++element;
					}
					row += tileMap.widthInTiles;
				    }
				}
			    }
			    
			} break;

			case SDL_MOUSEMOTION:
			{
			    Sint32 mouseX = event.motion.x;
			    Sint32 mouseY = event.motion.y;

			    selectionVisible = moveSelectionBox(&selectionBox, mouseX, mouseY, &tileMap, &currentTileSet);
			} break;
			
			case SDL_MOUSEBUTTONUP:
			{
			    int x = event.button.x;
			    int y = event.button.y;

			    uiHandleClicks(x, y, event.button.button);

			    if (pointInRect(x, y, createNewButton.pos))
			    {
				EditText *nameText = getEditTextByID(MAP_NAME_TEXT);
				tileMapName = nameText->text;

				tileMap.tileSize = (Uint32)convertStringToInt(tileSizeText->text, tileSizeText->letterCount);
			        tileMap.widthInTiles = (Uint32)convertStringToInt(widthTextTiles->text, widthTextTiles->letterCount);
				tileMap.heightInTiles = (Uint32)convertStringToInt(heightTextTiles->text, heightTextTiles->letterCount);

				if (tileMapName && tileMap.tileSize != 0 &&
				    tileMap.widthInTiles != 0 && tileMap.heightInTiles != 0)
				{
				    deleteAllEditTexts();
				    deleteAllTextFields();
				    buttonVisible = false;
				    createNewButton.pos = {};
				    SDL_DestroyTexture(createNewButton.image);
				    createNewButton.image = NULL;

				    int memorySize = sizeof(Tile)*tileMap.widthInTiles*tileMap.heightInTiles;
				    
				    HANDLE heapHandle = GetProcessHeap();
				    tileMap.tiles = (Tile*) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY,
						        memorySize);

				    if (tileMap.tiles)
				    {
					reorientEditingArea(window, tileMap.tileSize, tileMapPadding, tileMap.widthInTiles, tileMap.heightInTiles,
							    &tileMapStartX, &tileMapStartY);
					
					Tile *row = tileMap.tiles;
					for (Uint32 i = 0; i < tileMap.heightInTiles; ++i)
					{
					    Tile *element = row;
					    for (Uint32 j = 0; j < tileMap.widthInTiles; ++j)
					    {
						SDL_Rect rect;
						
						rect.h = tileMap.tileSize;
						rect.w = tileMap.tileSize;
						rect.x = tileMapStartX + j*tileMap.tileSize;
						rect.y = tileMapStartY + i*tileMap.tileSize;
						
						element->pos = rect;

						++element;
					    }
					    row += tileMap.widthInTiles;
					}

					SDL_Surface *tiles = IMG_Load(tileSetFileName);
					currentTileSet.image = SDL_CreateTextureFromSurface(renderer, tiles);
					SDL_GetClipRect(tiles, &currentTileSet.pos);
					SDL_FreeSurface(tiles);

					selectionBox = createFilledTexturedRect(
					    renderer, tileMap.tileSize, tileMap.tileSize, 0x77FFFFFF);
					selectionVisible = moveSelectionBox(&selectionBox, x, y, &tileMap, &currentTileSet);
				    }
				}
				else
				{
				    OutputDebugStringA("you gotta fill it all out!\n");
				}
			    }


			    //TODO(denis): THERE MIGHT BE A MEMORY LEAK?
			    // probably check that out
			    if (tileMap.tiles &&
				pointInRect(x, y, {tileMapStartX, tileMapStartY, tileMap.tileSize*tileMap.widthInTiles, tileMap.tileSize*tileMap.heightInTiles}))
			    {
				int tileX = (x-tileMapStartX)/tileMap.tileSize;
				int tileY = (y-tileMapStartY)/tileMap.tileSize;

				Tile *clicked = tileMap.tiles + tileX + tileY*tileMap.widthInTiles;
				clicked->sheetPos = selectedTile.pos;
			    }
			    else if (tileMap.tiles &&
				     pointInRect(x, y, currentTileSet.pos))
			    {
				selectedTile.pos.x = (selectionBox.pos.x - currentTileSet.pos.x)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.y = (selectionBox.pos.y - currentTileSet.pos.y)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.w = tileMap.tileSize;
				selectedTile.pos.h = tileMap.tileSize;
				
				selectedTile.image = currentTileSet.image;
			    }
			    
			} break;

			case SDL_TEXTINPUT:
			{
			    char* theText = event.text.text;

			    editTextAddLetter(theText[0], COLOUR_BLACK);

			    int tileSize = convertStringToInt(tileSizeText->text, tileSizeText->letterCount);
			    
			    if (tileSizeText->selected)
			    {
			        fillBWithAConverted(1.0F/(float)tileSize, widthTextPx, widthTextTiles);
				fillBWithAConverted(1.0F/(float)tileSize, heightTextPx, heightTextTiles);
			    }
			    else if (tileSizeText->text[0] != 0)
			    {
				if (widthTextTiles->text[0] != 0 ||
				    widthTextPx->text[0] != 0)
				{
				    if (widthTextTiles->selected)
				    {
					fillBWithAConverted((float)tileSize, widthTextTiles, widthTextPx);
				    }
				    else if (widthTextPx->selected)
				    {
				        fillBWithAConverted(1.0F/(float)tileSize, widthTextPx, widthTextTiles);
				    }
				}
				if (heightTextTiles->text[0]!=0 ||
				    heightTextPx->text[0] != 0)
				{
				    if (heightTextTiles->selected)
				    {
					fillBWithAConverted((float)tileSize, heightTextTiles, heightTextPx);
				    }
				    else if (heightTextPx->selected)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, heightTextPx, heightTextTiles);
				    }
				}
			    }
			} break;

			case SDL_KEYDOWN:
			{			    
			    if (event.key.keysym.sym == SDLK_BACKSPACE)
			    {
				EditText *text = getSelectedEditText();
				editTextEraseLetter(text);
				
				if (text)
				{
				    int tileSize = convertStringToInt(
					tileSizeText->text, tileSizeText->letterCount);

				    if (text->id == TILE_SIZE_TEXT)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, widthTextPx, widthTextTiles);
					fillBWithAConverted(1.0F/(float)tileSize, heightTextPx, heightTextTiles);
				    }
				    else
				    {
					switch(text->id)
					{
					    case WIDTH_TEXT_PX:
					    {
						fillBWithAConverted(1.0F/(float)tileSize, text, widthTextTiles);
					    } break;

					    case WIDTH_TEXT_TILES:
					    {
						fillBWithAConverted((float)tileSize, text, widthTextPx);
					    } break;

					    case HEIGHT_TEXT_PX:
					    {
						fillBWithAConverted(1.0F/(float)tileSize, text, heightTextTiles);
					    } break;

					    case HEIGHT_TEXT_TILES:
					    {
						fillBWithAConverted((float)tileSize, text, heightTextPx);
					    } break;
					}
				    }
				}
			    }
			} break;
		    }
		}

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderClear(renderer);

		if (buttonVisible)
		    SDL_RenderCopy(renderer, createNewButton.image, NULL, &createNewButton.pos);

		drawTextFields();
		drawEditTexts();

		if (currentTileSet.image != NULL)
		{
		    if (tileMapStartX == tileMapPadding)
		    {
			//NOTE(denis): draw on right side
			//TODO(denis): draw it centred on the right side
			currentTileSet.pos.x = tileMapStartX + tileMap.widthInTiles*tileMap.tileSize + 50;
			currentTileSet.pos.y = 50;
		    }
		    else if (tileMapStartY == tileMapPadding)
		    {
                        //NOTE(denis): draw on bottom
			//TODO(denis): draw it centred on the bottom
			currentTileSet.pos.x = 50;
			currentTileSet.pos.y = tileMapStartY + tileMap.heightInTiles*tileMap.tileSize + 50;
		    }

		    SDL_RenderCopy(renderer, currentTileSet.image, NULL, &currentTileSet.pos);
		}
		
		if (tileMap.tiles && tileMap.widthInTiles != 0 && tileMap.heightInTiles != 0)
		{
		    TexturedRect defaultTile = createFilledTexturedRect(renderer, tileMap.tileSize, tileMap.tileSize, 0xFF00FFFF);
		    Tile *row = tileMap.tiles;
		    for (Uint32 i = 0; i < tileMap.heightInTiles; ++i)
		    {
			Tile *element = row;
			for (Uint32 j = 0; j < tileMap.widthInTiles; ++j)
			{
			    if (element->sheetPos.w == 0 && element->sheetPos.h == 0)
			    {
				SDL_RenderCopy(renderer, defaultTile.image, NULL, &element->pos);
			    }
			    else
				SDL_RenderCopy(renderer, currentTileSet.image, &element->sheetPos, &element->pos);

			    ++element;
			}

			row += tileMap.widthInTiles;
		    }

		    if (selectionVisible)
			SDL_RenderCopy(renderer, selectionBox.image, NULL, &selectionBox.pos);
		}
		
		SDL_RenderPresent(renderer);
	    }

	    IMG_Quit();
	    
	    if (tileMap.tiles)
		HeapFree(GetProcessHeap(), 0, tileMap.tiles);
	}
	
	destroyUI();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
    }

    SDL_Quit();
    return 0;
}
