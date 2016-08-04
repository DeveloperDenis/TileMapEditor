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

static inline void openImportTileSetPanel(UIPanel *importTileSetPanel,
					  EditText *tileSizeEditText)
{
    importTileSetPanel->visible = true;

    int32 tileSize = newTileMapPanelGetTileSize();
    if (tileSize != 0)
    {
	char *tileSizeText = convertIntToString(tileSize);
	ui_setText(tileSizeEditText, tileSizeText);
	HEAP_FREE(tileSizeText);
    }
}

static inline void openNewTileMapPanel()
{
    newTileMapPanelSetVisible(true);

    int32 tileSize = tileSetPanelGetCurrentTileSize();
    if (tileSize != 0)
    {
	newTileMapPanelSetTileSize(tileSize);
    }
}

static inline void clipSelectionBoxToBoundary(TexturedRect *selectionBox,
					      SDL_Rect bounds)
{
    if (selectionBox->pos.x < bounds.x)
    {
	selectionBox->pos.w -= bounds.x - selectionBox->pos.x;
	selectionBox->pos.x = bounds.x;
    }
    if (selectionBox->pos.x + selectionBox->pos.w > bounds.x + bounds.w)
    {
	selectionBox->pos.w = (bounds.x + bounds.w) - selectionBox->pos.x;
    }

    if (selectionBox->pos.y < bounds.y)
    {
	selectionBox->pos.h -= bounds.y - selectionBox->pos.y;
	selectionBox->pos.y = bounds.y;
    }
    if (selectionBox->pos.y + selectionBox->pos.h > bounds.y + bounds.h)
    {
	selectionBox->pos.h = (bounds.y + bounds.h) - selectionBox->pos.y;
    }
}

static inline Vector2 convertTilePosToScreenPos(uint32 tileSize, Vector2 tileMapOffset,
						Vector2 scrollOffset, Vector2 tilePos)
{
    Vector2 screenPos = tileMapOffset;
    
    screenPos.x +=
	(tilePos.x - scrollOffset.x/tileSize)*tileSize - scrollOffset.x%tileSize;
    screenPos.y +=
	(tilePos.y - scrollOffset.y/tileSize)*tileSize - scrollOffset.y%tileSize;
    
    return screenPos;
}

static Vector2 convertScreenPosToTilePos(int32 tileSize, Vector2 tileMapOffset,
					 Vector2 scrollOffset, Vector2 screenPos)
{
    Vector2 tilePos = scrollOffset/tileSize;
    
    int32 firstTileWidth = tileSize - scrollOffset.x%tileSize;
    if (firstTileWidth < tileSize)
    {
        tileMapOffset.x += firstTileWidth;

	if (screenPos.x >= tileMapOffset.x)
	    ++tilePos.x;
    }
    tilePos.x += (screenPos.x - tileMapOffset.x)/tileSize;
					        
    int32 firstTileHeight = tileSize - scrollOffset.y%tileSize;
    if (firstTileHeight < tileSize)
    {
	tileMapOffset.y += firstTileHeight;

	if (screenPos.y >= tileMapOffset.y)
	    ++tilePos.y;
    }
    tilePos.y += (screenPos.y - tileMapOffset.y)/tileSize;

    return tilePos;
}

static void paintSelectedTile(TileMap *tileMap, SDL_Rect tileMapArea,
			      Vector2 scrollOffset, Vector2 mousePos)
{
    int32 tileSize = tileMap->tileSize;
    Vector2 offset = {tileMapArea.x, tileMapArea.y};
    Vector2 tilePos = convertScreenPosToTilePos(tileSize, offset,
						scrollOffset, mousePos);
    
    Tile *clicked = tileMap->tiles + tilePos.x + tilePos.y*tileMap->widthInTiles;

    if (tileSetPanelGetSelectedTile().sheetPos.w != 0 &&
	tileSetPanelGetSelectedTile().sheetPos.h != 0)
    {
	clicked->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
    }
}

