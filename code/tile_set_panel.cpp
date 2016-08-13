#include "SDL_render.h"
#include "SDL_surface.h"
#include "ui_elements.h"
#include "denis_math.h"
#include "main.h"
#include "tile_set_panel.h"
#include "TEMP_GeneralFunctions.cpp"

#define MIN_WIDTH 420
#define MIN_HEIGHT 670

#define PANEL_COLOUR 0xFF333333
#define PANEL_TEXT_COLOUR 0xFFFFFFFF
#define DROP_DOWN_COLOUR 0xFFAAAAAA
#define DROP_DOWN_TEXT_COLOUR 0xFF000000

#define PADDING 15

#define ALPHA_THRESHOLD 15

static SDL_Renderer *_renderer;

static TileSet _tileSets[15];
static uint32 _numTileSets;

static UIPanel _panel;
static DropDownMenu _tileSetDropDown;
static TexturedRect _selectedTileText;
static Tile _selectedTile;
static SDL_Rect _tempSelectedTile;

static TexturedRect _selectionBox;
static bool _selectionVisible;

static bool _importTileSetPressed;

static bool _startedClick;

//NOTE(denis): right now this function only returns false if every single pixel in
// a tile has an alpha value below the threshold
static bool tileIsValid(SDL_Surface *image, SDL_Rect tile)
{
    bool result = false;

    //TODO(denis): how much alpha should I allow in a tile before I consider it
    // "invalid"?

    if (SDL_MUSTLOCK(image) == SDL_TRUE)
	SDL_LockSurface(image);

    uint32 bytesPerPixel = image->format->BytesPerPixel;
    
    if (bytesPerPixel == 4)
    {
	uint32 *row = (uint32*)((uint8*)image->pixels + tile.x*bytesPerPixel + tile.y*image->pitch);
	for (int32 i = 0; i < tile.h && !result; ++i)
	{
	    uint32 *pixel = row;
	    for (int32 j = 0; j < tile.w && !result; ++j)
	    {
		uint32 alphaValue = (*pixel) & image->format->Amask;
		alphaValue >>= image->format->Ashift;

		if (alphaValue > ALPHA_THRESHOLD)
		{
		    result = true;
		}
		
		++pixel;
	    }
	    row = (uint32*)((uint8*)row + image->pitch);
	}
    }
    
    if (SDL_MUSTLOCK(image) == SDL_TRUE)
	SDL_UnlockSurface(image);
    
    return result;
}

void tileSetPanelCreateNew(SDL_Renderer *renderer,
			   uint32 x, uint32 y, uint32 width, uint32 height)
{
    _renderer = renderer;
    
    width = MAX(width, MIN_WIDTH);
    height = MAX(height, MIN_HEIGHT);
    
    _panel = ui_createPanel(x, y, width, height, PANEL_COLOUR);
    
    {
	char *items[] = {"No Tile Sheet Selected",
			 "Import a new tile sheet .."};
	int num = 2;
	int width = _panel.getWidth()-PADDING*2;
	int height = 40;
	_tileSetDropDown =
	    ui_createDropDownMenu(items, num, width, height, DROP_DOWN_TEXT_COLOUR,
				  DROP_DOWN_COLOUR);
    }
    ui_addToPanel(&_tileSetDropDown, &_panel);

    _selectedTileText = ui_createTextField("Selected Tile:", 0, 0, PANEL_TEXT_COLOUR);

    tileSetPanelSetPosition({_panel.panel.pos.x, _panel.panel.pos.y});
}

