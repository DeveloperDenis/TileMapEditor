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

#include "main.h"
#include "file_saving_loading.h"
#include "denis_math.h"
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
    
    char *text;
    TexturedRect foreground;

    void setPosition(Vector2);
    void destroy();
};
void Button::setPosition(Vector2 newPos)
{
    this->background.pos.x = newPos.x;
    this->background.pos.y = newPos.y;

    //TODO(denis): if centre/right/left aligned I would want to change this
    this->foreground.pos.x = newPos.x;
    this->foreground.pos.y = newPos.y;
}

void Button::destroy()
{
    if (this->background.image)
	SDL_DestroyTexture(this->background.image);

    if (this->foreground.image)
    	SDL_DestroyTexture(this->foreground.image);

    this->text = NULL;
    this->startedClick = false;
    this->background = {};
    this->foreground = {};
}

static void draw(SDL_Renderer *renderer, Button *button)
{
    if (button->background.image)
	SDL_RenderCopy(renderer, button->background.image, NULL, &button->background.pos);

    if (button->text && button->foreground.image)
    {
	SDL_RenderCopy(renderer, button->foreground.image, NULL, &button->foreground.pos);
    }
}

static bool buttonClicked(Button button, Vector2 mouse)
{
    return pointInRect(mouse, button.background.pos);
}

