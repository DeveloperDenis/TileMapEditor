#include "SDL_render.h"
#include "ui_elements.h"
#include "denis_math.h"
#include "main.h"
#include "tile_set_panel.h"
#include "TEMP_GeneralFunctions.cpp"

#define MIN_WIDTH 420
#define MIN_HEIGHT 670
#define BACKGROUND_COLOUR 0xFF00BB55
#define PADDING 15

static SDL_Renderer *_renderer;

static TileSet _tileSets[15];
static uint32 _numTileSets;

static UIPanel _panel;
static DropDownMenu _tileSetDropDown;
static TexturedRect _selectedTileText;
static Tile _selectedTile;

static TexturedRect _selectionBox;
static bool _selectionVisible;

static bool _importTileSetPressed;

//TODO(denis): do I even need this?
static SDL_Rect _tempSelectedTile;

void tileSetPanelCreateNew(SDL_Renderer *renderer,
			   uint32 x, uint32 y, uint32 width, uint32 height)
{
    _renderer = renderer;
    
    width = MAX(width, MIN_WIDTH);
    height = MAX(height, MIN_HEIGHT);
    
    _panel = ui_createPanel(x, y, width, height, BACKGROUND_COLOUR);
    
    {
	char *items[] = {"No Tile Sheet Selected",
			 "Import a new tile sheet .."};
	int num = 2;
	int width = _panel.getWidth()-PADDING*2;
	int height = 40;
	uint32 backgroundColour = 0xFFEEEEEE;
	_tileSetDropDown =
	    ui_createDropDownMenu(items, num, width, height, COLOUR_BLACK,
				  backgroundColour);

	Vector2 newPos = {_panel.panel.pos.x + PADDING,
			  _panel.panel.pos.y + PADDING};
	_tileSetDropDown.setPosition(newPos);
    }
    ui_addToPanel(&_tileSetDropDown, &_panel);

    {
	int x = _tileSetDropDown.getRect().x;
	
	_selectedTileText =
	    ui_createTextField("Selected Tile:", x, 0, COLOUR_BLACK);
	int y = _panel.panel.pos.y + _panel.getHeight() -
	    _selectedTileText.pos.h - PADDING;
	_selectedTileText.pos.y = y;
    }

    _selectedTile.pos.x = _selectedTileText.pos.x + _selectedTileText.pos.w + PADDING;
    _selectedTile.pos.y = _selectedTileText.pos.y;
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
			
	    Tile currentTile = {};
	    currentTile.sheetPos = {0, 0, tileSize, tileSize};
	    currentTile.pos = {tileSetStartX, tileSetStartY, tileSize, tileSize};
			
	    int numTilesY = _tileSets[0].imageSize.h/tileSize;
	    int numTilesX = _tileSets[0].imageSize.w/tileSize;

	    uint32 lastY = 0;
			
	    for (int i = 0; i < numTilesY; ++i)
	    {
		for (int j = 0; j < numTilesX; ++j)
		{
		    //TODO(denis): don't change the position of the next
		    // drawn tile if the tile sheet position is blank
				
		    currentTile.sheetPos.x = j * tileSize;
		    currentTile.sheetPos.y = i * tileSize;
				
		    currentTile.pos.x = tileSetStartX + ((i*numTilesX + j) % tilesPerRow)*tileSize;
		    currentTile.pos.y = tileSetStartY + ((i*numTilesX + j)/tilesPerRow)*tileSize;

		    lastY = currentTile.pos.y;

		    //TODO(denis): dunno about this tempSelectedTile thing
		    _tempSelectedTile.w = _tempSelectedTile.h = tileSize;
		    
		    if (currentTile.pos.x == _selectionBox.pos.x &&
		        currentTile.pos.y == _selectionBox.pos.y)
		    {
			_tempSelectedTile.x = currentTile.sheetPos.x;
			_tempSelectedTile.y = currentTile.sheetPos.y;
		    }
				
		    SDL_RenderCopy(_renderer, _tileSets[0].image,
				   &currentTile.sheetPos, &currentTile.pos);
		}
	    }
			
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
	_tileSetDropDown.startedClick = pointInRect(mousePos, _tileSetDropDown.getRect());
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
    else if (_selectionVisible && mouseButton == SDL_BUTTON_LEFT)
    {
	//TODO(denis): does this need to be like this?
	_selectedTile.sheetPos = _tempSelectedTile;
    }
}

void tileSetPanelInitializeNewTileSet(char *name, SDL_Rect imageRect, SDL_Texture *image,
			     uint32 tileSize)
{
    //TODO(denis): currentTileSet.name has to be freed if ever
    // the tileset is deleted
    _tileSets[_numTileSets].name = name;
    _tileSets[_numTileSets].imageSize = imageRect;
    _tileSets[_numTileSets].image = image;
    _tileSets[_numTileSets].tileSize = tileSize;
    ++_numTileSets;
    
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

bool tileSetPanelVisible()
{
    return _panel.visible;
}