static void moveSelectionInScrolledMap(TexturedRect *selectionBox, SDL_Rect tileMapArea,
				       Vector2 scrollOffset, Vector2 point, int32 tileSize)
{
    Vector2 offset = {tileMapArea.x, tileMapArea.y};
    Vector2 selectedTile = point - offset;
    
    int32 firstTileWidth = tileSize - scrollOffset.x%tileSize;
    if (selectedTile.x > firstTileWidth)
    {
	selectedTile.x -= firstTileWidth;

	selectionBox->pos.x = (selectedTile.x/tileSize)*tileSize + offset.x + firstTileWidth;
	selectionBox->pos.w = MIN(tileSize, tileMapArea.x + tileMapArea.w - selectionBox->pos.x);
    }
    else
    {
	selectionBox->pos.w = firstTileWidth;
	selectionBox->pos.x = offset.x;
    }

    int32 firstTileHeight = tileSize - scrollOffset.y%tileSize;
    if (selectedTile.y > firstTileHeight)
    {
	selectedTile.y -= firstTileHeight;

	selectionBox->pos.y = (selectedTile.y/tileSize)*tileSize + offset.y + firstTileHeight;
	selectionBox->pos.h = MIN(tileSize, tileMapArea.y + tileMapArea.h - selectionBox->pos.y);
    }
    else
    {
	selectionBox->pos.h = firstTileHeight;
	selectionBox->pos.y = offset.y;
    }
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
	    TexturedRect backgroundBarX = {};
	    TexturedRect scrollingBarX = {};
	    TexturedRect backgroundBarY = {};
	    TexturedRect scrollingBarY = {};
	    int32 scrollClickDelta = 0;
	    bool mouseDownScrollX = false;
	    bool mouseDownScrollY = false;
	    Vector2 tileMapDrawPosOffset = {};
	    int32 scrollBarWidth = 25;
	    
	    SDL_Rect tileMapArea = {};
	    SDL_Rect tileMapVisibleArea = {};
	    
	    UIPanel tileMapPanel = {};
	    {
		int x = 15;
		int y = topMenuBar.botRight.y + 15;
		int width = 800 - scrollBarWidth;
		int height = WINDOW_HEIGHT - y - 15 - scrollBarWidth;
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
	    
	    const enum ToolType
	    {
		PAINT_TOOL,
		FILL_TOOL,
		MOVE_TOOL
	    };
	    ToolType currentTool = PAINT_TOOL;
	    
	    Button paintToolIcon = ui_createImageButton("paint-brush-icon-32.png");
	    Button fillToolIcon = ui_createImageButton("paint-can-icon-32.png");
	    Button moveToolIcon = ui_createImageButton("cursor-icon-32.png");
	    Vector2 lastFramePos = {};
	    
	    TexturedRect selectedToolIcon = loadImage(renderer, "selected-icon-32.png");
	    TexturedRect hoveringToolIcon = loadImage(renderer, "hovering-icon-32.png");
	    bool hoverToolIconVisible = false;
	    
	    {
		int x = tileMapPanel.panel.pos.x + tileMapPanel.getWidth() - paintToolIcon.getWidth() - 15;
		int y = tileMapPanel.panel.pos.y + 15;
		paintToolIcon.setPosition({x, y});
		selectedToolIcon.pos.x = x;
		selectedToolIcon.pos.y = y;
		    
		y += paintToolIcon.getHeight() + 5;
		fillToolIcon.setPosition({x, y});

		y += fillToolIcon.getHeight() + 5;
		moveToolIcon.setPosition({x, y});

		tileMapArea.w = x - tileMapArea.x - 15;
	    }
	    ui_addToPanel(&paintToolIcon, &tileMapPanel);
	    ui_addToPanel(&fillToolIcon, &tileMapPanel);
	    ui_addToPanel(&moveToolIcon, &tileMapPanel);
	    ui_addToPanel(&selectedToolIcon, &tileMapPanel);

	    SDL_Cursor *arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	    SDL_Cursor *handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	    
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

			    if (mouseDownScrollX)
			    {
				scrollingBarX.pos.x = mouse.x - scrollClickDelta;
				if (scrollingBarX.pos.x < backgroundBarX.pos.x)
				    scrollingBarX.pos.x = backgroundBarX.pos.x;
				else if (scrollingBarX.pos.x > backgroundBarX.pos.x + backgroundBarX.pos.w - scrollingBarX.pos.w)
				    scrollingBarX.pos.x = backgroundBarX.pos.x + backgroundBarX.pos.w - scrollingBarX.pos.w;

				real32 percentMoved = (real32)(scrollingBarX.pos.x-backgroundBarX.pos.x)/(real32)(backgroundBarX.pos.w - scrollingBarX.pos.w);
				tileMapDrawPosOffset.x = (int32)((tileMap.widthInTiles*tileMap.tileSize - tileMapVisibleArea.w)*percentMoved);
			    }
			    else if (mouseDownScrollY)
			    {
				scrollingBarY.pos.y = mouse.y - scrollClickDelta;
				if (scrollingBarY.pos.y < backgroundBarY.pos.y)
				    scrollingBarY.pos.y = backgroundBarY.pos.y;
				else if (scrollingBarY.pos.y > backgroundBarY.pos.y + backgroundBarY.pos.h - scrollingBarY.pos.h)
				    scrollingBarY.pos.y = backgroundBarY.pos.y + backgroundBarY.pos.h - scrollingBarY.pos.h;

				real32 percentMoved = (real32)(scrollingBarY.pos.y-backgroundBarY.pos.y)/(real32)(backgroundBarY.pos.h - scrollingBarY.pos.h);
				tileMapDrawPosOffset.y = (int32)((tileMap.heightInTiles*tileMap.tileSize - tileMapVisibleArea.h)*percentMoved);
			    }
			    else if (currentTool == PAINT_TOOL && tileMapPanel.visible &&
				tileMap.tiles)
			    {
				if (selectionBox.pos.w != 0 && selectionBox.pos.h != 0)
				{
				    if (pointInRect(mouse, tileMapVisibleArea))
				    {
					selectionVisible = true;
				        
					moveSelectionInScrolledMap(&selectionBox, tileMapVisibleArea, tileMapDrawPosOffset, mouse, tileMap.tileSize);
				    }
				    else
				    {
					selectionVisible = false;
				    }
				}
				
				if (event.motion.state & SDL_BUTTON_LMASK)
				{
				    if (pointInRect(mouse, tileMapVisibleArea))
				    {
					paintSelectedTile(&tileMap, tileMapVisibleArea,
							  tileMapDrawPosOffset, mouse);
				    }
				}
			    }
			    else if (currentTool == FILL_TOOL && tileMapPanel.visible &&
				     tileMap.tiles)
			    {
				if (event.motion.state & SDL_BUTTON_LMASK &&
				    startSelectPos != Vector2{0,0})
				{
				    selectionVisible = true;

				    Vector2 offset = {tileMapArea.x, tileMapArea.y};

				    Vector2 tilePos =
					convertScreenPosToTilePos(tileMap.tileSize, offset, tileMapDrawPosOffset, mouse);
				    static int32 startY = tilePos.y;				    
				    Vector2 startedTilePos =
					convertScreenPosToTilePos(tileMap.tileSize, offset, tileMapDrawPosOffset, startSelectPos);

				    //TODO(denis): if the user is holding the mouse
				    // near an edge that can be scrolled more
				    // do it

				    //TODO(denis): do I still need these bounds checking
				    // if statements?
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
					selectionBox.pos.x =
					    convertTilePosToScreenPos(tileMap.tileSize, offset, tileMapDrawPosOffset, startedTilePos).x;
				    }	
				    else
				    {
					selectionBox.pos.x =
					    convertTilePosToScreenPos(tileMap.tileSize, offset, tileMapDrawPosOffset, tilePos).x;
				    }

				    selectionBox.pos.w = tileMap.tileSize + absValue(tilePos.x - startedTilePos.x)*tileMap.tileSize;
				    
				    if (startedTilePos.y < tilePos.y)
				    {
					selectionBox.pos.y =
					    convertTilePosToScreenPos(tileMap.tileSize, offset, tileMapDrawPosOffset, startedTilePos).y;
				    }
				    else
				    {
					selectionBox.pos.y =
					    convertTilePosToScreenPos(tileMap.tileSize, offset, tileMapDrawPosOffset, tilePos).y;
				    }
				    
				    selectionBox.pos.h = tileMap.tileSize + absValue(tilePos.y - startedTilePos.y)*tileMap.tileSize;

				    clipSelectionBoxToBoundary(&selectionBox, tileMapVisibleArea);
				}
				else
				{
				    selectionVisible = pointInRect(mouse, tileMapVisibleArea);
				    if (selectionVisible)
					moveSelectionInScrolledMap(&selectionBox, tileMapVisibleArea, tileMapDrawPosOffset, mouse, tileMap.tileSize);
				}
			    }
			    else if(currentTool == MOVE_TOOL && tileMapPanel.visible &&
				    tileMap.tiles)
			    {
				if (pointInRect(mouse, tileMapVisibleArea))
				{
				    SDL_SetCursor(handCursor);

				    if (event.motion.state & SDL_BUTTON_LMASK &&
					(backgroundBarX.image || backgroundBarY.image))
				    {
				        tileMapDrawPosOffset.x += lastFramePos.x - mouse.x;
				        tileMapDrawPosOffset.y += lastFramePos.y - mouse.y;

					if (tileMapDrawPosOffset.x < 0)
					{
					    tileMapDrawPosOffset.x = 0;
					}
					else if (tileMapDrawPosOffset.x > tileMap.widthInTiles*tileMap.tileSize - tileMapVisibleArea.w)
					{
					    tileMapDrawPosOffset.x = tileMap.widthInTiles*tileMap.tileSize - tileMapVisibleArea.w;
					}

					if (tileMapDrawPosOffset.y < 0)
					{
					    tileMapDrawPosOffset.y = 0;
					}
					else if (tileMapDrawPosOffset.y > tileMap.heightInTiles*tileMap.tileSize - tileMapVisibleArea.h)
					{
					    tileMapDrawPosOffset.y = tileMap.heightInTiles*tileMap.tileSize - tileMapVisibleArea.h;
					}

					scrollingBarY.pos.y = (int32)((real32)tileMapDrawPosOffset.y / (tileMap.heightInTiles*tileMap.tileSize - tileMapVisibleArea.h) * (backgroundBarY.pos.h - scrollingBarY.pos.h) + backgroundBarY.pos.y);
					scrollingBarX.pos.x = (int32)((real32)tileMapDrawPosOffset.x / (tileMap.widthInTiles*tileMap.tileSize - tileMapVisibleArea.w) * (backgroundBarX.pos.w - scrollingBarX.pos.w) + backgroundBarX.pos.x);
					
					if (scrollingBarX.pos.x < backgroundBarX.pos.x)
					{
					    scrollingBarX.pos.x = backgroundBarX.pos.x;
					}
					else if (scrollingBarX.pos.x > backgroundBarX.pos.x + backgroundBarX.pos.w - scrollingBarX.pos.w)
					{
					    scrollingBarX.pos.x = backgroundBarX.pos.x + backgroundBarX.pos.w - scrollingBarX.pos.w;
					}

					if (scrollingBarY.pos.y < backgroundBarY.pos.y)
					{
					    scrollingBarY.pos.y = backgroundBarY.pos.y;
					}
					else if (scrollingBarY.pos.y > backgroundBarY.pos.y + backgroundBarY.pos.h - scrollingBarY.pos.h)
					{
					    scrollingBarY.pos.y = backgroundBarY.pos.y + backgroundBarY.pos.h - scrollingBarY.pos.h;
					}

					lastFramePos = mouse;
				    }
				}
				else
				{
				    SDL_SetCursor(arrowCursor);
				}
			    }

			    hoverToolIconVisible = true;
			    if (pointInRect(mouse, paintToolIcon.background.pos))
			    {
				hoveringToolIcon.pos.x = paintToolIcon.background.pos.x;
				hoveringToolIcon.pos.y = paintToolIcon.background.pos.y;
			    }
			    else if (pointInRect(mouse, fillToolIcon.background.pos))
			    {
				hoveringToolIcon.pos.x = fillToolIcon.background.pos.x;
				hoveringToolIcon.pos.y = fillToolIcon.background.pos.y;
			    }
			    else if (pointInRect(mouse, moveToolIcon.background.pos))
			    {
				hoveringToolIcon.pos.x = moveToolIcon.background.pos.x;
				hoveringToolIcon.pos.y = moveToolIcon.background.pos.y;
			    }
			    else
			    {
				hoverToolIconVisible = false;
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
				    if (pointInRect(mouse, scrollingBarX.pos))
				    {
					mouseDownScrollX = true;
					scrollClickDelta = mouse.x - scrollingBarX.pos.x;
				    }
				    else if (pointInRect(mouse, scrollingBarY.pos))
				    {
					mouseDownScrollY = true;
					scrollClickDelta = mouse.y - scrollingBarY.pos.y;
				    }
				    
				    ui_processMouseDown(&tileMapPanel, mouse, mouseButton);
				    
				    createNewTileMapButton.startedClick =
					pointInRect(mouse, createNewTileMapButton.background.pos);

				    if (currentTool == PAINT_TOOL && tileMap.tiles)
				    {
					if (event.button.button == SDL_BUTTON_LEFT)
					{
					    if (pointInRect(mouse, tileMapVisibleArea))
					    {	
						paintSelectedTile(&tileMap, tileMapVisibleArea,
								  tileMapDrawPosOffset, mouse);
					    }
					}
				    }
				    else if (currentTool == FILL_TOOL && tileMap.tiles)
				    {
					if (event.button.button == SDL_BUTTON_LEFT)
					{
					    if (pointInRect(mouse, tileMapVisibleArea))
					    {
						Vector2 offset = {tileMapVisibleArea.x, tileMapVisibleArea.y};

						//TODO(denis): this seems redudant and silly
						Vector2 tilePos =
						    convertScreenPosToTilePos(tileMap.tileSize, offset,
									      tileMapDrawPosOffset, mouse);

						startSelectPos =
						    convertTilePosToScreenPos(tileMap.tileSize, offset,
									      tileMapDrawPosOffset, tilePos);

						if (startSelectPos.x < offset.x)
						{
						    selectionBox.pos.x = offset.x;
						    selectionBox.pos.w = tileMap.tileSize - tileMapDrawPosOffset.x%tileMap.tileSize;
						}
						else
						{
						    selectionBox.pos.x = startSelectPos.x;   
						    selectionBox.pos.w = tileMap.tileSize;
						}

						if (startSelectPos.y < offset.y)
						{
						    selectionBox.pos.y = offset.y;
						    selectionBox.pos.h = tileMap.tileSize - tileMapDrawPosOffset.y%tileMap.tileSize;
						}
						else
						{
						    selectionBox.pos.y = startSelectPos.y;
						    selectionBox.pos.h = tileMap.tileSize;
						}

						clipSelectionBoxToBoundary(&selectionBox, tileMapVisibleArea);
					    }
					    else
					    {
						startSelectPos = {0, 0};
					    }
					}
				    }
				    else if (currentTool == MOVE_TOOL && tileMap.tiles)
				    {
					if ((backgroundBarX.image || backgroundBarY.image) &&
					    event.button.button == SDL_BUTTON_LEFT)
					{
					    if (pointInRect(mouse, tileMapVisibleArea))
					    {
					        lastFramePos = mouse;
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

			    mouseDownScrollY = false;
			    mouseDownScrollX = false;
			    
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
				    int tileSize = convertStringToInt(tileSizeEditText.text,
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
				        openNewTileMapPanel();
					topMenuBar.menus[0].isOpen = false;
				    }
				    else if (selectionY == 4)
				    {
					//NOTE(denis): 4 = "import tile sheet"
					openImportTileSetPanel(&importTileSetPanel, &tileSizeEditText);
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
				        openImportTileSetPanel(&importTileSetPanel, &tileSizeEditText);
				}
			    
				//TODO(denis): have something like "!click used" or
				// something
				if (tileMapPanel.visible)
				{
				    //TODO(denis): implement a proper "radio button" type
				    // situation
				    if (ui_wasClicked(paintToolIcon, mouse))
				    {
					selectedToolIcon.pos.x = paintToolIcon.background.pos.x;
					selectedToolIcon.pos.y = paintToolIcon.background.pos.y;
					
					currentTool = PAINT_TOOL;
					paintToolIcon.startedClick = false;
					
				    }
				    else if (ui_wasClicked(fillToolIcon, mouse))
				    {
					selectedToolIcon.pos.x = fillToolIcon.background.pos.x;
					selectedToolIcon.pos.y = fillToolIcon.background.pos.y;
					
					currentTool = FILL_TOOL;
					fillToolIcon.startedClick = false;
				    }
				    else if (ui_wasClicked(moveToolIcon, mouse))
				    {
					selectedToolIcon.pos.x = moveToolIcon.background.pos.x;
					selectedToolIcon.pos.y = moveToolIcon.background.pos.y;
					
					currentTool = MOVE_TOOL;
					moveToolIcon.startedClick = false;
				    }

				    if (!tileMap.tiles)
				    {
					if (ui_wasClicked(createNewTileMapButton, mouse))
					{
					    openNewTileMapPanel();
					    createNewTileMapButton.startedClick = false;
					}
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
				    
						Vector2 botRight = {selectionBox.pos.x+selectionBox.pos.w-1,
								    selectionBox.pos.y+selectionBox.pos.h-1};

						Vector2 offset = {tileMapVisibleArea.x, tileMapVisibleArea.y};
						Vector2 startTile =
						    convertScreenPosToTilePos(tileMap.tileSize, offset, tileMapDrawPosOffset, topLeft);
						Vector2 endTile =
						    convertScreenPosToTilePos(tileMap.tileSize, offset, tileMapDrawPosOffset, botRight);

						if (endTile.x > tileMap.widthInTiles)
						{
						    endTile.x = tileMap.widthInTiles;
						}
						if (endTile.y > tileMap.heightInTiles)
						{
						    endTile.y = tileMap.heightInTiles;
						}
				    
						for (int i = startTile.y; i <= endTile.y; ++i)
						{
						    for (int j = startTile.x; j <= endTile.x; ++j)
						    {
							if (tileSetPanelGetSelectedTile().sheetPos.w != 0)
							{
							    (tileMap.tiles + i*tileMap.widthInTiles + j)->sheetPos = tileSetPanelGetSelectedTile().sheetPos;
							}
						    }
						}
					    }

					    selectionVisible = pointInRect(mouse, tileMapVisibleArea);
					    if (selectionVisible)
					    {
						moveSelectionInScrolledMap(&selectionBox, tileMapVisibleArea, tileMapDrawPosOffset, mouse, tileMap.tileSize);
					    }
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
			tileMapVisibleArea.w = MIN(tileMapWidth, (tileMapArea.w/tileMap.tileSize)*tileMap.tileSize);
			tileMapVisibleArea.h = MIN(tileMapHeight, (tileMapArea.h/tileMap.tileSize)*tileMap.tileSize);
			
			if (tileMapWidth > tileMapArea.w)
			{
			    int32 backgroundWidth = (tileMapArea.w/tileMap.tileSize)*tileMap.tileSize;
			    tileMapVisibleArea.w = backgroundWidth;
			    
			    backgroundBarX =
				createFilledTexturedRect(renderer, backgroundWidth,
							 scrollBarWidth, 0xFFFFFFFF);

			    real32 sizeRatio = (real32)tileMapArea.w/(real32)tileMapWidth;
			    
			    scrollingBarX =
				createFilledTexturedRect(renderer, (int32)(tileMapArea.w*sizeRatio),
							 scrollBarWidth, 0xFFAAAAAA);

			    scrollingBarX.pos.y = backgroundBarX.pos.y = tileMap.offset.y + tileMapVisibleArea.h + 4;
			    scrollingBarX.pos.x = backgroundBarX.pos.x = tileMapArea.x;
			}
			if (tileMapHeight > tileMapArea.h)
			{
			    int32 backgroundHeight = (tileMapArea.h/tileMap.tileSize)*tileMap.tileSize;
			    tileMapVisibleArea.h = backgroundHeight;

			    backgroundBarY =
				createFilledTexturedRect(renderer, scrollBarWidth, backgroundHeight,
							 0xFFFFFFFF);

			    real32 sizeRatio = (real32)tileMapArea.h/(real32)tileMapHeight;

			    scrollingBarY =
				createFilledTexturedRect(renderer, scrollBarWidth, (int32)(tileMapArea.h*sizeRatio),
							 0xFFAAAAAA);

			    scrollingBarY.pos.y = backgroundBarY.pos.y = tileMapArea.y;
			    scrollingBarY.pos.x = backgroundBarY.pos.x = tileMap.offset.x + tileMapVisibleArea.w + 4;
			}
			
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

		    if (hoverToolIconVisible)
		    {
			ui_draw(&hoveringToolIcon);
		    }
		    
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

				    bool usingDefault = false;
				    if (!tileSet)
				    {
					tileSet = defaultTile.image;
					usingDefault = true;
				    }
				    
				    SDL_Rect drawRectSheet = element->sheetPos;
				    SDL_Rect drawRectScreen = element->pos;

				    if (drawRectScreen.x - tileMapDrawPosOffset.x
					< tileMapVisibleArea.x)
				    {
					drawRectScreen.x = tileMapVisibleArea.x;
					drawRectScreen.w -= tileMapDrawPosOffset.x % tileMap.tileSize;
					drawRectSheet.w = drawRectScreen.w;

					if (usingDefault)
					{
					    real32 ratio = (real32)defaultTile.pos.w/(real32)tileMap.tileSize;
					    drawRectSheet.x += (int32)((tileMapDrawPosOffset.x % tileMap.tileSize)*ratio);
					}
					else
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

				    if (drawRectScreen.y - tileMapDrawPosOffset.y <
					tileMapVisibleArea.y)
				    {
					drawRectScreen.y = tileMapVisibleArea.y;
					drawRectScreen.h -= tileMapDrawPosOffset.y % tileMap.tileSize;
					drawRectSheet.h = drawRectScreen.h;

					if (usingDefault)
					{
					    real32 ratio = (real32)defaultTile.pos.h/(real32)tileMap.tileSize;
					    drawRectSheet.y += (int32)((tileMapDrawPosOffset.y % tileMap.tileSize)*ratio);
					}
					else
					    drawRectSheet.y += tileMapDrawPosOffset.y % tileMap.tileSize;
				    }
				    else
				    {
					drawRectScreen.y -= tileMapDrawPosOffset.y;

					if (drawRectScreen.y + drawRectScreen.h >
					    tileMapVisibleArea.y + tileMapVisibleArea.h)
					{
					    drawRectSheet.h = tileMapDrawPosOffset.y % tileMap.tileSize;
					    drawRectScreen.h = drawRectSheet.h;
					}
				    }
				    
				    //TODO(denis): first part of this is redundant
				    // because of startTileX
				    if (drawRectScreen.x+drawRectScreen.w >= tileMapVisibleArea.x
					&& drawRectScreen.x <= tileMapVisibleArea.x+tileMapVisibleArea.w &&
					drawRectScreen.y+drawRectScreen.h >= tileMapVisibleArea.y &&
					drawRectScreen.y <= tileMapVisibleArea.y+tileMapVisibleArea.h)
				    {
				        SDL_RenderCopy(renderer, tileSet, &drawRectSheet, &drawRectScreen);
				    }
				}
			    }

			    if (backgroundBarX.image != 0 && scrollingBarX.image != 0)
			    {
				ui_draw(&backgroundBarX);
				ui_draw(&scrollingBarX);
			    }
			    if (backgroundBarY.image != 0 && scrollingBarY.image != 0)
			    {
				ui_draw(&backgroundBarY);
				ui_draw(&scrollingBarY);
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