void tileSetPanelDraw()
{
    if (_panel.visible)
    {
	ui_draw(&_panel);
		    
	if (_tileSets[0].image != 0)
	{
	    int tileSize = _tileSets[0].tileSize;
	    int tilesPerRow = (_panel.getWidth() - PADDING*2)/tileSize;
			
	    int tilesPadding = (_panel.getWidth() - tilesPerRow*tileSize)/2;
	    int tileSetStartX = _panel.panel.pos.x + tilesPadding;
	    int tileSetStartY = _tileSetDropDown.items[0].pos.y +
		_tileSetDropDown.items[0].pos.h + PADDING;
	    
	    uint32 lastY = 0;
	    
	    for (uint32 i = 0; i < _tileSets[0].numTiles; ++i)
	    {
		if (i == _tileSets[0].numTiles-1)
		{
		    int x = 0;
		}
		
		//TODO(denis): this doesn't need to be done every frame
		// only when everything is resized
		uint32 xValue = tileSetStartX + (i%tilesPerRow)*tileSize;
		uint32 yValue = tileSetStartY + (i/tilesPerRow)*tileSize;
		
		Tile *currentTile = _tileSets[0].tiles + i;
		
		currentTile->pos.x = xValue;
		currentTile->pos.y = yValue;

		lastY = yValue;
		
		currentTile->pos.w = tileSize;
		currentTile->pos.h = tileSize;

		_tempSelectedTile.w = _tempSelectedTile.h = tileSize;

		if (currentTile->pos.x == _selectionBox.pos.x &&
		    currentTile->pos.y == _selectionBox.pos.y)
		{
		    _tempSelectedTile.x = currentTile->sheetPos.x;
		    _tempSelectedTile.y = currentTile->sheetPos.y;
		}
		
		SDL_RenderCopy(_renderer, _tileSets[0].image,
			       &currentTile->sheetPos, &currentTile->pos);
	    }
	    
	    //TODO(denis): should have multiple rectangles to better define the
	    // actual collision area
	    //TODO(denis): the height is wrong if there are a bunch of
	    // blank tiles being "drawn"
	    _tileSets[0].collisionBox.x = tileSetStartX;
	    _tileSets[0].collisionBox.y = tileSetStartY;
	    _tileSets[0].collisionBox.w = tilesPerRow*tileSize;
	    _tileSets[0].collisionBox.h = lastY - tileSetStartY;
			
	    ui_draw(&_selectedTileText);
	    SDL_RenderCopy(_renderer, _tileSets[0].image,
			   &_selectedTile.sheetPos, &_selectedTile.pos);
	}

	//TODO(denis): bad fix for the drawing order problem
	ui_draw(&_tileSetDropDown);

	if (_selectionVisible)
	    SDL_RenderCopy(_renderer, _selectionBox.image, NULL, &_selectionBox.pos);
    }
}

void tileSetPanelOnMouseMove(Vector2 mousePos)
{
    if (_tileSetDropDown.isOpen && pointInRect(mousePos, _tileSetDropDown.getRect()))
    {
	int highlighted = _tileSetDropDown.getItemAt(mousePos);
	_tileSetDropDown.highlightedItem = highlighted;
    }
    else if (!_tileSetDropDown.isOpen)
    {
	//TODO(denis): have "not initialized" check function ?
	if (_selectionBox.pos.w != 0 && _selectionBox.pos.h != 0 &&
	    _tileSets[0].tileSize != 0 &&
	    _tileSets[0].collisionBox.w != 0 &&
	    _tileSets[0].collisionBox.h != 0)
	{
	    _selectionVisible =
		moveSelectionInRect(&_selectionBox, mousePos,
				    _tileSets[0].collisionBox,
				    _tileSets[0].tileSize);
	}
    }
}

void tileSetPanelOnMouseDown(Vector2 mousePos, uint8 mouseButton)
{
    if (mouseButton == SDL_BUTTON_LEFT)
    {
	_tileSetDropDown.startedClick = pointInRect(mousePos, _tileSetDropDown.getRect());

	_startedClick = pointInRect(mousePos, _tileSets[0].collisionBox);
    }
}

void tileSetPanelOnMouseUp(Vector2 mousePos, uint8 mouseButton)
{
    if (_tileSetDropDown.isOpen)
    {
	if (pointInRect(mousePos, _tileSetDropDown.getRect()))
	{
	    if (mouseButton == SDL_BUTTON_LEFT)
	    {
		int selection = _tileSetDropDown.getItemAt(mousePos);
		if (selection == _tileSetDropDown.itemCount-1)
		{
		    _importTileSetPressed = true;
		}
		else
		{
		    SWAP_DATA(_tileSets[selection], _tileSets[0], TileSet);

		    _tileSetDropDown.items[selection].setPosition(_tileSetDropDown.items[0].getPosition());
		    SWAP_DATA(_tileSetDropDown.items[selection],
			      _tileSetDropDown.items[0], TextBox);

		    _selectedTile.sheetPos.x = 0;
		    _selectedTile.sheetPos.y = 0;
		}
	    }
	}
	
	_tileSetDropDown.highlightedItem = -1;
	_tileSetDropDown.isOpen = false;
    }
    else if (pointInRect(mousePos, _tileSetDropDown.getRect())
	     && _tileSetDropDown.startedClick && mouseButton == SDL_BUTTON_LEFT)
    {
	_tileSetDropDown.isOpen = true;
	_tileSetDropDown.highlightedItem = 0;
    }
    else if (_selectionVisible && _startedClick && mouseButton == SDL_BUTTON_LEFT)
    {
	_selectedTile.sheetPos = _tempSelectedTile;
    }
}

