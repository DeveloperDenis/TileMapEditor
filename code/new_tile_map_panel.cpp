#include "ui_elements.h"
#include "denis_adt.h"
#include "new_tile_map_panel.h"
#include "TEMP_GeneralFunctions.cpp"

#define PANEL_PADDING 15 //in pixels
#define PANEL_COLOUR 0xFFAAAAAA
#define PANEL_MIN_WIDTH 400
#define PANEL_MIN_HEIGHT 300

#define TEXT_COLOUR COLOUR_WHITE
#define BUTTON_COLOUR 0xFF333333

static UIPanel _panel;

static bool _newTileMapClicked;

static Button _cancelButton;
static Button _createButton;

static TexturedRect _tileMapNameText;
static TexturedRect _tileSizeText;
static TexturedRect _widthText;
static TexturedRect _pixelsText;
static TexturedRect _tilesText;
static TexturedRect _heightText;
static TexturedRect _pixelsText2;
static TexturedRect _tilesText2;

static char _numberChars[] =  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0};

static EditText _tileMapNameEditText;
static EditText _tileSizeEditText;
static EditText _widthTilesEditText;
static EditText _heightTilesEditText;
static EditText _widthPxEditText;
static EditText _heightPxEditText;

static NewTileMapPanelData _data;

static void setRowPositions(int startX, int startY, int maxX, int maxY,
			    UIElement elements[], int num)
{
    int x = startX;
    for (int i = 0; i < num; ++i)
    {
	//TODO(denis): check for maxX & maxY
	if (i > 0)
	    x += elements[i-1].getWidth() + 3;
	
	int y = startY;
	elements[i].setPosition({x,y});
    }
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

void createNewTileMapPanel(int startX, int startY, int maxWidth, int maxHeight)
{
    int width = MAX(PANEL_MIN_WIDTH, maxWidth);
    int height = MAX(PANEL_MIN_HEIGHT, maxHeight);
    
    _panel = ui_createPanel(startX, startY, width, height, PANEL_COLOUR);

    //TODO(denis): use the maxWidth and maxHeight to decide the sizes for things
    int tileNameWidth = 200;
    int editTextWidth = 50;
    int buttonWidth = 100;
    int buttonHeight = 50;
    
    ui_setFont("LiberationMono-Regular.ttf", 16);
    
    //NOTE(denis): first row
    _tileMapNameText =
	ui_createTextField("Tile Map Name: ", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_tileMapNameText, &_panel);

    _tileMapNameEditText =
	ui_createEditText(0, 0, tileNameWidth, _tileMapNameText.pos.h,
			  COLOUR_WHITE, 2);
    ui_addToPanel(&_tileMapNameEditText, &_panel);
    
    //NOTE(denis): second row
    _tileSizeText =
	ui_createTextField("Tile Size: ", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_tileSizeText, &_panel);

    _tileSizeEditText =
	ui_createEditText(0, 0, editTextWidth, _tileSizeText.pos.h, COLOUR_WHITE, 2);
    _tileSizeEditText.allowedCharacters = _numberChars;
    ui_addToPanel(&_tileSizeEditText, &_panel);

    //NOTE(denis): third row
    _widthText =
	ui_createTextField("Width: ", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_widthText, &_panel);

    _widthPxEditText =
	ui_createEditText(0, 0, editTextWidth, _widthText.pos.h, COLOUR_WHITE,  2);
    _widthPxEditText.allowedCharacters = _numberChars;
    ui_addToPanel(&_widthPxEditText, &_panel);

    _pixelsText =
	ui_createTextField("pixels", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_pixelsText, &_panel);

    _widthTilesEditText =
	ui_createEditText(0, 0, editTextWidth, _widthText.pos.h, COLOUR_WHITE, 2);
    _widthTilesEditText.allowedCharacters = _numberChars;
    ui_addToPanel(&_widthTilesEditText, &_panel);

    _tilesText =
	ui_createTextField("tiles", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_tilesText, &_panel);
    
    //NOTE(denis): fourth row
    _heightText =
	ui_createTextField("Height: ", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_heightText, &_panel);

    _heightPxEditText =
	ui_createEditText(0, 0, editTextWidth, _heightText.pos.h, COLOUR_WHITE, 2);
    _heightPxEditText.allowedCharacters = _numberChars;
    ui_addToPanel(&_heightPxEditText, &_panel);
    
    _pixelsText2 =
	ui_createTextField("pixels", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_pixelsText2, &_panel);
    
    _heightTilesEditText =
	ui_createEditText(0, 0, editTextWidth, _heightText.pos.h, COLOUR_WHITE, 2);
    _heightTilesEditText.allowedCharacters = _numberChars;
    ui_addToPanel(&_heightTilesEditText, &_panel);
    
    _tilesText2 =
	ui_createTextField("tiles", 0, 0, TEXT_COLOUR);
    ui_addToPanel(&_tilesText2, &_panel);	    
    
    //NOTE(denis): fifth row
    _cancelButton = ui_createTextButton("Cancel", COLOUR_WHITE, buttonWidth,
					buttonHeight, BUTTON_COLOUR);
    ui_addToPanel(&_cancelButton, &_panel);
    
    _createButton  = ui_createTextButton("Create New Map", COLOUR_WHITE,
					 buttonWidth, buttonHeight, BUTTON_COLOUR);
    ui_addToPanel(&_createButton, &_panel);

    newTileMapPanelSetPosition({startX, startY});
}

void newTileMapPanelSetPosition(Vector2 newPos)
{
    _panel.panel.pos.x = newPos.x;
    _panel.panel.pos.y = newPos.y;
    
    int rowX = newPos.x + PANEL_PADDING;
    int rowY = newPos.y + PANEL_PADDING;
    int width = _panel.panel.pos.w;
    int height = _panel.panel.pos.h;
    
    //NOTE(denis): row 1
    UIElement row1[] = {ui_packIntoUIElement(&_tileMapNameText),
			ui_packIntoUIElement(&_tileMapNameEditText)};
    setRowPositions(rowX, rowY, width, height, row1, 2);

    //NOTE(denis): row 2
    rowY += _tileMapNameText.pos.h + PANEL_PADDING;
    UIElement row2[] = {ui_packIntoUIElement(&_tileSizeText),
			ui_packIntoUIElement(&_tileSizeEditText)};
    setRowPositions(rowX, rowY, width, height, row2, 2);

    //NOTE(denis): row 3
    rowY += _tileSizeText.pos.h + PANEL_PADDING;
    UIElement row3[] = {ui_packIntoUIElement(&_widthText),
			ui_packIntoUIElement(&_widthPxEditText),
			ui_packIntoUIElement(&_pixelsText),
			ui_packIntoUIElement(&_widthTilesEditText),
			ui_packIntoUIElement(&_tilesText)};
    setRowPositions(rowX, rowY, width, height, row3, 5);

    //NOTE(denis): row 4
    rowY += _widthText.pos.h + PANEL_PADDING;
    UIElement row4[] = {ui_packIntoUIElement(&_heightText),
			ui_packIntoUIElement(&_heightPxEditText),
			ui_packIntoUIElement(&_pixelsText2),
			ui_packIntoUIElement(&_heightTilesEditText),
			ui_packIntoUIElement(&_tilesText2)};
    setRowPositions(rowX, rowY, width, height, row4, 5);

    //NOTE(denis0: row 5
    rowY += _heightText.pos.h;
    int centreY = rowY + (height-(rowY-newPos.y))/2 - _createButton.background.pos.h/2;
    int centreX = newPos.x + width/2 - _createButton.getWidth()/2;

    _cancelButton.setPosition({centreX-_cancelButton.getWidth()/2-15, centreY});
    _createButton.setPosition({centreX+_createButton.getWidth()/2+15, centreY});
}

int newTileMapPanelGetWidth()
{
    return _panel.panel.pos.w;
}
int newTileMapPanelGetHeight()
{
    return _panel.panel.pos.h;
}

void newTileMapPanelOnMouseDown(Vector2 mousePos, uint8 mouseButton)
{
    ui_processMouseDown(&_panel, mousePos, mouseButton);
}

void newTileMapPanelOnMouseUp(Vector2 mousePos, uint8 mouseButton)
{
    ui_processMouseUp(&_panel, mousePos, mouseButton);

    _newTileMapClicked = ui_wasClicked(_createButton, mousePos);
    _panel.visible = !ui_wasClicked(_cancelButton, mousePos);
}

void newTileMapPanelSelectNext()
{
    if (_panel.panelElements)
    {
	Node *current = _panel.panelElements->front;
	
	Node *currentSelection = NULL;
	Node *nextSelection = NULL;
	
	while (current != NULL && !nextSelection)
	{
	    if (current->data.type == UI_EDITTEXT)
	    {
		if (current->data.editText->selected)
		{
		    currentSelection = current;
		}
		if (currentSelection && !current->data.editText->selected)
		{
		    nextSelection = current;
		}
	    }

	    current = current->next;
	}

	//TODO(denis): waste of computing if there is only one EditText
	if (currentSelection && !nextSelection)
	{
	    current = _panel.panelElements->front;
	    while (current != NULL && !nextSelection)
	    {
		if (current->data.type == UI_EDITTEXT &&
		    !current->data.editText->selected)
		{
		    nextSelection = current;
		}

		current = current->next;
	    }
	}

	if (nextSelection)
	{
	    currentSelection->data.editText->selected = false;
	    nextSelection->data.editText->selected = true;
	}
    }
}

void newTileMapPanelSelectPrevious()
{
    if (_panel.panelElements)
    {
	Node *current = _panel.panelElements->front;

	Node *currentSelection = NULL;
	Node *previousSelection = NULL;

	bool previousFound = false;

	while (current != NULL && !previousFound)
	{
	    if (current->data.type == UI_EDITTEXT)
	    {
		if (current->data.editText->selected)
		{
		    currentSelection = current;

		    if (previousSelection)
			previousFound = true;
		}
		else if (!currentSelection)
		{
		    previousSelection = current;
		}

		if (currentSelection && !previousFound)
		    previousSelection = current;
	    }

	    current = current->next;
	}

	if (previousSelection && currentSelection)
	{
	    currentSelection->data.editText->selected = false;
	    previousSelection->data.editText->selected = true;
	}
    }
}

void newTileMapPanelEnterPressed()
{
    _newTileMapClicked = true;

    //TODO(denis): dunno if this is needed
    if (!newTileMapPanelDataReady())
	_newTileMapClicked = false;
}

void newTileMapPanelCharInput(char c)
{
    ui_processLetterTyped(c, &_panel);
			    
    int tileSize = convertStringToInt(_tileSizeEditText.text, _tileSizeEditText.letterCount);
			    
    if (_tileSizeEditText.selected)
    {
	fillBWithAConverted(1.0F/(float)tileSize, &_widthPxEditText, &_widthTilesEditText);
	fillBWithAConverted(1.0F/(float)tileSize, &_heightPxEditText, &_heightTilesEditText);
    }
    else if (_tileSizeEditText.text[0] != 0)
    {
	if (_widthTilesEditText.text[0] != 0 ||
	    _widthPxEditText.text[0] != 0)
	{
	    if (_widthTilesEditText.selected)
	    {
		fillBWithAConverted((float)tileSize, &_widthTilesEditText, &_widthPxEditText);
	    }
	    else if (_widthPxEditText.selected)
	    {
		fillBWithAConverted(1.0F/(float)tileSize, &_widthPxEditText, &_widthTilesEditText);
	    }
	}
	if (_heightTilesEditText.text[0]!=0 ||
	    _heightPxEditText.text[0] != 0)
	{
	    if (_heightTilesEditText.selected)
	    {
		fillBWithAConverted((float)tileSize, &_heightTilesEditText, &_heightPxEditText);
	    }
	    else if (_heightPxEditText.selected)
	    {
		fillBWithAConverted(1.0F/(float)tileSize, &_heightPxEditText, &_heightTilesEditText);
	    }
	}
    }
}

//TODO(denis): this and the above function has some pretty similar code
// try to consolodate
void newTileMapPanelCharDeleted()
{
    ui_eraseLetter(&_panel);

    int tileSize = convertStringToInt(_tileSizeEditText.text, _tileSizeEditText.letterCount);

    if (tileSize != 0)
    {
	if (_tileSizeEditText.selected)
	{
	    fillBWithAConverted(1.0F/(float)tileSize, &_widthPxEditText, &_widthTilesEditText);
	    fillBWithAConverted(1.0F/(float)tileSize, &_heightPxEditText, &_heightTilesEditText);
	}
	else if (_widthTilesEditText.selected)
	{
	    fillBWithAConverted((float)tileSize, &_widthTilesEditText, &_widthPxEditText);
	}
	else if (_widthPxEditText.selected)
	{
	    fillBWithAConverted(1.0F/(float)tileSize, &_widthPxEditText, &_widthTilesEditText);
	}
	else if (_heightTilesEditText.selected)
	{
	    fillBWithAConverted((float)tileSize, &_heightTilesEditText, &_heightPxEditText);
	}
	else if (_heightPxEditText.selected)
	{
	    fillBWithAConverted(1.0F/(float)tileSize, &_heightPxEditText, &_heightTilesEditText);
	}
    }
}

bool newTileMapPanelVisible()
{
    return _panel.visible;
}

void newTileMapPanelSetVisible(bool newValue)
{
    _panel.visible = newValue;

    if (newValue = false)
	_newTileMapClicked = false;
}

bool newTileMapPanelDataReady()
{
    bool result = false;
    
    char *tileMapName = _tileMapNameEditText.text;

    int tileSize = (uint32)convertStringToInt(_tileSizeEditText.text, _tileSizeEditText.letterCount);
    int widthInTiles = (uint32)convertStringToInt(_widthTilesEditText.text, _widthTilesEditText.letterCount);
    int heightInTiles = (uint32)convertStringToInt(_heightTilesEditText.text, _heightTilesEditText.letterCount);
				
    result = (tileMapName[0] != 0 && tileSize != 0 &&
	      widthInTiles != 0 && heightInTiles != 0 &&
	      _newTileMapClicked);

    if (result)
    {
	_data.tileMapName = tileMapName;
	_data.tileSize = tileSize;
	_data.widthInTiles = widthInTiles;
	_data.heightInTiles = heightInTiles;
    }

    return result;
}

NewTileMapPanelData *newTileMapPanelGetData()
{
    if (_data.tileMapName[0] != 0 && _data.tileSize != 0 &&
	_data.widthInTiles != 0 && _data.heightInTiles != 0)
	return &_data;
    else
	return 0;
}

void newTileMapPanelDraw()
{
    ui_draw(&_panel);
}
