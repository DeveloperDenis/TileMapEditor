#include "ui_elements.h"
#include "import_tile_set_panel.h"
#include "tile_set_panel.h"
#include "file_saving_loading.h"
#include "TEMP_GeneralFunctions.cpp"

#define MIN_WIDTH 950
#define MIN_HEIGHT 150
#define PADDING 15

#define PANEL_COLOUR 0xFF222222
#define BUTTON_COLOUR 0xFF000000

static SDL_Renderer *_renderer;

static UIPanel _panel;

static TexturedRect _tileSizeText;
static EditText _tileSizeEditText;
static TexturedRect _tileSheetText;
static EditText _tileSheetEditText;
static Button _browseTileSetButton;
static Button _cancelButton;
static Button _openButton;
static TexturedRect _warningText;

void importTileSetPanelCreateNew(SDL_Renderer *renderer, int32 x, int32 y,
				 int32 width, int32 height)
{
    _renderer = renderer;
    
    width = MAX(MIN_WIDTH, width);
    height = MAX(MIN_HEIGHT, height);
    
    _panel = ui_createPanel(x, y, width, height, PANEL_COLOUR);
    _panel.visible = false;
    
    _tileSizeText = ui_createTextField("Tile Size:", _panel.panel.pos.x + PADDING,
				       _panel.panel.pos.y + PADDING, COLOUR_WHITE);
    ui_addToPanel(&_tileSizeText, &_panel);

    _tileSizeEditText =
	ui_createEditText(_tileSizeText.pos.x + _tileSizeText.getWidth() + PADDING,
			  _tileSizeText.pos.y, 100, _tileSizeText.getHeight(),
			  COLOUR_WHITE, 5);
    char chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0 };
    _tileSizeEditText.allowedCharacters = chars;
    ui_addToPanel(&_tileSizeEditText, &_panel);

    _tileSheetText = ui_createTextField("Tile Sheet:", _tileSizeText.pos.x,
					_tileSizeText.pos.y + _tileSizeText.pos.h + 5,
					COLOUR_WHITE);
    ui_addToPanel(&_tileSheetText, &_panel);

    _tileSheetEditText =
	ui_createEditText(_tileSheetText.pos.x + _tileSheetText.getWidth() + PADDING,
			  _tileSheetText.pos.y, _panel.getWidth() - _tileSheetText.getWidth() - PADDING*3,
			  _tileSheetText.getHeight(),
			  COLOUR_WHITE, 5);
    ui_addToPanel(&_tileSheetEditText, &_panel);

    _browseTileSetButton =
	ui_createTextButton("Browse", COLOUR_WHITE, 0, 0, BUTTON_COLOUR);
    {
	int x = _tileSheetEditText.pos.x + _tileSheetEditText.getWidth()/2 - _browseTileSetButton.getWidth()/2;
	int y = _tileSheetEditText.pos.y + _browseTileSetButton.getHeight() + 5;
	_browseTileSetButton.setPosition({x,y});
    }
    ui_addToPanel(&_browseTileSetButton, &_panel);

    _cancelButton = ui_createTextButton("Cancel", COLOUR_WHITE, 0, 0, BUTTON_COLOUR);
    _openButton = ui_createTextButton("Open", COLOUR_WHITE, 0, 0, BUTTON_COLOUR);
    {
	int x = _panel.panel.pos.x + PADDING;
	int y = _browseTileSetButton.background.pos.y + PADDING;
	_cancelButton.setPosition({x,y});
	x += _cancelButton.getWidth() + PADDING;
	_openButton.setPosition({x,y});
    }
    ui_addToPanel(&_cancelButton, &_panel);
    ui_addToPanel(&_openButton, &_panel);

    ui_addToPanel(&_warningText, &_panel);
}

void importTileSetPanelOnMouseDown(Vector2 mousePos, uint8 mouseButton)
{
    ui_processMouseDown(&_panel, mousePos, mouseButton);
}

void importTileSetPanelOnMouseUp(Vector2 mousePos, uint8 mouseButton)
{
    ui_processMouseUp(&_panel, mousePos, mouseButton);

    if (ui_wasClicked(_cancelButton, mousePos))
    {
	_panel.visible = false;
	//TODO(denis): reset import tile set panel values
	// maybe save the value in the tile size field?
    }
    else if (ui_wasClicked(_openButton, mousePos))
    {
	int tileSize = convertStringToInt(_tileSizeEditText.text,
					  _tileSizeEditText.letterCount);
				    
	char *warning = "";
	if (tileSize == 0)
	{
	    warning = "tile size must be non-zero";
	}
	else if (_tileSheetEditText.letterCount == 0)
	{
	    warning = "no tile sheet file selected";
	}
	else
	{
	    TexturedRect newTileSheet = loadImage(_renderer, _tileSheetEditText.text);

	    if (newTileSheet.image != 0)
	    {
		char *fileName = _tileSheetEditText.text;
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
					    
		_panel.visible = false;
	    }
	    else
	    {
		warning = "Error opening file";
	    }
	}
	_warningText = ui_createTextField(warning, _cancelButton.background.pos.x,
					 _cancelButton.background.pos.y + _cancelButton.getHeight() + 15, COLOUR_RED);
    }
    else if (ui_wasClicked(_browseTileSetButton, mousePos))
    {
	char *fileName = getTileSheetFileName();

	if (fileName != 0)
	{
	    while(_tileSheetEditText.letterCount > 0)
		ui_eraseLetter(&_tileSheetEditText);

	    int i = 0;
	    for (i = 0; fileName[i] != 0; ++i)
	    {
		ui_addLetterTo(&_tileSheetEditText, fileName[i]);
	    }
	}

	HeapFree(GetProcessHeap(), 0, fileName);
    }
}

void importTileSetPanelCharInput(char c)
{
    ui_processLetterTyped(c, &_panel);
}

void importTileSetPanelCharDeleted()
{
    ui_eraseLetter(&_panel);
}

void importTileSetPanelDraw()
{
    if (_panel.visible)
	ui_draw(&_panel);
}

bool importTileSetPanelVisible()
{
    return _panel.visible;
}

void importTileSetPanelSetVisible(bool newValue)
{
    _panel.visible = newValue;
}

void importTileSetPanelSetTileSize(int32 newSize)
{
    char *tileSizeString = convertIntToString(newSize);
    ui_setText(&_tileSizeEditText, tileSizeString);

    HEAP_FREE(tileSizeString);
}
