/* TODO(denis):
 *
 * CTRL + N -> create new tile map
 * CTRL + O -> open tile map
 * CTRL + S -> save current tile map
 * etc...
 *
 * DISABLE top menu bar when popup is visible
 *
 * add a keyboard shortcut to change to next tool and change to previous tool
 *
 * add tool tips
 *
 * when typing in the panels, disable the shortcut keys
 *
 * when using the fill tool, make right click cancel the selection entirely
 *
 * add the ability to close maps and tile sets
 *
 * Whenever a tile sheet is imported, copy the image file into a directory made by
 * the program ("tilesheets" or something) and when loading a tile map, check in
 * that directory first, and then prompt the user to load a tile set if not found
 * The problem with this approach is that if the tile sheet is changed outside the 
 * program, I might be loading an out of date file
 * so maybe prompt the user if the tile set I'm about to load is the actual one
 * they want?
 *
 */

//IMPORTANT(denis): note to self, design everything expecting at least a 1280 x 720
// window

#include <SDL.h>
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "windows.h"
#include <math.h>

#include "ui_elements.h"
#include "main.h"
#include "file_saving_loading.h"
#include "denis_math.h"
#include "new_tile_map_panel.h"
#include "tile_set_panel.h"
#include "tile_map_panel.h"
#include "import_tile_set_panel.h"
#include "TEMP_GeneralFunctions.cpp"

#define TITLE "Tile Map Editor"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BACKGROUND_COLOUR 60,67,69,255

static inline void openNewTileMapPanel()
{
    newTileMapPanelResetData();
    newTileMapPanelSetVisible(true);

    int32 tileSize = tileSetPanelGetCurrentTileSize();
    if (tileSize != 0)
    {
	newTileMapPanelSetTileSize(tileSize);
    }
}

