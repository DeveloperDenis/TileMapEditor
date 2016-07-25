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
#define BACKGROUND_COLOUR 60,67,69,255


static Vector2 convertScreenPosToTilePos(TileMap *tileMap, Vector2 pos)
{
    Vector2 result = {};
    
    result = (pos - tileMap->offset)/tileMap->tileSize;
    
    return result;
}

static TileMap createNewTileMap(int startX, int startY,
				int32 *outWidth, int32 *outHeight)
{
    TileMap newTileMap = {};
    newTileMap.offset.x = startX;
    newTileMap.offset.y = startY;
    
    NewTileMapPanelData* data = newTileMapPanelGetData();
    
    newTileMap.widthInTiles = data->widthInTiles;
    newTileMap.heightInTiles = data->heightInTiles;
    newTileMap.tileSize = data->tileSize;

    *outWidth = newTileMap.widthInTiles*newTileMap.tileSize;
    *outHeight = newTileMap.heightInTiles*newTileMap.tileSize;
    
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
		element->sheetPos.w = element->sheetPos.h = newTileMap.tileSize;
		
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

	    TexturedRect defaultTile = loadImage(renderer, "default_tile.png");
	    TileMap tileMap = {};
	    tileMap.offset = {5, 5};

	    bool selectionVisible = false;
	    Vector2 startSelectPos = {};
	    TexturedRect selectionBox = {};

	    //NOTE(denis): setting up the top bar
	    ui_setFont(menuFontName, menuFontSize);

	    MenuBar topMenuBar = ui_createMenuBar(0, 0, WINDOW_WIDTH, 20,
						  0xFF667378, 0xFF000000);
	    
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
	    SDL_Rect tileMapArea = {};
	    SDL_Rect tileMapVisibleArea = {};
	    
	    UIPanel tileMapPanel = {};
	    {
		int x = 15;
		int y = topMenuBar.botRight.y + 15;
		int width = 800;
		int height = WINDOW_HEIGHT - y - 15;
		uint32 colour = 0xFFEE2288;
	        tileMapPanel = ui_createPanel(x, y, width, height, colour);

		tileMapArea.x = x + 15;
		tileMapArea.y = y + 15;
		tileMapArea.h = height - 30;
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

		tileMapArea.w = x - tileMapArea.x - 15;
	    }
	    ui_addToPanel(&paintToolIcon, &tileMapPanel);
	    ui_addToPanel(&fillToolIcon, &tileMapPanel);

	    //TODO(denis): testing scroll bar
	    TexturedRect backgroundBar = {};
	    TexturedRect scrollingBar = {};
	    int32 scrollClickDelta = 0;
	    bool mouseDownScroll = false;
	    Vector2 tileMapDrawPosOffset = {};
	    
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

			    if (mouseDownScroll)
			    {
				scrollingBar.pos.x = mouse.x - scrollClickDelta;
				if (scrollingBar.pos.x < backgroundBar.pos.x)
				    scrollingBar.pos.x = backgroundBar.pos.x;
				else if (scrollingBar.pos.x > backgroundBar.pos.x + backgroundBar.pos.w - scrollingBar.pos.w)
				    scrollingBar.pos.x = backgroundBar.pos.x + backgroundBar.pos.w - scrollingBar.pos.w;

				real32 percentMoved = (real32)(scrollingBar.pos.x-backgroundBar.pos.x)/(real32)(backgroundBar.pos.w - scrollingBar.pos.w);
				tileMapDrawPosOffset.x = (int32)((tileMap.widthInTiles*tileMap.tileSize - tileMapVisibleArea.w)*percentMoved);
			    }
			    else if (currentTool == PAINT_TOOL && tileMapPanel.visible &&
				tileMap.tiles)
			    {
				if (selectionBox.pos.w != 0 && selectionBox.pos.h != 0)
				{
				    selectionVisible = moveSelectionInRect(&selectionBox, mouse,
									   tileMapVisibleArea, tileMap.tileSize);
				}
				
				if (event.motion.state & SDL_BUTTON_LMASK)
				{
				    if (pointInRect(mouse, tileMap.getRect()))
				    {
					Vector2 tile =
					    (mouse - tileMap.offset)/tileMap.tileSize;

					Tile *clicked = tileMap.tiles + tile.x + tile.y*tileMap.widthInTiles;

					if (tileSetPanelGetSelectedTile().sheetPos.w != 0 &&
					    tileSetPanelGetSelectedTile().sheetPos.h != 0)
					{
					    clicked->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
					}
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
				    selectionVisible = moveSelectionInRect(&selectionBox, mouse, tileMapVisibleArea, tileMap.tileSize);
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
				    if (pointInRect(mouse, scrollingBar.pos))
				    {
					mouseDownScroll = true;
					scrollClickDelta = mouse.x - scrollingBar.pos.x;
				    }
				    
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

						if (tileSetPanelGetSelectedTile().sheetPos.w != 0 &&
						    tileSetPanelGetSelectedTile().sheetPos.h != 0)
						{
						    clicked->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
						}
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

			    mouseDownScroll = false;
			    
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
				//TODO(denis): need better solution
				tileSetPanelOnMouseUp({0,0}, mouseButton);
				
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
							if (tileSetPanelGetSelectedTile().sheetPos.w != 0)
							{
							    (tileMap.tiles + i*tileMap.widthInTiles + j)->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
							}
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

		SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOUR);
		SDL_RenderClear(renderer);

		if (newTileMapPanelVisible())
		{
		    //TODO(denis): if the new tile map panel "tile size" field
		    // doesn't have anything in it, make the default the same as the
		    // current tile set's tile size
		    
		    if (newTileMapPanelDataReady())
		    {
			int32 x = tileMapArea.x;
			int32 y = tileMapArea.y;
			int32 tileMapWidth = 0;
			int32 tileMapHeight = 0;
			
			tileMap = createNewTileMap(x, y,
						   &tileMapWidth, &tileMapHeight);
			
			tileMapVisibleArea = tileMapArea;
			tileMapVisibleArea.w = tileMapWidth;
			tileMapVisibleArea.h = tileMapHeight;
			
			if (tileMapWidth > tileMapArea.w)
			{
			    int32 backgroundWidth = (tileMapArea.w/tileMap.tileSize)*tileMap.tileSize;
			    tileMapVisibleArea.w = backgroundWidth;
			    
			    backgroundBar =
				createFilledTexturedRect(renderer, backgroundWidth,
							 25, 0xFFFFFFFF);

			    real32 sizeRatio = (real32)tileMapArea.w/(real32)tileMapWidth;
			    
			    scrollingBar =
				createFilledTexturedRect(renderer, (int32)(tileMapArea.w*sizeRatio),
							 25, 0xFFAAAAAA);

			    scrollingBar.pos.y = backgroundBar.pos.y = tileMap.offset.y + MIN(tileMapHeight, tileMapArea.h) + 2;
			    scrollingBar.pos.x = backgroundBar.pos.x = tileMapArea.x;
			}
			if (tileMapHeight > tileMapArea.h)
			{
			    int32 backgroundHeight = (tileMapArea.h/tileMap.tileSize)*tileMap.tileSize;
			    tileMapVisibleArea.h = backgroundHeight;

			    //TODO(denis): set up vertical scroll bar
			}
#if 0
			//TODO(denis): want to do all this kind of thing inside the
			// resize event instead
			// automatically resizing things is kinda jarring
			
			int32 width = MAX(tileMapWidth, tileMapArea.w);
			int32 height = MAX(tileMapHeight, tileMapArea.h);
			
			colourSquare =
			    createFilledTexturedRect(renderer, width,
						     height, 0xFFDDDDDD);
			

			if (width >= tileMapPanel.panel.pos.w ||
			    height >= tileMapPanel.panel.pos.h)
			{
			    int windowHeight = 0;
			    int windowWidth = 0;
			    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			    
			    Vector2 windowDelta = {width - tileMapArea.w,
						   height - tileMapArea.h};
			    
			    if (windowDelta.x > 0)
			    {
				windowWidth += windowDelta.x;
			    }
			    else
				windowDelta.x = 0;
			    
			    if (windowDelta.y > 0)
			    {
				windowHeight += windowDelta.y;
			    }
			    else
				windowDelta.y = 0;

			    //NOTE(denis): don't make a window bigger than the user's screen
			    SDL_Rect screenSize = {};
			    SDL_GetDisplayBounds(0, &screenSize);

			    if (windowWidth > screenSize.w - 100)
				windowWidth = screenSize.w - 100;
			    if (windowHeight > screenSize.h - 100)
				windowHeight = screenSize.h - 100;
			    
			    SDL_SetWindowSize(window, windowWidth, windowHeight);

			    //TODO(denis): if the tile map created is bigger than the
			    // user's screen, the tile map should appear in a smaller
			    // area that is scrollable
			    // so that the whole editing area is still visible
			    
			    tileMapPanel.panel.pos.w += windowDelta.x;
			    tileMapPanel.panel.pos.h += windowDelta.y;

			    paintToolIcon.setPosition({paintToolIcon.background.pos.x + windowDelta.x,
					paintToolIcon.background.pos.y + windowDelta.y});
			    fillToolIcon.setPosition({fillToolIcon.background.pos.x + windowDelta.x,
					fillToolIcon.background.pos.y + windowDelta.y});

			    topMenuBar.botRight.x += windowDelta.x;

			    tileSetPanelSetPosition(tileSetPanelGetPosition()+windowDelta);
			}
#endif
			
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
			    int32 startTileX = (tileMapDrawPosOffset.x)/tileMap.tileSize;
			    int32 startTileY = (tileMapDrawPosOffset.y)/tileMap.tileSize;
			    
			    for (int32 i = startTileY; i < tileMap.heightInTiles; ++i)
			    {
				for (int32 j = startTileX; j < tileMap.widthInTiles; ++j)
				{
				    Tile *element = tileMap.tiles + i*tileMap.widthInTiles + j;

				    SDL_Texture *tileSet = tileSetPanelGetCurrentTileSet();
				    
				    if (!tileSet)
				    {
					tileSet = defaultTile.image;
				    }
				    
				    SDL_Rect drawRectSheet = element->sheetPos;
				    SDL_Rect drawRectScreen = element->pos;

				    //TODO(denis): only worrying about horizontal scrolling
				    // for now

				    //TODO(denis): breaks with default tile
				    // I think it's because i'm assuming the images
				    // have the same size as the tile size
				    if (drawRectScreen.x - tileMapDrawPosOffset.x
					< tileMapVisibleArea.x)
				    {
					drawRectScreen.x = tileMapVisibleArea.x;
					drawRectScreen.w -= tileMapDrawPosOffset.x % tileMap.tileSize;
					drawRectSheet.w = drawRectScreen.w;
					drawRectSheet.x += tileMapDrawPosOffset.x % tileMap.tileSize;
				    }
				    else
				    {
					drawRectScreen.x -= tileMapDrawPosOffset.x;
					
					if (drawRectScreen.x+drawRectScreen.w >
					    tileMapVisibleArea.x+tileMapVisibleArea.w)
					{
					    drawRectSheet.w = tileMapDrawPosOffset.x % tileMap.tileSize;
					    drawRectScreen.w = drawRectSheet.w;
					}
				    }
				    
				    drawRectScreen.h = drawRectSheet.h;
				    
				    //TODO(denis): first part of this is redundant
				    // because of startTileX
				    if (drawRectScreen.x+drawRectScreen.w >= tileMapVisibleArea.x
					&& drawRectScreen.x <= tileMapVisibleArea.x+tileMapVisibleArea.w)
				    {
				        SDL_RenderCopy(renderer, tileSet, &drawRectSheet, &drawRectScreen);
				    }
				}
			    }

			    if (backgroundBar.image != 0 && scrollingBar.image != 0)
			    {
				ui_draw(&backgroundBar);
				ui_draw(&scrollingBar);
			    }
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