void tileSetPanelInitializeNewTileSet(char *name, SDL_Surface *image, uint32 tileSize)
{
    TileSet *currentTileSet = &_tileSets[_numTileSets];
    ++_numTileSets;
    //TODO(denis): currentTileSet.name has to be freed if ever
    // the tileset is deleted
    currentTileSet->name = name;
    currentTileSet->surface = image;
    currentTileSet->image = SDL_CreateTextureFromSurface(_renderer, image);
    currentTileSet->tileSize = tileSize;
    SDL_GetClipRect(image, &currentTileSet->imageSize);
    
    uint32 numXTiles = currentTileSet->imageSize.w/tileSize;
    uint32 numYTiles = currentTileSet->imageSize.h/tileSize;
    currentTileSet->tiles = (Tile*)HEAP_ALLOC(sizeof(Tile)*numXTiles*numYTiles);
    currentTileSet->numTiles = 0;

    //NOTE(denis): keep track of every valid tile in the tile set
    for (uint32 i = 0; i < numYTiles; ++i)
    {
	for (uint32 j = 0; j < numXTiles; ++j)
	{
	    SDL_Rect currentTile = {j*tileSize, i*tileSize, tileSize, tileSize};
	    if (tileIsValid(image, currentTile))
	    {
		(*(currentTileSet->tiles + currentTileSet->numTiles)).sheetPos = currentTile;
		++currentTileSet->numTiles;
	    }
	}
    }
    
    if (_numTileSets == 1)
    {
	_tileSetDropDown.changeItem(name, 0);
	_tileSetDropDown.setPosition({_tileSetDropDown.getRect().x, _tileSetDropDown.getRect().y});
    }
    else
    {
	int newPos = _tileSetDropDown.itemCount-1;
	_tileSetDropDown.addItem(name, newPos);

	SWAP_DATA(_tileSets[newPos], _tileSets[0], TileSet);

	_tileSetDropDown.items[newPos].setPosition(_tileSetDropDown.items[0].getPosition());
	SWAP_DATA(_tileSetDropDown.items[newPos],
		  _tileSetDropDown.items[0], TextBox);
    }
    
    _selectedTile.pos.w = tileSize;
    _selectedTile.pos.h = tileSize;
    _selectedTile.sheetPos.x = 0;
    _selectedTile.sheetPos.y = 0;
    _selectedTile.sheetPos.w = tileSize;
    _selectedTile.sheetPos.h = tileSize;
    
    _selectedTileText.pos.y = _panel.panel.pos.y + _panel.getHeight() -
	_selectedTileText.pos.h - PADDING - tileSize/2;
    _selectedTile.pos.y = _selectedTileText.pos.y - tileSize/2 + _selectedTileText.pos.h/2;

    initializeSelectionBox(_renderer, &_selectionBox, tileSize);
}

Tile tileSetPanelGetSelectedTile()
{
    return _selectedTile;
}

SDL_Texture* tileSetPanelGetCurrentTileSet()
{
    return _tileSets[0].image;
}

bool tileSetPanelImportTileSetPressed()
{
    bool result = false;
    
    if (_importTileSetPressed)
    {
	result = true;
	_importTileSetPressed = false;
    }

    return result;
}

int tileSetPanelGetCurrentTileSize()
{
    int result = 0;
    
    if (_numTileSets > 0)
	result = _tileSets[0].tileSize;

    return result;
}

bool tileSetPanelVisible()
{
    return _panel.visible;
}

Vector2 tileSetPanelGetPosition()
{
    return Vector2{_panel.panel.pos.x, _panel.panel.pos.y};
}

void tileSetPanelSetPosition(Vector2 newPos)
{
    _panel.panel.pos.x = newPos.x;
    _panel.panel.pos.y = newPos.y;

    _tileSetDropDown.setPosition({_panel.panel.pos.x + PADDING,
		_panel.panel.pos.y + PADDING});

    {
	int x = _tileSetDropDown.getRect().x;
	int y = _panel.panel.pos.y + _panel.getHeight() -
	    _selectedTileText.pos.h - PADDING;

	_selectedTileText.pos.y = y;
	_selectedTileText.pos.x = x;
    }

    _selectedTile.pos.x = _selectedTileText.pos.x + _selectedTileText.pos.w + PADDING;
    _selectedTile.pos.y = _selectedTileText.pos.y;
}
