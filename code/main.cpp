/* TODO(denis):
 * in the beginning, check the resolution of the user's monitor, and don't allow them
 * to make tile maps that would be too big!
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
 *
 * CTRL + N -> create new tile map
 * CTRL + O -> open tile map
 * CTRL + S -> save current tile map
 * etc...
 */

//IMPORTANT(denis): note to self, design everything expecting at least a 1280 x 720
// window

#include <SDL.h>
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "windows.h"
#include <math.h>

#include "main.h"
#include "file_saving_loading.h"
#include "denis_math.h"
#include "TEMP_GeneralFunctions.cpp"
#include "new_tile_map_panel.h"

#include "ui_elements.h"

#define TITLE "Tile Map Editor"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

struct TileSet
{
    char *name;
    SDL_Rect pos;
    SDL_Texture *image;
    uint32 tileSize;
};

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

static void reorientEditingArea(int windowWidth, int windowHeight, TileMap *tileMap,
				int *menuX, int *menuY)
{   
    int widthSpace = windowWidth - tileMap->tileSize*tileMap->widthInTiles;
    int heightSpace = windowHeight - tileMap->tileSize*tileMap->heightInTiles;

    int padding = 30;
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

static TileMap createNewTileMap(int startX, int startY)
{
    TileMap newTileMap = {};
    newTileMap.offset.x = startX;
    newTileMap.offset.y = startY;
    
    //TODO(denis): maybe this should move out?
    NewTileMapPanelData* data = newTileMapPanelGetData();
    newTileMapPanelSetVisible(false);

    newTileMap.widthInTiles = data->widthInTiles;
    newTileMap.heightInTiles = data->heightInTiles;
    newTileMap.tileSize = data->tileSize;
				    
    int memorySize = sizeof(Tile)*newTileMap.widthInTiles*newTileMap.heightInTiles;
    
    HANDLE heapHandle = GetProcessHeap();
    newTileMap.tiles = (Tile*) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, memorySize);

    if (newTileMap.tiles)
    {	
	Tile *row = newTileMap.tiles;
	for (int i = 0; i < newTileMap.heightInTiles; ++i)
	{
	    Tile *element = row;
	    for (int j = 0; j < newTileMap.widthInTiles; ++j)
	    {
		SDL_Rect rect;
						
		rect.h = newTileMap.tileSize;
		rect.w = newTileMap.tileSize;
		rect.x = newTileMap.offset.x + j*newTileMap.tileSize;
		rect.y = newTileMap.offset.y + i*newTileMap.tileSize;
		
		element->pos = rect;
		
		++element;
	    }
	    row += newTileMap.widthInTiles;
	}
    }

    return newTileMap;
}

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_Window *window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED,
					  SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
					  WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(window, WINDOW_WIDTH, WINDOW_HEIGHT);
    
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
	    
	    Button saveButton = {};
	    
	    char *tileMapName = NULL;

	    TileMap tileMap = {};
	    tileMap.offset = {5, 5};

	    bool selectionVisible = false;
	    Vector2 startSelectPos = {};
	    TexturedRect selectionBox = {};
	    TexturedRect selectedTile = {};

	    //NOTE(denis): setting up the top bar
	    ui_setFont(menuFontName, menuFontSize);

	    MenuBar topMenuBar = ui_createMenuBar(0, 0, WINDOW_WIDTH, 20,
						  0xFFCCCCCC, 0xFF000000);
	    
	    char *items[] = {"File", "Open Tile Map...", "Save Tile Map...",
			     "Import Tile Sheet...", "Exit"};	     
	    topMenuBar.addMenu(items, 5, 225);
	    items[0] = "Tile Maps";
	    items[1] = "Create New";
	    items[2] = "NOT FINAL";
	    topMenuBar.addMenu(items, 3, 225);
	    
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
	    
	    {
		createNewTileMapPanel(0, 0, 0, 0);
		int centreX = WINDOW_WIDTH/2 - newTileMapPanelGetWidth()/2;
		int centreY = WINDOW_HEIGHT/2 - newTileMapPanelGetHeight()/2;
		newTileMapPanelSetPosition({centreX, centreY});
		newTileMapPanelSetVisible(false);
	    }

	    //NOTE(denis): this is for the pop up window that appears when you
	    // click "import tile sheet"
	    UIPanel importTileSetPanel = {};
	    {
		int width = 950;
		int height = 150;
		int x = WINDOW_WIDTH/2 - width/2;
		int y = WINDOW_HEIGHT/2 - height/2;
		uint32 colour = 0xFF222222;
		importTileSetPanel = ui_createPanel(x, y, width, height, colour);
		importTileSetPanel.visible = false;
	    }
	    TexturedRect tileSizeText =
		ui_createTextField("Tile Size:", importTileSetPanel.panel.pos.x + 15,
				   importTileSetPanel.panel.pos.y + 15, COLOUR_WHITE);
	    ui_addToPanel(&tileSizeText, &importTileSetPanel);

	    EditText tileSizeEditText =
		ui_createEditText(tileSizeText.pos.x + tileSizeText.getWidth() + 15,
				  tileSizeText.pos.y, 100, tileSizeText.getHeight(),
				  COLOUR_WHITE, 5);
	    char chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0 };
	    tileSizeEditText.allowedCharacters = chars;
	    ui_addToPanel(&tileSizeEditText, &importTileSetPanel);

	    TexturedRect tileSheetText =
		ui_createTextField("Tile Sheet:", tileSizeText.pos.x,
				   tileSizeText.pos.y+tileSizeText.pos.h + 5,
				   COLOUR_WHITE);
	    ui_addToPanel(&tileSheetText, &importTileSetPanel);

	    EditText tileSheetEditText =
		ui_createEditText(tileSheetText.pos.x + tileSheetText.getWidth() + 15,
				  tileSheetText.pos.y, importTileSetPanel.getWidth() - tileSheetText.getWidth() - 45,
				  tileSheetText.getHeight(),
				  COLOUR_WHITE, 5);
	    ui_addToPanel(&tileSheetEditText, &importTileSetPanel);

	    Button browseTileSetButton =
		ui_createTextButton("Browse", COLOUR_WHITE, 0, 0, 0xFF000000);
	    {
		int x = tileSheetEditText.pos.x + tileSheetEditText.getWidth()/2 - browseTileSetButton.getWidth()/2;
		int y = tileSheetEditText.pos.y + browseTileSetButton.getHeight() + 5;
		browseTileSetButton.setPosition({x,y});
	    }
	    ui_addToPanel(&browseTileSetButton, &importTileSetPanel);

	    Button cancelButton = ui_createTextButton("Cancel", COLOUR_WHITE, 0, 0, 0xFF000000);
	    Button openButton = ui_createTextButton("Open", COLOUR_WHITE, 0, 0, 0xFF000000);
	    {
		int x = importTileSetPanel.panel.pos.x + 15;
		int y = browseTileSetButton.background.pos.y + 15;
		cancelButton.setPosition({x,y});
		x += cancelButton.getWidth() + 15;
		openButton.setPosition({x,y});
	    }
	    ui_addToPanel(&cancelButton, &importTileSetPanel);
	    ui_addToPanel(&openButton, &importTileSetPanel);

	    TexturedRect warningText = {};
	    ui_addToPanel(&warningText, &importTileSetPanel);


	    TileSet currentTileSet = {};
	    
	    
	    UIPanel tileSetPanel = {};
	    {
		int width = WINDOW_WIDTH/3;
		int height = WINDOW_HEIGHT - 
		    (topMenuBar.botRight.y - topMenuBar.topLeft.y) - 30;
		int x = WINDOW_WIDTH - width - 15;
		int y = topMenuBar.botRight.y + 15;
		Uint32 colour = 0xFF00BB55;
	        tileSetPanel = ui_createPanel(x, y, width, height, colour);
	    }
	    DropDownMenu tileSetDropDown = {};
	    {
		char *items[] = {"No Tile Sheet Selected",
				 "Import a new tile sheet .."};
		int num = 2;
		int width = tileSetPanel.getWidth()-30;
		int height = 40;
		uint32 backgroundColour = 0xFFEEEEEE;
		tileSetDropDown =
		    ui_createDropDownMenu(items, num, width, height, COLOUR_BLACK,
					  backgroundColour);

		Vector2 newPos = {WINDOW_WIDTH - WINDOW_WIDTH/3,
				  topMenuBar.getHeight() + 30};
		tileSetDropDown.setPosition(newPos);
	    }
	    ui_addToPanel(&tileSetDropDown, &tileSetPanel);
		
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

			        topMenuBar.botRight.x =
				    topMenuBar.topLeft.x + windowWidth;
				
				if (tileMap.tiles)
				{
				    reorientEditingArea(windowWidth, windowHeight, &tileMap,
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

				//TODO(denis): might want to resize the new tile map
				// panel here
				if (newTileMapPanelVisible())
				{
				    Vector2 newPos = {};
				    newPos.x = windowWidth/2 - newTileMapPanelGetWidth()/2;
				    newPos.y = windowHeight/2 - newTileMapPanelGetHeight()/2;
				    newTileMapPanelSetPosition(newPos);
				}
			    }
			    
			} break;

			case SDL_MOUSEMOTION:
			{
			    //TODO(denis): also handle when the user has focus on our
			    // window but has moved off of it

			    Vector2 mouse = {event.motion.x, event.motion.y};
			    
			    topMenuBar.onMouseMove(mouse);


			    if (tileSetPanel.visible && tileSetDropDown.isOpen)
			    {
				if (pointInRect(mouse, tileSetDropDown.getRect()))
				{
				    int highlighted =
					(mouse.y - tileSetDropDown.getRect().y) / tileSetDropDown.items[0].pos.h;
				    tileSetDropDown.highlightedItem = highlighted;
				}
			    }
			    
			    if (currentTool == PAINT_TOOL && tileMap.tiles)
			    {
#if 0
				selectionVisible = moveSelectionBox(&selectionBox, mouse, &tileMap, &currentTileSet.background);
#endif
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
#if 0
				    selectionVisible = moveSelectionBox(&selectionBox, mouse, &tileMap, &currentTileSet.background);
#endif
				}
			    }
			} break;

			case SDL_MOUSEBUTTONDOWN:
			{
			    Vector2 mouse = {event.button.x, event.button.y};
			    uint8 mouseButton = event.button.button;
			    
			    newTileMapPanelRespondToMouseDown(mouse, mouseButton);

			    //TODO(denis): testing tile set panel
			    tileSetDropDown.startedClick =
				pointInRect(mouse, tileSetDropDown.getRect());
			    
			    //TODO(denis): replace this with the ui call
			    //TODO(denis): only do this if the top bar isn't clicked,
			    // etc..
			    fillToolIcon.startedClick = pointInRect(mouse, fillToolIcon.background.pos);
			    paintToolIcon.startedClick = pointInRect(mouse, paintToolIcon.background.pos);
#if 0
			    currentTileSet.startedClick = pointInRect(mouse, currentTileSet.background.pos);
#endif
			    saveButton.startedClick = pointInRect(mouse, saveButton.background.pos);

			    topMenuBar.onMouseDown(mouse, event.button.button);

			    if (importTileSetPanel.visible)
				ui_processMouseDown(&importTileSetPanel, mouse, mouseButton);
			    
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
			    uint8 mouseButton = event.button.button;
			    
			    //TODO(denis): have a heirarchy of parts that detect mouse
			    // clicks, if one ui element responds to a click, the
			    // rest of the ui elements should be ignored

			    if (importTileSetPanel.visible)
			    {
				ui_processMouseUp(&importTileSetPanel, mouse, mouseButton);

				if (ui_wasClicked(cancelButton, mouse))
				{
				    importTileSetPanel.visible = false;
				    //TODO(denis): reset import tile set panel values
				    // maybe save the value in the tile size field?
				}
				else if (ui_wasClicked(openButton, mouse))
				{
				    int tileSize =
					convertStringToInt(tileSizeEditText.text,
							   tileSizeEditText.letterCount);

				    char *warning = "";
				    if (tileSize == 0)
				    {
					warning = "tile size must be non-zero";
				    }
				    else if (tileSheetEditText.letterCount == 0)
				    {
					warning = "no tile sheet file selected";
				    }
				    else
				    {
					TexturedRect newTileSheet = loadImage(renderer, tileSheetEditText.text);

					if (newTileSheet.image != 0)
					{
					    char *fileName = tileSheetEditText.text;
					    int startOfFileName = 0;
					    int endOfFileName = 0;
					    for (int i = 0; fileName[i] != 0; ++i)
					    {
						if (fileName[i] == '\\' || fileName[i] == '/')
						    startOfFileName = i+1;

						if (fileName[i+1] == 0)
						    endOfFileName = i+1;
					    }

					    char* fileNameTruncated =
						(char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (endOfFileName-startOfFileName+1)*sizeof(char));
					    for (int i = 0; i < endOfFileName-startOfFileName; ++i)
					    {
						fileNameTruncated[i] = fileName[i+startOfFileName];
					    }
					    fileNameTruncated[endOfFileName-startOfFileName] = 0;

					    //TODO(denis): currentTileSet.name has to be freed if ever
					    // the tileset is deleted
					    currentTileSet.name = fileNameTruncated;
					    currentTileSet.pos = newTileSheet.pos;
					    currentTileSet.image = newTileSheet.image;
					    currentTileSet.tileSize = tileSize;

					    //TODO(denis): don't do this when I have multiple tile sets
					    // don't have the ".png" part?
					    tileSetDropDown.changeItem(fileNameTruncated, 0);
					    tileSetDropDown.setPosition({tileSetDropDown.getRect().x, tileSetDropDown.getRect().y});
					    
					    importTileSetPanel.visible = false;
					}
					else
					{
					    warning = "Error opening file";
					}
				    }
				    warningText = ui_createTextField(warning, cancelButton.background.pos.x,
								     cancelButton.background.pos.y + cancelButton.getHeight() + 15, COLOUR_RED);
				}
				else if (ui_wasClicked(browseTileSetButton, mouse))
				{
				    char *fileName = getTileSheetFileName();

				    if (fileName != 0)
				    {
					while(tileSheetEditText.letterCount > 0)
					    ui_eraseLetter(&tileSheetEditText);

					int i = 0;
					for (i = 0; fileName[i] != 0; ++i)
					{
					    ui_addLetterTo(&tileSheetEditText, fileName[i]);
					}
				    }

				    HeapFree(GetProcessHeap(), 0, fileName);
				}
			    }
			    else if (topMenuBar.onMouseUp(mouse, event.button.button))
			    {
				//TODO(denis): top bar was clicked
			    }
			    else if (newTileMapPanelVisible())
			    {
				newTileMapPanelRespondToMouseUp(mouse, mouseButton);
			    }

			    if (tileSetPanel.visible)
			    {
				if (tileSetDropDown.isOpen)
				{
				    if (pointInRect(mouse, tileSetDropDown.getRect()))
				    {
					int selection = (mouse.y - tileSetDropDown.getRect().y)/tileSetDropDown.items[0].pos.h;
					if (selection == 1)
					{
					    importTileSetPanel.visible = true;
					}
				    }

				    tileSetDropDown.highlightedItem = -1;
				    tileSetDropDown.isOpen = false;
				}
				else if (pointInRect(mouse, tileSetDropDown.getRect())
					 && tileSetDropDown.startedClick)
				{
				    tileSetDropDown.isOpen = true;
				    tileSetDropDown.highlightedItem = 0;
				}
				    
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

#if 0
			    if (tileMap.tiles && ui_wasClicked(currentTileSet, mouse))
			    {
				selectedTile.pos.x = (selectionBox.pos.x - currentTileSet.background.pos.x)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.y = (selectionBox.pos.y - currentTileSet.background.pos.y)/tileMap.tileSize * tileMap.tileSize;
				selectedTile.pos.w = tileMap.tileSize;
				selectedTile.pos.h = tileMap.tileSize;
				
				selectedTile.image = currentTileSet.background.image;

				currentTileSet.startedClick = false;
			    }
#endif
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
#if 0
				    selectionVisible = moveSelectionBox(&selectionBox, mouse,
									&tileMap, &currentTileSet.background);
#endif
				    selectionBox.pos.w = tileMap.tileSize;
				    selectionBox.pos.h = tileMap.tileSize;
				    
				}
			    }
			} break;

			case SDL_TEXTINPUT:
			{
			    char* theText = event.text.text;

			    if (newTileMapPanelVisible())
				newTileMapPanelCharInput(theText[0]);
			    else if(importTileSetPanel.visible)
			    {
				ui_processLetterTyped(theText[0], &importTileSetPanel);
			    }
			} break;

			case SDL_KEYDOWN:
			{
			    if (event.key.keysym.sym == SDLK_BACKSPACE)
			    {
				if (newTileMapPanelVisible())
				    newTileMapPanelCharDeleted();
				else if (importTileSetPanel.visible)
				{
				    ui_eraseLetter(&importTileSetPanel);
				}   
			    }
			} break;

			case SDL_KEYUP:
			{
			    if (event.key.keysym.sym == SDLK_TAB)
			    {
				if (newTileMapPanelVisible())
				{
				    SDL_Keymod mod = SDL_GetModState();

				    if ((mod & KMOD_LSHIFT) || (mod & KMOD_RSHIFT))
					newTileMapPanelSelectPrevious();
				    else
					newTileMapPanelSelectNext();
				}
			    }
			    else if (event.key.keysym.sym == SDLK_RETURN ||
				     event.key.keysym.sym == SDLK_KP_ENTER)
			    {
				if (newTileMapPanelVisible())
				    newTileMapPanelEnterPressed();
			    }
			    
			} break;
		    }
		}

		SDL_SetRenderDrawColor(renderer, 15, 65, 95, 255);
		SDL_RenderClear(renderer);

		newTileMapPanelDraw();
		
		ui_draw(&saveButton);

		if (newTileMapPanelVisible())
		{
		    if (newTileMapPanelDataReady())
		    {
			/*
			  int windowHeight = 0;
			  int windowWidth = 0;
			  SDL_GetWindowSize(window, &windowWidth, &windowHeight);

			  reorientEditingArea(windowWidth, windowHeight, &tileMap,
			  &menuX, &menuY);
			*/
			
			tileMap = createNewTileMap(50, 100);
			
			//TODO(denis): this should only be done once
			// preferably at the beginning
			int textBoxWidth = tileMap.widthInTiles*tileMap.tileSize - 20;
			item1 = ui_createTextBox("first tile map", textBoxWidth, 35, COLOUR_BLACK,
						 0xFFFFFFFF);
			item2 = ui_createTextBox("second tile map", textBoxWidth, 35, COLOUR_BLACK,
						 0xFFFFFFFF);
			item3 = ui_createTextBox("third tile map", textBoxWidth, 35, COLOUR_BLACK,
						 0xFFFFFFFF);
			item1.setPosition({menuX, menuY});

			selectionBox = createFilledTexturedRect(
			    renderer, tileMap.tileSize, tileMap.tileSize, 0x77FFFFFF);

			saveButton = ui_createTextButton("Save", COLOUR_WHITE,
							 100, 50, 0xFF33AA88);
		    }
		}
		
		if (tileSetPanel.visible)
		{
		    ui_draw(&tileSetPanel);
		    
		    if (currentTileSet.image != 0)
		    {
			int tilesPerRow = (tileSetPanel.getWidth() - 30)/currentTileSet.tileSize;

			int tilesPadding = (tileSetPanel.getWidth() - tilesPerRow*currentTileSet.tileSize)/2;
			int tileSetStartX = tileSetPanel.panel.pos.x + tilesPadding;
			int tileSetStartY = tileSetDropDown.items[0].pos.y + tileSetDropDown.items[0].pos.h + 15;

			//TODO(denis): bundle these up into a Tile struct
			SDL_Rect currentTileInSheet =
			    {0, 0, currentTileSet.tileSize, currentTileSet.tileSize};
			SDL_Rect currentTileOnScreen =
			    {tileSetStartX, tileSetStartY,
			     currentTileSet.tileSize, currentTileSet.tileSize};

			int numTilesY = currentTileSet.pos.h/currentTileSet.tileSize;
			int numTilesX = currentTileSet.pos.w/currentTileSet.tileSize;
			    
			for (int i = 0; i < numTilesY; ++i)
			{
			    for (int j = 0; j < numTilesX; ++j)
			    {
				//TODO(denis): don't change the position of the next
				// drawn tile if the tile sheet position is blank
			    
				currentTileInSheet.x = j * currentTileSet.tileSize;
				currentTileInSheet.y = i * currentTileSet.tileSize;

				currentTileOnScreen.x = tileSetStartX + ((i*numTilesX + j) % tilesPerRow)*currentTileSet.tileSize;
			        currentTileOnScreen.y = tileSetStartY + ((i*numTilesX + j)/tilesPerRow)*currentTileSet.tileSize;
				
				SDL_RenderCopy(renderer, currentTileSet.image, &currentTileInSheet, &currentTileOnScreen);
			    }
			}
		    }

		    //TODO(denis): bad fix for the drawing order problem
		    ui_draw(&tileSetDropDown);
		}

		SDL_RenderCopy(renderer, paintToolIcon.background.image, NULL, &paintToolIcon.background.pos);
		SDL_RenderCopy(renderer, fillToolIcon.background.image, NULL, &fillToolIcon.background.pos);
		
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
#if 0
			    else
				SDL_RenderCopy(renderer, currentTileSet.background.image, &element->sheetPos, &element->pos);
#endif
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

		ui_draw(&topMenuBar);

		ui_draw(&importTileSetPanel);
		
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
