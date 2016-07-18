/* TODO(denis):
 * in the beginning, check the resolution of the user's monitor, and don't allow them
 * to make tile maps that would be too big!
 * SDL_GetDisplayUsableBounds
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
 *
 * DISABLE top menu bar when popup is visible
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
#include "tile_set_panel.h"

#include "ui_elements.h"

#define TITLE "Tile Map Editor"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


static Vector2 convertScreenPosToTilePos(TileMap *tileMap, Vector2 pos)
{
    Vector2 result = {};

    result = (pos - tileMap->offset)/tileMap->tileSize;
     
    return result;
}

static TileMap createNewTileMap(int startX, int startY)
{
    TileMap newTileMap = {};
    newTileMap.offset.x = startX;
    newTileMap.offset.y = startY;
    
    NewTileMapPanelData* data = newTileMapPanelGetData();
    
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
	    
	    char *tileMapName = NULL;

	    TileMap tileMap = {};
	    tileMap.offset = {5, 5};

	    bool selectionVisible = false;
	    Vector2 startSelectPos = {};
	    TexturedRect selectionBox = {};

	    //NOTE(denis): setting up the top bar
	    ui_setFont(menuFontName, menuFontSize);

	    MenuBar topMenuBar = ui_createMenuBar(0, 0, WINDOW_WIDTH, 20,
						  0xFFCCCCCC, 0xFF000000);
	    
	    char *items[] = {"File", "Create New Tile Map", "Open Tile Map...",
			     "Save Tile Map...", "Import Tile Sheet...", "Exit"};	     
	    topMenuBar.addMenu(items, 6, 225);
	    
	    items[0] = "Tile Maps";
	    items[1] = "Create New";
	    items[2] = "NOT FINAL";
	    topMenuBar.addMenu(items, 3, 225);
	    
	    //NOTE(denis): create new tile map panel
	    ui_setFont(defaultFontName, defaultFontSize);
	    
	    {
		createNewTileMapPanel(0, 0, 0, 0);
		int centreX = WINDOW_WIDTH/2 - newTileMapPanelGetWidth()/2;
		int centreY = WINDOW_HEIGHT/2 - newTileMapPanelGetHeight()/2;
		newTileMapPanelSetPosition({centreX, centreY});
		newTileMapPanelSetVisible(false);
	    }

	    //NOTE(denis): import tile sheet panel
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

	    //NOTE(denis): tile set panel
	    {
		int width = WINDOW_WIDTH/3;
		int height = WINDOW_HEIGHT - 
		    (topMenuBar.botRight.y - topMenuBar.topLeft.y) - 30;
		int x = WINDOW_WIDTH - width - 15;
		int y = topMenuBar.botRight.y + 15;

		tileSetPanelCreateNew(renderer, x, y, width, height);
	    }
	    //NOTE(denis): tile map panel
	    UIPanel tileMapPanel = {};
	    {
		int x = 15;
		int y = topMenuBar.botRight.y + 15;
		int width = 800;
		int height = WINDOW_HEIGHT - y - 15;
		uint32 colour = 0xFFEE2288;
	        tileMapPanel = ui_createPanel(x, y, width, height, colour);
	    }

	    Button createNewTileMapButton =
		ui_createTextButton("Create New Tile Map", COLOUR_WHITE, 0, 0,
				    0xFFFF0000);
	    {
		int x = tileMapPanel.panel.pos.x + tileMapPanel.getWidth()/2 -
		    createNewTileMapButton.getWidth()/2;
		int y = tileMapPanel.panel.pos.y + tileMapPanel.getHeight()/2 -
		    createNewTileMapButton.getHeight()/2;
		createNewTileMapButton.setPosition({x, y});
	    }
	    
	    Button saveButton = {};
	    
	    const enum ToolType{ PAINT_TOOL,
		   FILL_TOOL
	    };
	    ToolType currentTool = PAINT_TOOL;
	    
	    Button paintToolIcon = ui_createImageButton("paint-brush-icon-32.png");
	    Button fillToolIcon = ui_createImageButton("paint-can-icon-32.png");

	    {
		int x = tileMapPanel.panel.pos.x + tileMapPanel.getWidth() - paintToolIcon.getWidth() - 15;
		int y = tileMapPanel.panel.pos.y + 15;
		paintToolIcon.setPosition({x, y});
		fillToolIcon.setPosition({x, y+paintToolIcon.getHeight() + 5});
	    }
	    ui_addToPanel(&paintToolIcon, &tileMapPanel);
	    ui_addToPanel(&fillToolIcon, &tileMapPanel);
	    
	    
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

				//TODO(denis): resize all the panels here

				
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

			    if (tileSetPanelVisible())
			    {
				tileSetPanelOnMouseMove(mouse);
			    }
			    
			    if (currentTool == PAINT_TOOL && tileMapPanel.visible &&
				tileMap.tiles)
			    {
				if (selectionBox.pos.w != 0 && selectionBox.pos.h != 0)
				{
				    selectionVisible = moveSelectionInRect(&selectionBox, mouse,
									   tileMap.getRect(), tileMap.tileSize);
				}
				
				if (event.motion.state & SDL_BUTTON_LMASK)
				{
				    if (pointInRect(mouse, tileMap.getRect()))
				    {
					Vector2 tile =
					    (mouse - tileMap.offset)/tileMap.tileSize;

					Tile *clicked = tileMap.tiles + tile.x + tile.y*tileMap.widthInTiles;
					clicked->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
				    }
				}
			    }
			    else if (currentTool == FILL_TOOL && tileMapPanel.visible &&
				     tileMap.tiles)
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
				    selectionVisible = moveSelectionInRect(&selectionBox, mouse, tileMap.getRect(), tileMap.tileSize);
				}
			    }
			} break;

			case SDL_MOUSEBUTTONDOWN:
			{
			    Vector2 mouse = {event.button.x, event.button.y};
			    uint8 mouseButton = event.button.button;

			    if (importTileSetPanel.visible)
			    {
				ui_processMouseDown(&importTileSetPanel, mouse, mouseButton);
			    }
			    else if (newTileMapPanelVisible())
			    {
				newTileMapPanelOnMouseDown(mouse, mouseButton);
			    }
			    else if (tileSetPanelVisible() || tileMapPanel.visible)
			    {
				if (tileSetPanelVisible())
				{
				    tileSetPanelOnMouseDown(mouse, mouseButton);
				}

				if (tileMapPanel.visible)
				{
				    ui_processMouseDown(&tileMapPanel, mouse, mouseButton);
				    
				    createNewTileMapButton.startedClick =
					pointInRect(mouse, createNewTileMapButton.background.pos);

				    if (currentTool == PAINT_TOOL && tileMap.tiles)
				    {
					if (event.button.button == SDL_BUTTON_LEFT)
					{			    
					    if (pointInRect(mouse, tileMap.getRect()))
					    {
						Vector2 tilePos =
						    convertScreenPosToTilePos(&tileMap, mouse);

						Tile *clicked = tileMap.tiles + tilePos.x + tilePos.y*tileMap.widthInTiles;
						clicked->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
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
				}
			    }
			    
			    saveButton.startedClick = pointInRect(mouse, saveButton.background.pos);
			    
			    topMenuBar.onMouseDown(mouse, event.button.button);
			    
			} break;
			
			case SDL_MOUSEBUTTONUP:
			{
			    Vector2 mouse = {event.button.x, event.button.y};
			    uint8 mouseButton = event.button.button;
			    
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
				    int tileSize;
				    if (tileMap.tileSize != 0)
				    {
					tileSize = tileMap.tileSize;
				    }
				    else
				    {
					tileSize = convertStringToInt(tileSizeEditText.text,
								      tileSizeEditText.letterCount);
				    }

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
					    //TODO(denis): don't have the ".png" part?
					    fileNameTruncated[endOfFileName-startOfFileName] = 0;

					    tileSetPanelInitializeNewTileSet(fileNameTruncated, newTileSheet.pos, newTileSheet.image,
								    tileSize);
					    
					    importTileSetPanel.visible = false;

					    if (selectionBox.pos.w == 0 && selectionBox.pos.h == 0)
					    {
						initializeSelectionBox(renderer,
								       &selectionBox,
								       tileSize);
					    }
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
			    else if (newTileMapPanelVisible())
			    {
				newTileMapPanelOnMouseUp(mouse, mouseButton);
			    }
			    else if (topMenuBar.onMouseUp(mouse, event.button.button))
			    {
				if (pointInRect(mouse, topMenuBar.menus[0].getRect()))
				{
				    //NOTE(denis): clicked on the file menu
				    int selectionY = (mouse.y - topMenuBar.menus[0].getRect().y)/topMenuBar.menus[0].items[0].pos.h;
				    if (selectionY == 1)
				    {
					//NOTE(denis): 1 == "create new tile map"
					newTileMapPanelSetVisible(true);
					topMenuBar.menus[0].isOpen = false;
				    }
				    else if (selectionY == 4)
				    {
					//NOTE(denis): 4 = "import tile sheet"
					importTileSetPanel.visible = true;
					topMenuBar.menus[0].isOpen = false;
				    }
				}
			    }
			    else if (tileSetPanelVisible() || tileMapPanel.visible)
			    {
				if (tileSetPanelVisible())
				{
				    tileSetPanelOnMouseUp(mouse, mouseButton);

				    if (tileSetPanelImportTileSetPressed())
					importTileSetPanel.visible = true;
				}
			    
				//TODO(denis): have something like "!click used" or
				// something
				if (tileMapPanel.visible)
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
				    else if (ui_wasClicked(createNewTileMapButton, mouse))
				    {
					newTileMapPanelSetVisible(true);
					createNewTileMapButton.startedClick = false;
				    }

				    //NOTE(denis): tool behaviour
				    if (tileMap.tiles)
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
							(tileMap.tiles + i*tileMap.widthInTiles + j)->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
						    }
						}
					    }
					    selectionVisible =
						moveSelectionInRect(&selectionBox, mouse, tileMap.getRect(),
								    tileMap.tileSize);

					    selectionBox.pos.w = tileMap.tileSize;
					    selectionBox.pos.h = tileMap.tileSize;
					}
				    }
				}
			    }			
#if 0			    
			    //NOTE(denis): hit the save button
			    if (tileMap.tiles && saveButton.startedClick)
			    {
				saveTileMapToFile(&tileMap, tileMapName);
				
				saveButton.startedClick = false;
			    }
#endif
			} break;

			case SDL_TEXTINPUT:
			{
			    char* theText = event.text.text;

			    if (newTileMapPanelVisible())
			    {
				newTileMapPanelCharInput(theText[0]);
			    }
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
				{
				    newTileMapPanelCharDeleted();
				}
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

		if (newTileMapPanelVisible())
		{
		    //TODO(denis): if the new tile map panel "tile size" field
		    // doesn't have anything in it, make the default the same as the
		    // current tile set's tile size
		    
		    if (newTileMapPanelDataReady())
		    {
			int x = tileMapPanel.panel.pos.x + 15;
			int y = tileMapPanel.panel.pos.y + 15;
			tileMap = createNewTileMap(x, y);

			newTileMapPanelSetVisible(false);

			if (selectionBox.pos.w == 0 && selectionBox.pos.h == 0)
			{
			    initializeSelectionBox(renderer,
						   &selectionBox, tileMap.tileSize);
			}
		    }
		}

		if (tileSetPanelVisible())
		{
		    tileSetPanelDraw();
		}		

		if (tileMapPanel.visible)
		{
		    ui_draw(&tileMapPanel);

		    if (!tileMap.tiles)
		    {
			ui_draw(&createNewTileMapButton);
		    }
		    else
		    {
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
					SDL_RenderCopy(renderer, tileSetPanelGetCurrentTileSet(), &element->sheetPos, &element->pos);
				    ++element;
				}

				row += tileMap.widthInTiles;
			    }

			    SDL_DestroyTexture(defaultTile.image);
			}	
		    }
		}

		if (selectionVisible)
		    SDL_RenderCopy(renderer, selectionBox.image, NULL, &selectionBox.pos);

		newTileMapPanelDraw();

		//TODO(denis): if there is a created tile map, have the tile size
		// field default with that value if it doesn't already have something
		// in it
		ui_draw(&importTileSetPanel);
		
		ui_draw(&topMenuBar);
		
		SDL_RenderPresent(renderer);
	    }

	    IMG_Quit();
	    
	    if (tileMap.tiles)
		HeapFree(GetProcessHeap(), 0, tileMap.tiles);
	}
	
	ui_destroy();
	if (renderer)
	    SDL_DestroyRenderer(renderer);
	if (window)
	    SDL_DestroyWindow(window);
    }

    SDL_Quit();
    return 0;
}
