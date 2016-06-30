/* TODO(denis):
 * in the beginning, check the resolution of the user's monitor, and don't allow them
 * to make tile maps that would be too big!

 * Additionally, I think it would be cool if when you pressed TAB, your selected 
 * EditText changed to the next logical EditText
 * same with hitting Shift+TAB, it should make you select the previous text box
 *
 * add tile set loading
 *
 * slice up all the tiles in the loaded tileset and layout them out so they don't go
 * off screen
 *
 * add saving of tile maps to .js files
 * (support saving to different formats as well?)
 *
 * add saving to arbitrary folders
 *
 * loading a tilemap
 * (again in different formats)
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

// TODO(denis): have a Button and a TextButton ?
struct Button
{
    bool startedClick;
    TexturedRect background;
};

struct Vec2
{
    int x;
    int y;
};

struct Tile
{
    SDL_Rect pos;
    SDL_Rect sheetPos;
};

struct TileMap
{
    Tile *tiles;
    int tileSize;
    int widthInTiles;
    int heightInTiles;

    //TODO(denis): should this be in here?
    int offsetX;
    int offsetY;
};


static bool buttonClicked(Button button, int mouseX, int mouseY)
{
    return pointInRect(mouseX, mouseY, button.background.pos);
}

static Vec2 convertScreenPosToTilePos(TileMap *tileMap, Vec2 pos)
{
    Vec2 result = {};

    result.x = (pos.x - tileMap->offsetX)/tileMap->tileSize;
    result.y = (pos.y - tileMap->offsetY)/tileMap->tileSize;
    
    return result;
}

static TexturedRect loadImage(SDL_Renderer *renderer, char *fileName)
{
    TexturedRect result = {};
    
    SDL_Surface *tempSurf = IMG_Load(fileName);
    result.image = SDL_CreateTextureFromSurface(renderer, tempSurf);
    SDL_GetClipRect(tempSurf, &result.pos);
    SDL_FreeSurface(tempSurf);
    
    return result;
}

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

static void reorientEditingArea(SDL_Window *window, TileMap *tileMap, int padding)
{
    int windowHeight = 0;
    int windowWidth = 0;

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    int widthSpace = windowWidth - tileMap->tileSize*tileMap->widthInTiles;
    int heightSpace = windowHeight - tileMap->tileSize*tileMap->heightInTiles;

    if (widthSpace > heightSpace)
    {
        tileMap->offsetX = padding;
        tileMap->offsetY = windowHeight/2 - tileMap->tileSize*tileMap->heightInTiles/2;
    }
    else
    {
	tileMap->offsetY = padding;
        tileMap->offsetX = windowWidth/2 - tileMap->tileSize*tileMap->widthInTiles/2;
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
	    
	    //TODO(denis): where is this used?
	    int tileMapPadding = 30;
	    
	    char *tileSetFileName = "some_tiles.png";
	    Button currentTileSet = {};
	    
	    char *tileMapName = NULL;

	    TileMap tileMap = {};
	    tileMap.offsetX = 5;
	    tileMap.offsetY = 5;

	    bool selectionVisible = false;
	    Uint32 startSelectX = 0;
	    Uint32 startSelectY = 0;
	    TexturedRect selectionBox = {};
	    TexturedRect selectedTile = {};

	    const enum {
		MAP_NAME_TEXT,
		TILE_SIZE_TEXT,
		WIDTH_TEXT_PX,
		WIDTH_TEXT_TILES,
		HEIGHT_TEXT_PX,
		HEIGHT_TEXT_TILES
	    };

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

	    const enum { PAINT_TOOL,
		   FILL_TOOL
	    };
	    Uint32 currentTool = FILL_TOOL;
		
	    Button paintToolIcon = {};
	    paintToolIcon.background = loadImage(renderer, "paint-brush-icon-32.png");

	    Button fillToolIcon = {};
	    fillToolIcon.background = loadImage(renderer, "paint-can-icon-32.png");
		
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
				    reorientEditingArea(window, &tileMap, tileMapPadding);
				    
				    Tile *row = tileMap.tiles;
				    for (int i = 0; i < tileMap.heightInTiles; ++i)
				    {
					Tile *element = row;
					for (int j = 0; j < tileMap.widthInTiles; ++j)
					{
					    element->pos.x = tileMap.offsetX + j*tileMap.tileSize;
					    element->pos.y = tileMap.offsetY + i*tileMap.tileSize;
					    ++element;
					}
					row += tileMap.widthInTiles;
				    }
				}
			    }
			    
			} break;

			case SDL_MOUSEMOTION:
			{
			    //TODO(denis): also handle when the user has focus on our
			    // window but has moved off of it
			    
			    Sint32 mouseX = event.motion.x;
			    Sint32 mouseY = event.motion.y;

			    if (currentTool == PAINT_TOOL && tileMap.tiles)
			    {
				selectionVisible = moveSelectionBox(&selectionBox, mouseX, mouseY, &tileMap, &currentTileSet.background);

				if (event.motion.state & SDL_BUTTON_LMASK)
				{
				    if (pointInRect(mouseX, mouseY, {tileMap.offsetX, tileMap.offsetY, tileMap.tileSize*tileMap.widthInTiles, tileMap.tileSize*tileMap.heightInTiles}))
				    {
					int tileX = (mouseX-tileMap.offsetX)/tileMap.tileSize;
					int tileY = (mouseY-tileMap.offsetY)/tileMap.tileSize;

					Tile *clicked = tileMap.tiles + tileX + tileY*tileMap.widthInTiles;
					clicked->sheetPos = selectedTile.pos;
				    }
				}
			    }
			    else if (currentTool == FILL_TOOL && tileMap.tiles)
			    {	
				if (event.motion.state & SDL_BUTTON_LMASK &&
				    startSelectX != 0 && startSelectY != 0)
				{
				    //TODO(denis): make this if into a function?
				    // or at least simplify it a bit
				    if (mouseX < tileMap.offsetX)
					int x = 0;
				    if (mouseY < tileMap.offsetY)
					int y = 0;
				    
				    Vec2 temp = {mouseX, mouseY};
				    Vec2 tilePos =
					convertScreenPosToTilePos(&tileMap, temp);

				    temp = {startSelectX, startSelectY};
				    Vec2 startedTilePos =
					convertScreenPosToTilePos(&tileMap, temp);

				    if (tilePos.x < 0)
					tilePos.x = 0;
				    else if (tilePos.x >= tileMap.widthInTiles)
					tilePos.x = tileMap.widthInTiles-1;

				    if (tilePos.y < 0)
					tilePos.y = 0;
				    else if (tilePos.y >= tileMap.heightInTiles)
					tilePos.y = tileMap.heightInTiles-1;
				    
				    
				    if (startedTilePos.x < tilePos.x)
				    {
					//TODO(denis): I do this same sort of calculation all the time
					// gotta be a better way
					selectionBox.pos.x = tileMap.offsetX + startedTilePos.x*tileMap.tileSize;
					selectionBox.pos.w = tileMap.tileSize + (tilePos.x - startedTilePos.x)*tileMap.tileSize;
				    }
				    else
				    {
					selectionBox.pos.x = tileMap.offsetX + tilePos.x*tileMap.tileSize;
					//TODO(denis): having some sort of absolute value function/macro would
					// simplify this
					selectionBox.pos.w = tileMap.tileSize + (startedTilePos.x - tilePos.x)*tileMap.tileSize;
				    }


				    if (startedTilePos.y < tilePos.y)
				    {
					selectionBox.pos.y = tileMap.offsetY + startedTilePos.y*tileMap.tileSize;
					selectionBox.pos.h = tileMap.tileSize + (tilePos.y - startedTilePos.y)*tileMap.tileSize;
				    }
				    else
				    {
					selectionBox.pos.y = tileMap.offsetY + tilePos.y*tileMap.tileSize;
					selectionBox.pos.h = tileMap.tileSize + (startedTilePos.y - tilePos.y)*tileMap.tileSize;
				    }
				    
				}
				else
				{
				    selectionVisible = moveSelectionBox(&selectionBox, mouseX, mouseY, &tileMap, &currentTileSet.background);
				}
			    }
			} break;

			case SDL_MOUSEBUTTONDOWN:
			{
			    int x = event.button.x;
			    int y = event.button.y;

			    fillToolIcon.startedClick = buttonClicked(fillToolIcon, x, y);
			    paintToolIcon.startedClick = buttonClicked(paintToolIcon, x, y);
			    currentTileSet.startedClick = buttonClicked(currentTileSet, x, y);
			    
			    if (currentTool == PAINT_TOOL && tileMap.tiles)
			    {
				if (event.button.button == SDL_BUTTON_LEFT)
				{			    
				    if (pointInRect(x, y, {tileMap.offsetX, tileMap.offsetY, tileMap.tileSize*tileMap.widthInTiles, tileMap.tileSize*tileMap.heightInTiles}))
				    {
					Vec2 tilePos =
					    convertScreenPosToTilePos(&tileMap, {x,y});

					Tile *clicked = tileMap.tiles + tilePos.x + tilePos.y*tileMap.widthInTiles;
					clicked->sheetPos = selectedTile.pos;
				    }
				}
			    }
			    else if (currentTool == FILL_TOOL && tileMap.tiles)
			    {
				if (event.button.button == SDL_BUTTON_LEFT)
				{
				    if (pointInRect(x, y, {tileMap.offsetX, tileMap.offsetY, tileMap.tileSize*tileMap.widthInTiles, tileMap.tileSize*tileMap.heightInTiles}))
				    {
					Vec2 tilePos =
					    convertScreenPosToTilePos(&tileMap, {x,y});

					startSelectX = tileMap.offsetX + tilePos.x*tileMap.tileSize;
					startSelectY = tileMap.offsetY + tilePos.y*tileMap.tileSize;
					
					selectionBox.pos.x = startSelectX;
					selectionBox.pos.y = startSelectY;
					selectionBox.pos.w = tileMap.tileSize;
					selectionBox.pos.h = tileMap.tileSize;
				    }
				    else
				    {
					startSelectX = 0;
					startSelectY = 0;
				    }
				}
			    }
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
				    tileMap.tiles = (Tile*) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, memorySize);

				    if (tileMap.tiles)
				    {
					reorientEditingArea(window, &tileMap, tileMapPadding);
					
					Tile *row = tileMap.tiles;
					for (int i = 0; i < tileMap.heightInTiles; ++i)
					{
					    Tile *element = row;
					    for (int j = 0; j < tileMap.widthInTiles; ++j)
					    {
						SDL_Rect rect;
						
						rect.h = tileMap.tileSize;
						rect.w = tileMap.tileSize;
						rect.x = tileMap.offsetX + j*tileMap.tileSize;
						rect.y = tileMap.offsetY + i*tileMap.tileSize;
						
						element->pos = rect;

						++element;
					    }
					    row += tileMap.widthInTiles;
					}

					currentTileSet.background = loadImage(renderer,
								   tileSetFileName);

					selectionBox = createFilledTexturedRect(
					    renderer, tileMap.tileSize, tileMap.tileSize, 0x77FFFFFF);
					selectionVisible = moveSelectionBox(&selectionBox, x, y, &tileMap, &currentTileSet.background);
				    }
				}
				else
				{
				    OutputDebugStringA("you gotta fill it all out!\n");
				}
			    }

			    if (tileMap.tiles && buttonClicked(currentTileSet, x, y) &&
				currentTileSet.startedClick)
			    {
				selectedTile.pos.x = (selectionBox.pos.x - currentTileSet.background.pos.x)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.y = (selectionBox.pos.y - currentTileSet.background.pos.y)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.w = tileMap.tileSize;
				selectedTile.pos.h = tileMap.tileSize;
				
				selectedTile.image = currentTileSet.background.image;

				currentTileSet.startedClick = false;
			    }

			    //NOTE(denis): changing between the tools
			    if (tileMap.tiles)
			    {
				//TODO(denis): implement a proper "radio button" type
				// situation
				if (buttonClicked(paintToolIcon, x, y) &&
				    paintToolIcon.startedClick)
				{
				    currentTool = PAINT_TOOL;
				    paintToolIcon.startedClick = false;
				}
				else if (buttonClicked(fillToolIcon, x, y) &&
					 fillToolIcon.startedClick)
				{
				    currentTool = FILL_TOOL;
				    fillToolIcon.startedClick = false;
				}
			    }

			    //NOTE(denis): tool behaviour
			    if (tileMap.tiles)
			    {
				if (currentTool == FILL_TOOL)
				{
				    //TODO(denis): STILL BROKEN WHEN SELECTING
				    // a new tile in the current tileset
				    // the draw doesn't select the correct tile and
				    // draws nothing
				    if (selectionVisible && startSelectX != 0 &&
					startSelectY != 0)
				    {
					Vec2 topLeft = {selectionBox.pos.x,
							selectionBox.pos.y};
				    
					Vec2 botRight = {selectionBox.pos.x+selectionBox.pos.w,
							 selectionBox.pos.y+selectionBox.pos.h};
				    
					Vec2 startTile =
					    convertScreenPosToTilePos(&tileMap, topLeft);
					Vec2 endTile =
					    convertScreenPosToTilePos(&tileMap, botRight);

					if (endTile.x > tileMap.widthInTiles)
					{
					    endTile.x = tileMap.widthInTiles;
					}
					if (endTile.y > tileMap.heightInTiles)
					{
					    endTile.y = tileMap.heightInTiles;
					}
				    
					for (int i = startTile.y; i < endTile.y; ++i)
					{
					    for (int j = startTile.x; j < endTile.x; ++j)
					    {
						(tileMap.tiles + i*tileMap.widthInTiles + j)->sheetPos = selectedTile.pos;
					    }
					}
				    }

				    selectionVisible = moveSelectionBox(&selectionBox, x, y,
									&tileMap, &currentTileSet.background);
				    selectionBox.pos.w = tileMap.tileSize;
				    selectionBox.pos.h = tileMap.tileSize;
				    
				}
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

		SDL_SetRenderDrawColor(renderer, 15, 65, 95, 255);
		SDL_RenderClear(renderer);

		if (buttonVisible)
		    SDL_RenderCopy(renderer, createNewButton.image, NULL, &createNewButton.pos);

		drawTextFields();
		drawEditTexts();

		if (currentTileSet.background.image != NULL)
		{
		    int tileMapBottom = tileMap.offsetY + tileMap.heightInTiles*tileMap.tileSize;
		    int tileMapRight = tileMap.offsetX + tileMap.widthInTiles*tileMap.tileSize;
		    
		    if (tileMap.offsetX == tileMapPadding)
		    {
			//NOTE(denis): draw on right side
			//TODO(denis): draw it centered on the right side
			currentTileSet.background.pos.x = tileMapRight + 50;
			currentTileSet.background.pos.y = 50;

			paintToolIcon.background.pos.x = tileMapRight + 10;
			paintToolIcon.background.pos.y = 50;

			fillToolIcon.background.pos.x = tileMapRight + 10;
			fillToolIcon.background.pos.y = 90;
		    }
		    else if (tileMap.offsetY == tileMapPadding)
		    {
                        //NOTE(denis): draw on bottom
			//TODO(denis): draw it centered on the bottom
			currentTileSet.background.pos.x = 50;
			currentTileSet.background.pos.y = tileMapBottom + 50;
			
			paintToolIcon.background.pos.x = 50;
			paintToolIcon.background.pos.y = tileMapBottom + 10;

			fillToolIcon.background.pos.x = 90;
			fillToolIcon.background.pos.y = tileMapBottom + 10;
		    }

		    SDL_RenderCopy(renderer, currentTileSet.background.image, NULL, &currentTileSet.background.pos);
		    SDL_RenderCopy(renderer, paintToolIcon.background.image, NULL, &paintToolIcon.background.pos);
		    SDL_RenderCopy(renderer, fillToolIcon.background.image, NULL, &fillToolIcon.background.pos);
		}
		
		if (tileMap.tiles && tileMap.widthInTiles != 0 && tileMap.heightInTiles != 0)
		{
		    TexturedRect defaultTile = createFilledTexturedRect(renderer, tileMap.tileSize, tileMap.tileSize, 0xFFAAAA00);
		    Tile *row = tileMap.tiles;
		    for (int i = 0; i < tileMap.heightInTiles; ++i)
		    {
			Tile *element = row;
			for (int j = 0; j < tileMap.widthInTiles; ++j)
			{
			    if (element->sheetPos.w == 0 && element->sheetPos.h == 0)
			    {
				SDL_RenderCopy(renderer, defaultTile.image, NULL, &element->pos);
			    }
			    else
				SDL_RenderCopy(renderer, currentTileSet.background.image, &element->sheetPos, &element->pos);

			    ++element;
			}

			row += tileMap.widthInTiles;
		    }

		    SDL_DestroyTexture(defaultTile.image);
		    
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
