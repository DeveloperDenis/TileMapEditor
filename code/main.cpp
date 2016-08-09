/* TODO(denis):
 * support saving to different formats?
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
 *
 * add a keyboard shortcut to change to next tool and change to previous tool
 *
 * add tool tips
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
#include "new_tile_map_panel.h"
#include "tile_set_panel.h"
#include "tile_map_panel.h"
#include "import_tile_set_panel.h"
#include "TEMP_GeneralFunctions.cpp"

#include "ui_elements.h"

#define TITLE "Tile Map Editor"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BACKGROUND_COLOUR 60,67,69,255

static inline void openNewTileMapPanel()
{
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
	//TODO(denis): maybe add renderer flags?
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

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
				if (tileSetPanelVisible())
				{
				    tileSetPanelOnMouseDown(mouse, mouseButton);
				}

				if (tileMapPanelVisible())
				{
				    tileMapPanelOnMouseDown(mouse, mouseButton);
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
				    }
				    else if (selectionY == 3)
				    {
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
				    
				    topMenuBar.menus[0].isOpen = false;
				}
				else if (pointInRect(mouse, topMenuBar.menus[1].getRect()))
				{
				    //NOTE(denis): clicked on the tile map menu
				    uint32 selectionY = (mouse.y - topMenuBar.menus[1].getRect().y)/topMenuBar.menus[1].items[0].pos.h;
				    if (selectionY == (uint32)(topMenuBar.menus[1].itemCount-1))
				    {
					openNewTileMapPanel();
				    }
				    else if (selectionY != 0)
				    {
					tileMapPanelSelectTileMap(selectionY-1);
				    }
				    
				    topMenuBar.menus[1].isOpen = false;
				}
			    }
			    else if (tileSetPanelVisible() || tileMapPanelVisible())
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

		SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOUR);
		SDL_RenderClear(renderer);

		if (newTileMapPanelVisible())
		{   
		    if (newTileMapPanelDataReady())
		    {
			TileMap *tileMap = tileMapPanelAddNewTileMap();
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