static inline void openImportTileSetPanel()
{
    importTileSetPanelSetVisible(true);

    int32 tileSize = newTileMapPanelGetTileSize();
    if (tileSize != 0)
    {
	importTileSetPanelSetTileSize(tileSize);
    }
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
	uint32 renderFlags = SDL_RENDERER_PRESENTVSYNC;
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, renderFlags);

	if (renderer && ui_init(renderer, defaultFontName, defaultFontSize)
	    && IMG_Init(IMG_INIT_PNG) != 0)
	{   
	    bool running = true;

	    //NOTE(denis): setting up the top bar
	    ui_setFont(menuFontName, menuFontSize);

	    MenuBar topMenuBar = ui_createMenuBar(0, 0, WINDOW_WIDTH, 20,
						  0xFF667378, 0xFF000000);
	    
	    char *items[] = {"File", "Create New Tile Map", "Open Tile Map...",
			     "Save Tile Map...", "Import Tile Sheet...", "Exit"};	     
	    topMenuBar.addMenu(items, 6, 225);
	    
	    items[0] = "Tile Maps";
	    items[1] = "Create New";
	    topMenuBar.addMenu(items, 2, 225);
	    
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
	    {
		int x = WINDOW_WIDTH/2 - 900/2;
		int y = WINDOW_HEIGHT/2 - 150/2;
		importTileSetPanelCreateNew(renderer, x, y, 0, 0);
	    }
	    
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
	    {
		uint32 padding = 15;
		uint32 x = padding;
		uint32 y = topMenuBar.botRight.y + padding;
		uint32 width = 800;
		uint32 height = WINDOW_HEIGHT - y - padding;
		
		tileMapPanelCreateNew(renderer, x, y, width, height);
	    }
	    
	    while (running)
	    {
		static uint32 topMenuOpenDelay = 0;
                #define DELAY_THRESHOLD 20
		
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
			    int32 leftClickFlag = event.motion.state & SDL_BUTTON_LMASK;
			    
			    topMenuBar.onMouseMove(mouse);
			    if (!topMenuBar.isOpen())
			    {
				if (tileSetPanelVisible())
				{
				    tileSetPanelOnMouseMove(mouse);
				}

				if (tileMapPanelVisible())
				{
				    tileMapPanelOnMouseMove(mouse, leftClickFlag);
				}
			    }
			    
			} break;

			case SDL_MOUSEBUTTONDOWN:
			{
			    Vector2 mouse = {event.button.x, event.button.y};
			    uint8 mouseButton = event.button.button;

			    if (importTileSetPanelVisible())
			    {
				importTileSetPanelOnMouseDown(mouse, mouseButton);
			    }
			    else if (newTileMapPanelVisible())
			    {
				newTileMapPanelOnMouseDown(mouse, mouseButton);
			    }
			    else if (tileSetPanelVisible() || tileMapPanelVisible())
			    {
				if (!topMenuBar.isOpen() &&
				    topMenuOpenDelay > DELAY_THRESHOLD)
				{
				    if (tileSetPanelVisible())
				    {
					tileSetPanelOnMouseDown(mouse, mouseButton);
				    }

				    if (tileMapPanelVisible())
				    {
					tileMapPanelOnMouseDown(mouse, mouseButton);
				    }
				}
			    }
			   
			    topMenuBar.onMouseDown(mouse, event.button.button);
			    
			} break;
			
			case SDL_MOUSEBUTTONUP:
			{   
			    Vector2 mouse = {event.button.x, event.button.y};
			    uint8 mouseButton = event.button.button;
			    
			    if (importTileSetPanelVisible())
			    {
				importTileSetPanelOnMouseUp(mouse, mouseButton);
			    }
			    else if (newTileMapPanelVisible())
			    {
				newTileMapPanelOnMouseUp(mouse, mouseButton);
			    }
			    else if (topMenuBar.onMouseUp(mouse, event.button.button))
			    {
				topMenuOpenDelay = 0;
				
				//TODO(denis): need better solution
				tileSetPanelOnMouseUp({0,0}, mouseButton);
				
				if (pointInRect(mouse, topMenuBar.menus[0].getRect()))
				{
				    //NOTE(denis): clicked on the file menu

				    topMenuBar.menus[0].isOpen = false;
				    
				    int selectionY = (mouse.y - topMenuBar.menus[0].getRect().y)/topMenuBar.menus[0].items[0].pos.h;
				    if (selectionY == 1)
				    {
					//NOTE(denis): 1 == "create new tile map"
				        openNewTileMapPanel();
				    }
				    else if (selectionY == 2)
				    {
					//NOTE(denis): 2 == "open tile map file"
					LoadTileMapResult data = {};
					data = loadTileMapFromFile();

					TileMapTile *tileMapTiles = (TileMapTile*)HEAP_ALLOC(data.tileMapWidth*data.tileMapHeight*sizeof(TileMapTile));

					assert(tileMapTiles);
					for (uint32 i = 0; i < data.tileMapHeight; ++i)
					{
					    for (uint32 j = 0; j < data.tileMapWidth; ++j)
					    {
						(tileMapTiles + i*data.tileMapWidth + j)->tile = *(data.tiles + i*data.tileMapWidth + j);
						(tileMapTiles + i*data.tileMapWidth + j)->initialized = true;
					    }
					}

					HEAP_FREE(data.tiles);
					
					TileMap *tileMap = tileMapPanelAddTileMap(tileMapTiles, data.tileMapName, data.tileMapWidth, data.tileMapHeight, data.tileSize,
										  data.tileSheetFileName);
					topMenuBar.menus[1].addItem(tileMap->name, topMenuBar.menus[1].itemCount-1);

					//TODO(denis): need to attempt to open the tile set that
					// is mentioned in the data
					// (not sure how to do that yet)
					// do I want to attempt to load, and if it
					// fails, prompt the user to open the tile
					// set?

					SDL_Surface *tileSet = 0;
					
					// check module directory, then current directory
					// using GetModuleFileName and GetCurrentDirectory
					TCHAR fileNameBuffer[MAX_PATH+1];
					DWORD result = GetModuleFileName(NULL, fileNameBuffer, MAX_PATH+1);
					if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
					{
					    char filePath[MAX_PATH+1] = {};
					    uint32 indexOfLastSlash = 0;
					    for (int i = 0; i < MAX_PATH; ++i)
					    {
						if (fileNameBuffer[i] == '\\')
						    indexOfLastSlash = i;
					    }

					    copyIntoString(filePath, fileNameBuffer,
							   0, indexOfLastSlash);
					    
					    char *tileSheetFullPath = concatStrings(filePath, data.tileSheetFileName);

					    tileSet = loadImageAsSurface(tileSheetFullPath);
					    HEAP_FREE(tileSheetFullPath);
					}
					else
					{
					    //TODO(denis): try again with a bigger buffer?
					}

					if (!tileSet)
					{
					    fileNameBuffer[0] = 0;
					    result = GetCurrentDirectory(MAX_PATH+1, fileNameBuffer);

					    if (result != 0 && result <= MAX_PATH+1)
					    {
						fileNameBuffer[result] = '\\';
						fileNameBuffer[result+1] = 0;
						
					        char *tileSheetFullPath = concatStrings(fileNameBuffer, data.tileSheetFileName);

						tileSet = loadImageAsSurface(tileSheetFullPath);
						HEAP_FREE(tileSheetFullPath);
					    }
					    else
					    {
						//TODO(denis): try again with bigger buffer?
					    }
					}

					if (tileSet)
					{
					    tileSetPanelInitializeNewTileSet(data.tileSheetFileName, tileSet, data.tileSize);
					}
					else
					{
					    //TODO(denis): prompt the user to open the tile set
					}
				    }
				    else if (selectionY == 3)
				    {
					//NOTE(denis): 3 == "save tile map to file"
					if (tileMapPanelTileMapIsValid())
					{
					    TileMap *current = tileMapPanelGetCurrentTileMap();
					    char *fileName = current->name;

					    saveTileMapToFile(current, fileName);
					}
				    }
				    else if (selectionY == 4)
				    {
					//NOTE(denis): 4 = "import tile sheet"
					openImportTileSetPanel();
				    }
				    else if (selectionY == 0)
				    {
					topMenuBar.menus[0].isOpen = true;
				    }
				    
				}
				else if (pointInRect(mouse, topMenuBar.menus[1].getRect()))
				{
				    //NOTE(denis): clicked on the tile map menu
				    topMenuBar.menus[1].isOpen = false;
				    
				    uint32 selectionY = (mouse.y - topMenuBar.menus[1].getRect().y)/topMenuBar.menus[1].items[0].pos.h;
				    if (selectionY == (uint32)(topMenuBar.menus[1].itemCount-1))
				    {
					openNewTileMapPanel();
				    }
				    else if (selectionY != 0)
				    {
					tileMapPanelSelectTileMap(selectionY-1);
				    }
				    else if (selectionY == 0)
				    {
					topMenuBar.menus[1].isOpen = true;
				    }
				}
			    }
			    else if (tileSetPanelVisible() || tileMapPanelVisible())
			    {
				if (!topMenuBar.isOpen() &&
				    topMenuOpenDelay > DELAY_THRESHOLD)
				{
				    if (tileSetPanelVisible())
				    {
					tileSetPanelOnMouseUp(mouse, mouseButton);

					if (tileSetPanelImportTileSetPressed())
					    openImportTileSetPanel();
				    }
			    
				    if (tileMapPanelVisible())
				    {
					tileMapPanelOnMouseUp(mouse, mouseButton);
				    }
				}
			    }
			} break;

			case SDL_TEXTINPUT:
			{
			    char* theText = event.text.text;

			    if (newTileMapPanelVisible())
			    {
				newTileMapPanelCharInput(theText[0]);
			    }
			    else if(importTileSetPanelVisible())
			    {
				importTileSetPanelCharInput(theText[0]);
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
				else if (importTileSetPanelVisible())
				{
				    importTileSetPanelCharDeleted();
				}  
			    }
			    
			    if (event.key.keysym.sym == SDLK_SPACE &&
				tileMapPanelVisible())
			    {
				tileMapPanelOnKeyPressed(SDLK_SPACE);
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

			    tileMapPanelOnKeyReleased(event.key.keysym.sym);
			} break;
		    }
		}

		if (topMenuOpenDelay <= DELAY_THRESHOLD)
		    ++topMenuOpenDelay;
		
		SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOUR);
		SDL_RenderClear(renderer);

		if (newTileMapPanelVisible())
		{   
		    if (newTileMapPanelDataReady())
		    {
			TileMap *tileMap = tileMapPanelCreateNewTileMap();
			topMenuBar.menus[1].addItem(tileMap->name,
						    topMenuBar.menus[1].itemCount-1);
		    }
		}

		tileSetPanelDraw();
		tileMapPanelDraw();    

		newTileMapPanelDraw();
		importTileSetPanelDraw();
		
		ui_draw(&topMenuBar);
	
		SDL_RenderPresent(renderer);
	    }

	    IMG_Quit();
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