static Vector2 convertScreenPosToTilePos(TileMap *tileMap, Vector2 pos)
{
    Vector2 result = {};

    result = (pos - tileMap->offset)/tileMap->tileSize;
     
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

static bool moveSelectionBox(TexturedRect *selectionBox, Vector2 mouse,
			     TileMap *tileMap, TexturedRect *otherArea)
{
    bool shouldBeDrawn = true;
    
    if (selectionBox->image != NULL)
    {	
	Uint32 tileSize = tileMap->tileSize;

	if (pointInRect(mouse, tileMap->getRect()))
	{
	    Vector2 newPos = ((mouse - tileMap->offset)/tileSize) * tileSize + tileMap->offset;
	    selectionBox->pos.x = newPos.x;
	    selectionBox->pos.y = newPos.y;
	}
	else if (pointInRect(mouse, otherArea->pos))
	{
	    Vector2 otherPos = {otherArea->pos.x, otherArea->pos.y};

	    //TODO(denis): this seems dumb
	    // get rid of / tileSize * tileSize
	    Vector2 newPos = (mouse - otherPos)/tileSize * tileSize + otherPos;
	    selectionBox->pos.x = newPos.x;
	    selectionBox->pos.y = newPos.y;
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
	int offsetY = windowHeight/2 - tileMap->tileSize*tileMap->heightInTiles/2;
	tileMap->offset = {padding, offsetY};
    }
    else
    {
        int offsetX = windowWidth/2 - tileMap->tileSize*tileMap->widthInTiles/2;
	tileMap->offset = {offsetX, padding};
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

//TODO(denis): perhaps have multiple versions that do a different kind of
// TTF_RenderText, like Shaded and Solid
static TexturedRect createNewTextField(SDL_Renderer* renderer,
				       char* text, int x, int y, SDL_Color colour)
{
    TexturedRect result = {};
    
    SDL_Surface* tempSurf = TTF_RenderText_Blended(getFont(), text, colour);

    SDL_GetClipRect(tempSurf, &result.pos);
    result.pos.x = x;
    result.pos.y = y;

    result.image = SDL_CreateTextureFromSurface(renderer, tempSurf);
    SDL_FreeSurface(tempSurf);

    return result;
}

static Button createNewTextButton(SDL_Renderer *renderer,
				  char *text, SDL_Colour textColour,
				  int width, int height, Uint32 backgroundColour)
{
    Button result = {};

    result.text = text;
    result.background = createFilledTexturedRect(renderer, width, height, backgroundColour);

    //TODO(denis): add options for left align, centre aligned, and right aligned text
    result.foreground = createNewTextField(renderer, text, result.background.pos.x,
					result.background.pos.y, textColour);
    
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
	    
	    //TODO(denis): i don't like this loose variable here
	    int tileMapPadding = 30;
	    
	    char *tileSetFileName = "some_tiles.png";
	    Button currentTileSet = {};
	    
	    char *tileMapName = NULL;

	    TileMap tileMap = {};
	    tileMap.offset = {5, 5};

	    bool selectionVisible = false;
	    Vector2 startSelectPos = {};
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

	    Button newTileMapButton =
		createNewTextButton(renderer, "Create New Map", COLOUR_WHITE,
				    200, 100, 0xFFFF0000);
	    newTileMapButton.setPosition({500, 500});
	    bool buttonVisible = true;

	    
	    Button saveButton = {};
	    
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
					    element->pos.x = tileMap.offset.x + j*tileMap.tileSize;
					    element->pos.y = tileMap.offset.y + i*tileMap.tileSize;
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

			    Vector2 mouse = {event.motion.x, event.motion.y};
			    
			    if (currentTool == PAINT_TOOL && tileMap.tiles)
			    {
				selectionVisible = moveSelectionBox(&selectionBox, mouse, &tileMap, &currentTileSet.background);

				if (event.motion.state & SDL_BUTTON_LMASK)
				{
				    if (pointInRect(mouse, tileMap.getRect()))
				    {
					Vector2 tile =
					    (mouse - tileMap.offset)/tileMap.tileSize;

					Tile *clicked = tileMap.tiles + tile.x + tile.y*tileMap.widthInTiles;
					clicked->sheetPos = selectedTile.pos;
				    }
				}
			    }
			    else if (currentTool == FILL_TOOL && tileMap.tiles)
			    {	
				if (event.motion.state & SDL_BUTTON_LMASK &&
				    startSelectPos != Vector2{0,0})
				{
				    //TODO(denis): make this if into a function?
				    // or at least simplify it a bit
				    if (mouse.x < tileMap.offset.x)
					int x = 0;
				    if (mouse.y < tileMap.offset.y)
					int y = 0;
				    
				    Vector2 tilePos =
					convertScreenPosToTilePos(&tileMap, mouse);

				    Vector2 startedTilePos =
					convertScreenPosToTilePos(&tileMap, startSelectPos);

				    if (tilePos.x < 0)
					tilePos.x = 0;
				    else if (tilePos.x >= tileMap.widthInTiles)
					tilePos.x = tileMap.widthInTiles-1;

				    if (tilePos.y < 0)
					tilePos.y = 0;
				    else if (tilePos.y >= tileMap.heightInTiles)
					tilePos.y = tileMap.heightInTiles-1;
				    

				    //TODO(denis): I do this same sort of calculation all the time
				    // gotta be a better way
				    if (startedTilePos.x < tilePos.x)
					selectionBox.pos.x = tileMap.offset.x + startedTilePos.x*tileMap.tileSize;
					
				    else
					selectionBox.pos.x = tileMap.offset.x + tilePos.x*tileMap.tileSize;
				    
				    selectionBox.pos.w = tileMap.tileSize + absValue(tilePos.x - startedTilePos.x)*tileMap.tileSize;
				    

				    if (startedTilePos.y < tilePos.y)
					selectionBox.pos.y = tileMap.offset.y + startedTilePos.y*tileMap.tileSize;
				    else
					selectionBox.pos.y = tileMap.offset.y + tilePos.y*tileMap.tileSize;

				    selectionBox.pos.h = tileMap.tileSize + absValue(tilePos.y - startedTilePos.y)*tileMap.tileSize;
				    
				}
				else
				{
				    selectionVisible = moveSelectionBox(&selectionBox, mouse, &tileMap, &currentTileSet.background);
				}
			    }
			} break;

			case SDL_MOUSEBUTTONDOWN:
			{
			    Vector2 mouse = {event.button.x, event.button.y};

			    fillToolIcon.startedClick = buttonClicked(fillToolIcon, mouse);
			    paintToolIcon.startedClick = buttonClicked(paintToolIcon, mouse);
			    currentTileSet.startedClick = buttonClicked(currentTileSet, mouse);
			    saveButton.startedClick = buttonClicked(saveButton, mouse);
			    
			    if (currentTool == PAINT_TOOL && tileMap.tiles)
			    {
				if (event.button.button == SDL_BUTTON_LEFT)
				{			    
				    if (pointInRect(mouse, tileMap.getRect()))
				    {
					Vector2 tilePos =
					    convertScreenPosToTilePos(&tileMap, mouse);

					Tile *clicked = tileMap.tiles + tilePos.x + tilePos.y*tileMap.widthInTiles;
					clicked->sheetPos = selectedTile.pos;
				    }
				}
			    }
			    else if (currentTool == FILL_TOOL && tileMap.tiles)
			    {
				if (event.button.button == SDL_BUTTON_LEFT)
				{
				    if (pointInRect(mouse, tileMap.getRect()))
				    {
					Vector2 tilePos =
					    convertScreenPosToTilePos(&tileMap, mouse);

					startSelectPos = tileMap.offset + tilePos*tileMap.tileSize;
					
					selectionBox.pos.x = startSelectPos.x;
					selectionBox.pos.y = startSelectPos.y;
					selectionBox.pos.w = tileMap.tileSize;
					selectionBox.pos.h = tileMap.tileSize;
				    }
				    else
				    {
					startSelectPos = {0, 0};
				    }
				}
			    }
			} break;
			
			case SDL_MOUSEBUTTONUP:
			{
			    Vector2 mouse = {event.button.x, event.button.y};

			    uiHandleClicks(mouse, event.button.button);
			    
			    if (pointInRect(mouse, newTileMapButton.background.pos))
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
				    newTileMapButton.destroy();
				    
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
						rect.x = tileMap.offset.x + j*tileMap.tileSize;
						rect.y = tileMap.offset.y + i*tileMap.tileSize;
						
						element->pos = rect;

						++element;
					    }
					    row += tileMap.widthInTiles;
					}

					currentTileSet.background = loadImage(renderer,
								   tileSetFileName);

					selectionBox = createFilledTexturedRect(
					    renderer, tileMap.tileSize, tileMap.tileSize, 0x77FFFFFF);
					selectionVisible = moveSelectionBox(&selectionBox, mouse, &tileMap, &currentTileSet.background);

					saveButton = createNewTextButton(renderer, "Save", COLOUR_WHITE,
						    100, 50, 0xFF33AA88);
				    }
				}
				else
				{
				    OutputDebugStringA("you gotta fill it all out!\n");
				}
			    }

			    if (tileMap.tiles && buttonClicked(currentTileSet, mouse) &&
				currentTileSet.startedClick)
			    {
				selectedTile.pos.x = (selectionBox.pos.x - currentTileSet.background.pos.x)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.y = (selectionBox.pos.y - currentTileSet.background.pos.y)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.w = tileMap.tileSize;
				selectedTile.pos.h = tileMap.tileSize;
				
				selectedTile.image = currentTileSet.background.image;

				currentTileSet.startedClick = false;
			    }

			    //NOTE(denis): hit the save button
			    if (tileMap.tiles && saveButton.startedClick)
			    {
				OutputDebugStringA("you have saved the data\n");

				//TODO(denis): actually save data
				saveTileMapToFile(&tileMap, tileMapName);
				
				saveButton.startedClick = false;
			    }
			    
			    //NOTE(denis): changing between the tools
			    if (tileMap.tiles)
			    {
				//TODO(denis): implement a proper "radio button" type
				// situation
				if (buttonClicked(paintToolIcon, mouse) &&
				    paintToolIcon.startedClick)
				{
				    currentTool = PAINT_TOOL;
				    paintToolIcon.startedClick = false;
				}
				else if (buttonClicked(fillToolIcon, mouse) &&
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
				    if (selectionVisible &&
					startSelectPos != Vector2{0,0})
				    {
					Vector2 topLeft = {selectionBox.pos.x,
							selectionBox.pos.y};
				    
					Vector2 botRight = {selectionBox.pos.x+selectionBox.pos.w,
							 selectionBox.pos.y+selectionBox.pos.h};
				    
					Vector2 startTile =
					    convertScreenPosToTilePos(&tileMap, topLeft);
					Vector2 endTile =
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

				    selectionVisible = moveSelectionBox(&selectionBox, mouse,
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
		{
		    draw(renderer, &newTileMapButton);
		}
		
		draw(renderer, &saveButton);

		drawTextFields();
		drawEditTexts();

		if (currentTileSet.background.image != NULL)
		{
		    int tileMapBottom = tileMap.offset.y + tileMap.heightInTiles*tileMap.tileSize;
		    int tileMapRight = tileMap.offset.x + tileMap.widthInTiles*tileMap.tileSize;
		    
		    if (tileMap.offset.x == tileMapPadding)
		    {
			//NOTE(denis): draw on right side
			//TODO(denis): draw it centered on the right side
			currentTileSet.background.pos.x = tileMapRight + 50;
			currentTileSet.background.pos.y = 50;

			Vector2 temp = {currentTileSet.background.pos.x + 20,
					currentTileSet.background.pos.y + 100};
			saveButton.setPosition(temp);

			paintToolIcon.background.pos.x = tileMapRight + 10;
			paintToolIcon.background.pos.y = 50;

			fillToolIcon.background.pos.x = tileMapRight + 10;
			fillToolIcon.background.pos.y = 90;
		    }
		    else if (tileMap.offset.y == tileMapPadding)
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
