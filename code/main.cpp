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
 * (support saving to different formats as well?)
 *
 * loading a tilemap
 * (again in different formats)
 *
 * limit the FPS
 */

#include <SDL.h>
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "windows.h"
#include <math.h>

#include "main.h"
#include "file_saving_loading.h"
#include "denis_math.h"
#include "TEMP_GeneralFunctions.cpp"

#include "ui_elements.h"

#define TITLE "Tile Map Editor"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

static SDL_Color hexColourToRGBA(Uint32 hex)
{
    SDL_Color result = {};

    //NOTE(denis): hex colour format 0xAARRGGBB
    result.r = (hex >> 16) & 0xFF;
    result.g = (hex >> 8) & 0xFF;
    result.b = hex & 0xFF;
    result.a = hex >> 24;

    return result;
}

static Vector2 convertScreenPosToTilePos(TileMap *tileMap, Vector2 pos)
{
    Vector2 result = {};

    result = (pos - tileMap->offset)/tileMap->tileSize;
     
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

	    Vector2 newPos = (mouse - otherPos)/tileSize * tileSize + otherPos;
	    selectionBox->pos.x = newPos.x;
	    selectionBox->pos.y = newPos.y;
	}
	else
	    shouldBeDrawn = false;
    }

    return shouldBeDrawn;
}

static void reorientEditingArea(int windowWidth, int windowHeight, TileMap *tileMap, int padding,
				int *menuX, int *menuY)
{   
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

    //TODO(denis): hard-coding not good
    *menuY = tileMap->offset.y - 50;
    *menuX = tileMap->offset.x + 10;
}

static void fillBWithAConverted(float conversionFactor, EditText *A, EditText *B)
{
    int numberA = convertStringToInt(A->text, A->letterCount);
    int numberB = (int)(numberA*conversionFactor);

    while (B->letterCount > 0)
        ui_eraseLetter(B);

    
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
	ui_addLetterTo(B, letter);
    }
}

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_Window *window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED,
					  SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
					  WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

    char *defaultFontName= "LiberationMono-Regular.ttf";
    int defaultFontSize = 16;

    char *menuFontName = "LiberationMono-Regular.ttf";
    int menuFontSize = 14;
    
    if (window)
    {
	//TODO(denis): maybe add renderer flags?
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer && ui_init(renderer, defaultFontName, defaultFontSize)
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

	    int newTileMapGroup = 0;

	    Panel createTileMapPanel = {};
	    
	    TexturedRect tileMapNameText =
		ui_createTextField("Tile Map Name: ", 100, 100, COLOUR_WHITE);
	    newTileMapGroup = ui_addToGroup(&tileMapNameText);

	    TexturedRect tileSizeText =
		ui_createTextField("Tile Size: ", 100, 150, COLOUR_WHITE);
	    ui_addToGroup(&tileSizeText, newTileMapGroup);
	    
	    TexturedRect widthText =
		ui_createTextField("Width: ", 100, 200, COLOUR_WHITE);
	    ui_addToGroup(&widthText, newTileMapGroup);
	    
	    TexturedRect pixelsText =
		ui_createTextField("pixels", 455, 200, COLOUR_WHITE);
	    ui_addToGroup(&pixelsText, newTileMapGroup);
	    
	    TexturedRect tilesText =
		ui_createTextField("tiles", 760, 200, COLOUR_WHITE);
	    ui_addToGroup(&tilesText, newTileMapGroup);
	    
	    TexturedRect heightText =
		ui_createTextField("Height: ", 100, 250, COLOUR_WHITE);
	    ui_addToGroup(&heightText, newTileMapGroup);
	    
	    TexturedRect pixelsText2 =
		ui_createTextField("pixels", 455, 250, COLOUR_WHITE);
	    ui_addToGroup(&pixelsText2, newTileMapGroup);
	    
	    TexturedRect tilesText2 =
		ui_createTextField("tiles", 760, 250, COLOUR_WHITE);
	    ui_addToGroup(&tilesText2, newTileMapGroup);
	    
	    char numberChars[] =  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0};
	    
	    EditText tileMapNameEditText =
		ui_createEditText(250, 100, 200, 20, COLOUR_WHITE, 2);
	    newTileMapGroup = ui_addToGroup(&tileMapNameEditText);
	    
	    EditText tileSizeEditText =
		ui_createEditText(250, 150, 200, 20, COLOUR_WHITE, 2);
	    tileSizeEditText.allowedCharacters = numberChars;
	    ui_addToGroup(&tileSizeEditText, newTileMapGroup);
	    
	    EditText widthPxEditText =
		ui_createEditText(250, 200, 200, 20, COLOUR_WHITE,  2);
	    widthPxEditText.allowedCharacters = numberChars;
	    ui_addToGroup(&widthPxEditText, newTileMapGroup);
	    
	    EditText widthTilesEditText =
		ui_createEditText(550, 200, 200, 20, COLOUR_WHITE, 2);
	    widthTilesEditText.allowedCharacters = numberChars;
	    ui_addToGroup(&widthTilesEditText, newTileMapGroup);
	    
	    EditText heightPxEditText =
		ui_createEditText(250, 250, 200, 20, COLOUR_WHITE, 2);
	    heightPxEditText.allowedCharacters = numberChars;
	    ui_addToGroup(&heightPxEditText, newTileMapGroup);
	    
	    EditText heightTilesEditText =
		ui_createEditText(550, 250, 200, 20, COLOUR_WHITE, 2);
	    heightTilesEditText.allowedCharacters = numberChars;
	    ui_addToGroup(&heightTilesEditText, newTileMapGroup);
	    
	    Button newTileMapButton =
		ui_createTextButton("Create New Map", COLOUR_WHITE,
				    200, 100, 0xFFFF0000);
	    newTileMapButton.setPosition({500, 500});
	    ui_addToGroup(&newTileMapButton, newTileMapGroup);

	    ui_setFont(menuFontName, menuFontSize);

	    //TODO(denis): update this as windows shrinks and grows
	    int fileMenuWidth = WINDOW_WIDTH;
	    int fileMenuHeight = 20;
	    Uint32 fileMenuColour = 0xFFCCCCCC;
	    char *items[] = {"File", "Create New Tile Map", "Save current tile map", "Exit"};
	    //TODO(denis): center align text	     
	    DropDownMenu fileMenu = ui_createDropDownMenu(items, 4, 225, fileMenuHeight, COLOUR_BLACK,
							  fileMenuColour);
	    
	    TextBox item1 = {};
	    TextBox item2 = {};
	    TextBox item3 = {};

	    TextBox *dropDown[] = {&item1, &item2, &item3};
	    bool menuOpen = false;
	    int menuX = 0;
	    int menuY = 0;
	    int selectedItem = 0;
	    int menuLength = 3;
	    int menuPause = 0;

	    ui_setFont(defaultFontName, defaultFontSize);
	    
	    Button saveButton = {};
	    
	    const enum { PAINT_TOOL,
		   FILL_TOOL
	    };
	    Uint32 currentTool = FILL_TOOL;
	    
	    Button paintToolIcon = ui_createImageButton("paint-brush-icon-32.png");
	    Button fillToolIcon = ui_createImageButton("paint-can-icon-32.png");
		
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
				int windowHeight = 0;
				int windowWidth = 0;
				SDL_GetWindowSize(window, &windowWidth, &windowHeight);

				fileMenuWidth = windowWidth;
				
				if (tileMap.tiles)
				{
				    reorientEditingArea(windowWidth, windowHeight, &tileMap, tileMapPadding,
							&menuX, &menuY);
				    
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

			    if (pointInRect(mouse, fileMenu.getRect()))
			    {
				SDL_Rect tempRect = fileMenu.getRect();
				int selectedY = (mouse.y - tempRect.y)/fileMenu.items[0].pos.h;
				fileMenu.highlightedItem = selectedY;
			    }
			    else
				fileMenu.highlightedItem = -1;
			    
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

			    ui_processMouseDown(mouse, event.button.button);
			    
			    //TODO(denis): replace this with the ui call
			    fillToolIcon.startedClick = pointInRect(mouse, fillToolIcon.background.pos);
			    paintToolIcon.startedClick = pointInRect(mouse, paintToolIcon.background.pos);
			    currentTileSet.startedClick = pointInRect(mouse, currentTileSet.background.pos);
			    saveButton.startedClick = pointInRect(mouse, saveButton.background.pos);

			    fileMenu.startedClick = pointInRect(mouse, fileMenu.getRect());
			    
			    if (menuPause > 15 &&
				currentTool == PAINT_TOOL && tileMap.tiles)
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

			    ui_processMouseUp(mouse, event.button.button);
			    
			    if (ui_wasClicked(newTileMapButton, mouse))
			    {
				tileMapName = tileMapNameEditText.text;

				tileMap.tileSize = (Uint32)convertStringToInt(tileSizeEditText.text, tileSizeEditText.letterCount);
			        tileMap.widthInTiles = (Uint32)convertStringToInt(widthTilesEditText.text, widthTilesEditText.letterCount);
				tileMap.heightInTiles = (Uint32)convertStringToInt(heightTilesEditText.text, heightTilesEditText.letterCount);
				
				if (tileMapName && tileMap.tileSize != 0 &&
				    tileMap.widthInTiles != 0 && tileMap.heightInTiles != 0)
				{   
				    ui_deleteGroup(newTileMapGroup);
				    
				    int memorySize = sizeof(Tile)*tileMap.widthInTiles*tileMap.heightInTiles;
				    
				    HANDLE heapHandle = GetProcessHeap();
				    tileMap.tiles = (Tile*) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, memorySize);

				    if (tileMap.tiles)
				    {
					int windowHeight = 0;
					int windowWidth = 0;
					SDL_GetWindowSize(window, &windowWidth, &windowHeight);
					
					reorientEditingArea(windowWidth, windowHeight, &tileMap, tileMapPadding,
							    &menuX, &menuY);

					int textBoxWidth = tileMap.widthInTiles*tileMap.tileSize - 20;
					item1 = ui_createTextBox("first tile map", textBoxWidth, 35, COLOUR_BLACK,
								 0xFFFFFFFF);
					item2 = ui_createTextBox("second tile map", textBoxWidth, 35, COLOUR_BLACK,
								 0xFFFFFFFF);
					item3 = ui_createTextBox("third tile map", textBoxWidth, 35, COLOUR_BLACK,
								 0xFFFFFFFF);
					item1.setPosition({menuX, menuY});
					
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

					saveButton = ui_createTextButton("Save", COLOUR_WHITE,
									 100, 50, 0xFF33AA88);
				    }
				}
				else
				{
				    OutputDebugStringA("you gotta fill it all out!\n");
				}
			    }

			    //NOTE(denis): top file menu
			    if (fileMenu.isOpen && fileMenu.startedClick &&
				pointInRect(mouse, fileMenu.getRect()))
			    {
				SDL_Rect tempRect = fileMenu.getRect();
				int selectedY = (mouse.y - tempRect.y)/fileMenu.items[0].pos.h;
				if (selectedY == 0)
				{
				    fileMenu.isOpen = false;
				    fileMenu.startedClick = false;
				}
			    }
			    else if (!fileMenu.isOpen && fileMenu.startedClick &&
				     pointInRect(mouse, fileMenu.getRect()))
			    {
				fileMenu.isOpen = true;
			    }
			    else if (fileMenu.isOpen)
			    {
				fileMenu.isOpen = false;
			    }
			    
			    //TODO(denis): temporary drop down menu test
			    if (!menuOpen && tileMap.tiles && 
				pointInRect(mouse, dropDown[selectedItem]->pos))
			    {
				menuOpen = true;
			    }
			    else if (menuOpen)
			    {
				menuOpen = false;
				//TODO(denis): there are still bugs related to this
				menuPause = 0;

				for (int i = 0; i < menuLength; ++i)
				{
				    if (pointInRect(mouse, dropDown[i]->pos))
				    {
					selectedItem = i;
				    }
				}
			    }
			    
			    if (tileMap.tiles && ui_wasClicked(currentTileSet, mouse))
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
				saveTileMapToFile(&tileMap, tileMapName);
				
				saveButton.startedClick = false;
			    }
			    
			    //NOTE(denis): changing between the tools
			    if (tileMap.tiles)
			    {
				//TODO(denis): implement a proper "radio button" type
				// situation
				if (ui_wasClicked(paintToolIcon, mouse))
				{
				    currentTool = PAINT_TOOL;
				    paintToolIcon.startedClick = false;
				}
				else if (ui_wasClicked(fillToolIcon, mouse))
				{
				    currentTool = FILL_TOOL;
				    fillToolIcon.startedClick = false;
				}
			    }

			    //NOTE(denis): tool behaviour
			    if (tileMap.tiles && menuPause > 15)
			    {
				if (currentTool == FILL_TOOL)
				{
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

			    ui_processLetterTyped(theText[0], newTileMapGroup);
			    
			    int tileSize = convertStringToInt(tileSizeEditText.text, tileSizeEditText.letterCount);
			    
			    if (tileSizeEditText.selected)
			    {
			        fillBWithAConverted(1.0F/(float)tileSize, &widthPxEditText, &widthTilesEditText);
				fillBWithAConverted(1.0F/(float)tileSize, &heightPxEditText, &heightTilesEditText);
			    }
			    else if (tileSizeEditText.text[0] != 0)
			    {
				if (widthTilesEditText.text[0] != 0 ||
				    widthPxEditText.text[0] != 0)
				{
				    if (widthTilesEditText.selected)
				    {
					fillBWithAConverted((float)tileSize, &widthTilesEditText, &widthPxEditText);
				    }
				    else if (widthPxEditText.selected)
				    {
				        fillBWithAConverted(1.0F/(float)tileSize, &widthPxEditText, &widthTilesEditText);
				    }
				}
				if (heightTilesEditText.text[0]!=0 ||
				    heightPxEditText.text[0] != 0)
				{
				    if (heightTilesEditText.selected)
				    {
					fillBWithAConverted((float)tileSize, &heightTilesEditText, &heightPxEditText);
				    }
				    else if (heightPxEditText.selected)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, &heightPxEditText, &heightTilesEditText);
				    }
				}
			    }
			} break;

			case SDL_KEYDOWN:
			{
			    if (event.key.keysym.sym == SDLK_BACKSPACE)
			    {
				ui_eraseLetter(newTileMapGroup);

				int tileSize = convertStringToInt(tileSizeEditText.text, tileSizeEditText.letterCount);

				if (tileSize != 0)
				{
				    if (tileSizeEditText.selected)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, &widthPxEditText, &widthTilesEditText);
					fillBWithAConverted(1.0F/(float)tileSize, &heightPxEditText, &heightTilesEditText);
				    }
				    else if (widthTilesEditText.selected)
				    {
					fillBWithAConverted((float)tileSize, &widthTilesEditText, &widthPxEditText);
				    }
				    else if (widthPxEditText.selected)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, &widthPxEditText, &widthTilesEditText);
				    }
				    else if (heightTilesEditText.selected)
				    {
					fillBWithAConverted((float)tileSize, &heightTilesEditText, &heightPxEditText);
				    }
				    else if (heightPxEditText.selected)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, &heightPxEditText, &heightTilesEditText);
				    }
				}
			    }
			} break;
		    }
		}

		SDL_SetRenderDrawColor(renderer, 15, 65, 95, 255);
		SDL_RenderClear(renderer);

		SDL_Color colour = hexColourToRGBA(fileMenuColour);
		SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
		SDL_Rect rect = {0, 0, fileMenuWidth, fileMenuHeight};
		SDL_RenderFillRect(renderer, &rect);
		
		ui_draw();
		
		ui_draw(&saveButton);

		ui_draw(&fileMenu);
		
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

		    //TODO(denis): testing drop down menu
		    ++menuPause;
		    if (menuOpen)
		    {
			dropDown[selectedItem]->setPosition({menuX, menuY});
		    
			int menuPos = 1;
			ui_draw(dropDown[selectedItem]);
		    
			for (int i = 0; i < menuLength; ++i)
			{
			    if (i != selectedItem)
			    {
				int x = dropDown[selectedItem]->pos.x;
				int y = dropDown[selectedItem]->pos.y + dropDown[selectedItem]->pos.h*menuPos;
				dropDown[i]->setPosition({x, y});
				ui_draw(dropDown[i]);
				++menuPos;
			    }
			}
		    }
		    else
		    {
			dropDown[selectedItem]->setPosition({menuX, menuY});
			ui_draw(dropDown[selectedItem]);
		    }
		}
		
		SDL_RenderPresent(renderer);
	    }

	    IMG_Quit();
	    
	    if (tileMap.tiles)
		HeapFree(GetProcessHeap(), 0, tileMap.tiles);
	}
	
	ui_destroy();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
    }

    SDL_Quit();
    return 0;
}
