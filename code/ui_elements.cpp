#include "main.h"
#include "ui_elements.h"
#include "SDL_render.h"
#include "TEMP_GeneralFunctions.cpp"

#include "denis_adt.h"

int UIElement::getWidth()
{
    int width = 0;
    
    switch(this->type)
    {
	case UI_TEXTFIELD:
	    width = this->textField->getWidth();
	    break;

	case UI_EDITTEXT:
	    width = this->editText->getWidth();
	    break;

	case UI_BUTTON:
	    width = this->button->getWidth();
	    break;

	case UI_TEXTBOX:
	    width = this->textBox->getWidth();
	    break;

	case UI_DROPDOWNMENU:
	    width = this->dropDownMenu->getWidth();
	    break;
    }

    return width;
}

void UIElement::setPosition(Vector2 newPos)
{
    switch(this->type)
    {
	case UI_TEXTFIELD:
	    this->textField->setPosition(newPos);
	    break;

	case UI_EDITTEXT:
	    this->editText->setPosition(newPos);
	    break;

	case UI_BUTTON:
	    this->button->setPosition(newPos);
	    break;

	case UI_TEXTBOX:
	    this->textBox->setPosition(newPos);
	    break;

	case UI_DROPDOWNMENU:
	    this->dropDownMenu->setPosition(newPos);
	    break;
    }
}

void EditText::setPosition(Vector2 newPos)
{
    this->pos.x = newPos.x;
    this->pos.y = newPos.y;
    
    for (int i = 0; i < this->letterCount; ++i)
    {
	int x = newPos.x + i*this->letters[0].pos.w;
	int y = newPos.y;
	this->letters[i].setPosition({x,y});
    }
}

void Button::setPosition(Vector2 newPos)
{
    this->background.pos.x = newPos.x;
    this->background.pos.y = newPos.y;

    if (this->foreground.image)
    {
	int x = newPos.x + this->background.pos.w/2 - this->foreground.pos.w/2;
	int y = newPos.y + this->background.pos.h/2 - this->foreground.pos.h/2;
	
	this->foreground.pos.x = x;
	this->foreground.pos.y = y;
    }
}

void Button::destroy()
{
    if (this->background.image)
	SDL_DestroyTexture(this->background.image);

    if (this->foreground.image)
    	SDL_DestroyTexture(this->foreground.image);

    this->text = NULL;
    this->startedClick = false;
    this->background = {};
    this->foreground = {};
}

void TextBox::setPosition(Vector2 newPos)
{
    this->pos.x = newPos.x;
    this->pos.y = newPos.y;

    //TODO(denis): doesn't take into account any text padding
    this->text.pos.x = newPos.x;
    this->text.pos.y = newPos.y;
}

SDL_Rect DropDownMenu::getRect()
{
    SDL_Rect result = {};
    result.x = this->items[0].pos.x;
    result.y = this->items[0].pos.y;
    result.w = this->items[0].pos.w;
    result.h = this->items[0].pos.h;
    
    if (this->isOpen)
    {
	result.h *= this->itemCount;
    }
    
    return result;
}

void DropDownMenu::setPosition(Vector2 newPos)
{
    this->items[0].pos.x = newPos.x;
    this->items[0].pos.y = newPos.y;
}

void MenuBar::addMenu(char *items[], int numItems, int menuWidth)
{
    int height = this->botRight.y - this->topLeft.y;
    SDL_Color textColour = hexColourToRGBA(this->textColour);
    
    this->menus[this->menuCount] = ui_createDropDownMenu(items, numItems, menuWidth,
							 height, textColour,
							 this->colour);
    int x = this->topLeft.x;
    if (this->menuCount > 0)
	x += this->menuCount*this->menus[this->menuCount-1].items[0].pos.w;
    
    this->menus[this->menuCount].items[0].setPosition({x, 0});
    
    ++this->menuCount;
}

void MenuBar::onMouseMove(Vector2 mousePos)
{
    for (int i = 0; i < this->menuCount; ++i)
    {
	if (pointInRect(mousePos, this->menus[i].getRect()))
	{
	    SDL_Rect tempRect = this->menus[i].getRect();
	    int selectedY = (mousePos.y - tempRect.y)/this->menus[i].items[0].pos.h;
	    this->menus[i].highlightedItem = selectedY;
	}
	else
	    this->menus[i].highlightedItem = -1;
    }
}

void MenuBar::onMouseDown(Vector2 mousePos, Uint8 button)
{   
    for (int i = 0; i < this->menuCount; ++i)
    {
	if (button == SDL_BUTTON_LEFT)
	    this->menus[i].startedClick =
		pointInRect(mousePos, this->menus[i].getRect());
    }
}

bool MenuBar::onMouseUp(Vector2 mousePos, Uint8 button)
{
    bool processed = false;
    
    for (int i = 0; i < this->menuCount; ++i)
    {
	if (button == SDL_BUTTON_LEFT)
	{
	    bool menuClicked = pointInRect(mousePos, this->menus[i].getRect());
	    DropDownMenu *menu = &this->menus[i];
	
	    if (menu->isOpen && menu->startedClick && menuClicked)
	    {
		SDL_Rect tempRect = menu->getRect();
		int selectedY = (mousePos.y - tempRect.y)/menu->items[0].pos.h;
		if (selectedY == 0)
		{
		    menu->isOpen = false;
		    menu->startedClick = false;
		}
		processed = true;
	    }
	    else if (!menu->isOpen && menu->startedClick && menuClicked)
	    {
		menu->isOpen = true;
		processed = true;
	    }
	    else if (menu->isOpen)
	    {
		menu->isOpen = false;
		processed = true;
	    }
	}
    }

    return processed;
}

#include "SDL_ttf.h"
/* TODO(denis):
 *   to dos:
 *     -have a "flush fonts" function that deletes any fonts currently open to prevent
 *      clutter (or maybe dynamically decide when to delete fonts by keeping  counter
 *      of how many calls since they were used?)
 */

#define MAX_FONT_NUM 15

static SDL_Renderer *_renderer;

static TTF_Font *_fonts[MAX_FONT_NUM];
static int _fontsLength;
static int _selectedFont;

static TextCursor _cursor;

static void ui_delete(LinkedList *ll)
{
    Node *current = ll->front;

    while (current != NULL)
    {
	//TODO(denis): put this in a ui_delete(UIElement *) function?
	switch(current->data.type)
	{
	    case UI_TEXTFIELD:
		ui_delete(current->data.textField);
		break;

	    case UI_EDITTEXT:
		ui_delete(current->data.editText);
		break;

	    case UI_BUTTON:
		ui_delete(current->data.button);
		break;

	    case UI_TEXTBOX:
		ui_delete(current->data.textBox);
		break;

	    case UI_DROPDOWNMENU:
		ui_delete(current->data.dropDownMenu);
		break;
	}

	Node *next = current->next;
	free(current);
	current = next;
    }

    ll->front = 0;
}

static void resetCursorPosition(EditText *editText)
{
    _cursor.pos.y = editText->pos.y + editText->padding/2;
    _cursor.pos.x = editText->padding + editText->pos.x + editText->letterCount*editText->letters[0].pos.w;

    if (_cursor.pos.x > editText->pos.x + editText->pos.w)
	_cursor.pos.x -= editText->letters[editText->letterCount-1].pos.w;
}

//NOTE(denis): only one EditText can be selected per panel
static EditText* getSelectedEditText(UIPanel *panel)
{
    EditText *result = NULL;
    Node *current = panel->panelElements->front;
    
    while (current != NULL && !result)
    {
	if (current->data.type == UI_EDITTEXT)
	{
	    EditText *editText = current->data.editText;
	    result = editText->selected ? editText : NULL;
	}

	current = current->next;
    }    

    return result;
}

//NOTE(denis): assumes all letters are roughly the same width
bool editTextCanHoldLetter(EditText *editText)
{   
    int textWidth = editText->pos.w;
    int textX = editText->pos.x;
    int letterCount = editText->letterCount;
    int letterWidth = editText->letters[0].pos.w;

    return textX + letterCount*letterWidth < textX + textWidth;
}

//TODO(denis): split this into an init and a addFont or something
bool ui_init(SDL_Renderer *renderer, char *fontName, int fontSize)
{
    bool result = false;

    if (TTF_Init() == 0 &&
	(_fonts[_fontsLength++] = TTF_OpenFont(fontName, fontSize)))
    {	
        _renderer = renderer;
	    
        _cursor.pos.w = 5;
        _cursor.pos.h = 15;
        _cursor.flashRate = 300;
        _cursor.visible = true;
	
	result = true;
    }

    return result;
}

void ui_destroy()
{
    for (int i = 0; i < _fontsLength; ++i)
    {
	TTF_CloseFont(_fonts[i]);
    }
    _fontsLength = 0;
    
    TTF_Quit();
}

bool ui_setFont(char *fontName, int fontSize)
{
    bool result = false;
    
    for (int i = 0; i < _fontsLength; ++i)
    {
	char *currFontName = TTF_FontFaceFamilyName(_fonts[i]);
	int currFontSize = TTF_FontHeight(_fonts[i]);

	if (currFontSize == fontSize &&
	    stringsEqual(fontName, currFontName))
	{
	    _selectedFont = i;
	    result = true;
	}
    }

    if (!result)
    {
	_fonts[_fontsLength] = TTF_OpenFont(fontName, fontSize);
	if (_fonts[_fontsLength] != NULL)
	{
	    result = true;
	    _selectedFont = _fontsLength;
	    ++_fontsLength;
	}
    }

    return result;
}

bool ui_wasClicked(Button button, Vector2 mouse)
{
    return pointInRect(mouse, button.background.pos) && button.startedClick;
}

bool ui_processMouseDown(UIPanel *panel, Vector2 mousePos, Uint8 button)
{
    bool processed = false;
    
    if (panel && panel->panelElements)
    {
	Node *current = panel->panelElements->front;

	while (current != NULL)
	{
	    if (button == SDL_BUTTON_LEFT)
	    {
		if (current->data.type == UI_BUTTON)
		{
		    Button *data = current->data.button;
		    data->startedClick = pointInRect(mousePos, data->background.pos);

		    if (data->startedClick)
			processed = true;
		}
	    }
	    
	    current = current->next;
	}
    }

    return processed;
}

bool ui_processMouseUp(UIPanel *panel, Vector2 mousePos, Uint8 button)
{
    bool processed = false;
    
    if (panel && panel->panelElements)
    {
	Node *current = panel->panelElements->front;

	while (current != 0)
	{
	    if (button == SDL_BUTTON_LEFT)
	    {
		if (current->data.type == UI_EDITTEXT)
		{
		    EditText *data = current->data.editText;
		    data->selected = pointInRect(mousePos, data->pos);

		    if (data->selected)
		    {
			resetCursorPosition(data);
			_cursor.flashCounter = 0;
			processed = true;
		    }
		}
	    }

	    current = current->next;
	}
    }

    return processed;
}

void ui_addLetterTo(EditText *editText, char c)
{
    if ( (editText->allowedCharacters == NULL ||
	  charInArray(c, editText->allowedCharacters)) &&
	 editTextCanHoldLetter(editText))
    {
	char letters[] = {c, 0};
	int charX = editText->pos.x +
	    editText->letterCount*editText->letters[0].pos.w;

	//TODO(denis): give the edit text a colour property?
	TexturedRect letterText =
	    ui_createTextField(letters, charX, editText->pos.y,
			       COLOUR_BLACK);

        editText->text[editText->letterCount] = c;
        editText->letters[editText->letterCount++] = letterText;
    }
}

void ui_processLetterTyped(char c, UIPanel *panel)
{
    EditText *editText = getSelectedEditText(panel);

    if (editText)
	ui_addLetterTo(editText, c);
}

void ui_eraseLetter(EditText *editText)
{
    if (editText->letterCount > 0)
    {
	--(editText->letterCount);

        editText->text[editText->letterCount] = 0;
	SDL_DestroyTexture(editText->letters[editText->letterCount].image);
    }
}

void ui_eraseLetter(UIPanel *panel)
{
    EditText *editText = getSelectedEditText(panel);
    
    if (editText)
	ui_eraseLetter(editText);
}

static void addToPanel(UIElement data, UIPanel *panel)
{
    if (!panel->panelElements)
    {
	panel->panelElements = (LinkedList *)malloc(sizeof(panel->panelElements));
	panel->panelElements->front = 0;
    }
	
    adt_addTo(panel->panelElements, data);
}

UIPanel ui_addToPanel(EditText *editText)
{
    UIPanel newPanel = {};
    ui_addToPanel(editText, &newPanel);
    
    return newPanel;
}

void ui_addToPanel(EditText *editText, UIPanel *panel)
{
    UIElement data = {};
    data.type = UI_EDITTEXT;
    data.editText = editText;

    addToPanel(data, panel);
}

UIPanel ui_addToPanel(TexturedRect *textField)
{
    UIPanel newPanel = {};
    ui_addToPanel(textField, &newPanel);

    return newPanel;
}
void ui_addToPanel(TexturedRect *textField, UIPanel *panel)
{
    UIElement data = {};
    data.type = UI_TEXTFIELD;
    data.textField = textField;

    addToPanel(data, panel);
}

UIPanel ui_addToPanel(Button *button)
{
    UIPanel newPanel = {};
    ui_addToPanel(button, &newPanel);
    
    return newPanel;
}
void ui_addToPanel(Button *button, UIPanel *panel)
{
    UIElement data = {};
    data.type = UI_BUTTON;
    data.button = button;

    addToPanel(data, panel);
}

UIPanel ui_addToPanel(TextBox *textBox)
{
    UIPanel newPanel = {};
    ui_addToPanel(textBox, &newPanel);
    
    return newPanel;
}
void ui_addToPanel(TextBox *textBox, UIPanel *panel)
{
    UIElement data = {};
    data.type = UI_TEXTBOX;
    data.textBox = textBox;

    addToPanel(data, panel);
}

void ui_delete(UIPanel *panel)
{
    ui_delete(panel->panelElements);
    free(panel->panelElements);
    panel->panelElements = 0;
}

void ui_delete(TexturedRect *texturedRect)
{
    SDL_DestroyTexture(texturedRect->image);
    texturedRect->image = NULL;
    texturedRect->pos = {};
}

void ui_delete(EditText *editText)
{
    for (int i = 0; i < editText->letterCount; ++i)
    {
	ui_delete(&editText->letters[i]);
    }

    editText->letterCount = 0;
}

void ui_delete(Button *button)
{
    button->destroy();
}

void ui_delete(TextBox *textBox)
{
    SDL_DestroyTexture(textBox->background);
    ui_delete(&textBox->text);

    textBox->background = 0;
    textBox->text = {};
    textBox->pos = {};
}

void ui_delete(DropDownMenu *dropDownMenu)
{
    for (int i = 0; i < dropDownMenu->itemCount; ++i)
    {
	ui_delete(&dropDownMenu->items[i]);
    }

    SDL_DestroyTexture(dropDownMenu->unselectedTexture);
    SDL_DestroyTexture(dropDownMenu->selectionBox);

    dropDownMenu->itemCount = 0;
    dropDownMenu->isOpen = false;
    dropDownMenu->startedClick = false;
}

//TODO(denis): perhaps have multiple versions that do a different kind of
// TTF_RenderText, like Shaded and Solid
//TODO(denis): exchange everything to 0xAABBGGRR ? dunno
// or have a setTextColour method instead of passing SDL_Color around
// have a colour bitfield?
TexturedRect ui_createTextField(char *text, int x, int y, SDL_Color colour)
{
    TexturedRect result = {};
    
    SDL_Surface* tempSurf = TTF_RenderText_Blended(_fonts[_selectedFont], text, colour);

    SDL_GetClipRect(tempSurf, &result.pos);
    result.pos.x = x;
    result.pos.y = y;

    result.image = SDL_CreateTextureFromSurface(_renderer, tempSurf);
    SDL_FreeSurface(tempSurf);

    return result;
}

//TODO(denis): really don't like this mismatch of SDL_Color and Uint32
TextBox ui_createTextBox(char *text, int minWidth, int minHeight,
			 SDL_Color textColour, Uint32 backgroundColour)
{
    TextBox result = {};

    result.text = ui_createTextField(text, 0, 0, textColour);

    int backgroundWidth = minWidth > result.text.pos.w ? minWidth : result.text.pos.w;
    int backgroundHeight = minHeight > result.text.pos.h ? minHeight : result.text.pos.h;
    TexturedRect temp =
	createFilledTexturedRect(_renderer, backgroundWidth,
				 backgroundHeight, backgroundColour);
    result.background = temp.image;
    result.pos = temp.pos;
    
    return result;
}

//TODO(denis): have a function with a "default text" parameter
EditText ui_createEditText(int x, int y, int width, int height,
			   SDL_Color backgroundColour, int padding)
{
    EditText result = {};
    result.backgroundColour = backgroundColour;
    result.pos = {x, y, width, height};

    //TODO(denis): actually use padding for stuff
    result.padding = padding;
    
    return result;
}

Button ui_createTextButton(char *text, SDL_Colour textColour,
			   int width, int height, Uint32 backgroundColour)
{
    Button result = {};

    result.text = text;
    result.foreground = ui_createTextField(text, 0, 0, textColour);
    
    int bgWidth = max(width, result.foreground.pos.w) + 5;
    int bgHeight = max(height, result.foreground.pos.h) + 5;
    result.background = createFilledTexturedRect(_renderer, bgWidth, bgHeight,
						 backgroundColour);

    result.setPosition({0,0});
    
    return result;
}

Button ui_createImageButton(char *imageFileName)
{
    Button result = {};
    
    result.background = loadImage(_renderer, imageFileName);
    
    return result;
}

//TODO(denis): define some function/macro or something to simplify this colour nonsense
DropDownMenu ui_createDropDownMenu(char *items[], int itemNum,
				   int itemWidth, int itemHeight,
				   SDL_Color textColour, Uint32 backgroundColour)
{
    DropDownMenu result = {};
    //TODO(denis): don't hardcode selection colour
    Uint32 selectionColour = 0xFF555555;
    result.highlightedItem = -1;
    result.selectionBox = createFilledTexturedRect(_renderer, itemWidth, itemHeight,
						   selectionColour).image;
    
    //TODO(denis): since result.items is only length 10 this could cause issues
    result.itemCount = itemNum;
    
    for (int i = 0; i < result.itemCount; ++i)
    {
	result.items[i] = ui_createTextBox(items[i], itemWidth, itemHeight,
					   textColour, backgroundColour);
	result.unselectedTexture = result.items[i].background;
    }

    return result;
}

MenuBar ui_createMenuBar(int x, int y, int width, int height, Uint32 backgroundColour,
			 Uint32 textColour)
{
    MenuBar result = {};
    result.topLeft.x = x;
    result.topLeft.y = y;
    result.botRight.x = x+width;
    result.botRight.y = y+height;
    result.colour = backgroundColour;
    result.textColour = textColour;
    
    return result;
}

UIPanel ui_createPanel(int x, int y, int width, int height, Uint32 colour)
{
    UIPanel result = {};
    result.visible = true;
    
    result.panel = createFilledTexturedRect(_renderer, width, height, colour);

    result.panel.pos.x = x;
    result.panel.pos.y = y;

    return result;
}

UIElement ui_packIntoUIElement(EditText *editText)
{
    UIElement result = {};
    result.type = UI_EDITTEXT;
    result.editText = editText;
    return result;
}
UIElement ui_packIntoUIElement(TexturedRect *texturedRect)
{
    UIElement result = {};
    result.type = UI_TEXTFIELD;
    result.textField = texturedRect;
    return result;
}
UIElement ui_packIntoUIElement(Button *button)
{
    UIElement result = {};
    result.type = UI_BUTTON;
    result.button = button;
    return result;
}
UIElement ui_packIntoUIElement(TextBox *textBox)
{
    UIElement result = {};
    result.type = UI_TEXTBOX;
    result.textBox = textBox;
    return result;
}
UIElement ui_packIntoUIElement(DropDownMenu *dropDownMenu)
{
    UIElement result = {};
    result.type = UI_DROPDOWNMENU;
    result.dropDownMenu = dropDownMenu;
    return result;
}

// NOTE(denis): all the drawing functions

void ui_draw(UIPanel *panel)
{
    bool editTextSelected = false;

    if (panel->visible)
    {
	if (panel->panel.image)
	{
	    SDL_RenderCopy(_renderer, panel->panel.image, NULL, &panel->panel.pos);
	}
	
	if (panel && panel->panelElements)
	{
	    Node *current = panel->panelElements->front;
    
	    while (current != NULL)
	    {
		switch (current->data.type)
		{
		    case UI_TEXTFIELD:
			ui_draw(current->data.textField);
			break;

		    case UI_EDITTEXT:
			if (current->data.editText->selected)
			{
			    editTextSelected = true;
			    resetCursorPosition(current->data.editText);
			}
		    
			ui_draw(current->data.editText);
			break;

		    case UI_TEXTBOX:
			ui_draw(current->data.textBox);
			break;
		    
		    case UI_BUTTON:
			ui_draw(current->data.button);
			break;

		    case UI_DROPDOWNMENU:
			ui_draw(current->data.dropDownMenu);
			break;
		}

		current = current->next;
	    }
	}

	if (editTextSelected)
	{
	    if (_cursor.flashCounter >= _cursor.flashRate)
	    {
		//TODO(denis): don't hardcode this, probably
		SDL_SetRenderDrawColor(_renderer, 140,140,140,255);
		SDL_RenderFillRect(_renderer, &_cursor.pos);
		_cursor.visible = false;
	    }
	    if (!_cursor.visible && _cursor.flashCounter >= _cursor.flashRate*2)
	    {
		_cursor.visible = true;
		_cursor.flashCounter = 0;
	    }
	    ++_cursor.flashCounter;
	}
    }
}

void ui_draw(Button *button)
{
    if (button->background.image)
	SDL_RenderCopy(_renderer, button->background.image, NULL, &button->background.pos);

    if (button->text && button->foreground.image)
    {
	SDL_RenderCopy(_renderer, button->foreground.image, NULL, &button->foreground.pos);
    }
}

void ui_draw(TexturedRect *texturedRect)
{
    if (texturedRect && texturedRect->image)
	SDL_RenderCopy(_renderer, texturedRect->image, NULL, &texturedRect->pos);
}

void ui_draw(EditText *editText)
{
    if (editText)
    {
	SDL_Color c = editText->backgroundColour;
	SDL_SetRenderDrawColor(_renderer, c.r, c.g, c.b, c.a);
	SDL_RenderFillRect(_renderer, &editText->pos);

	for (int i = 0; i < editText->letterCount; ++i)
	{
	    SDL_RenderCopy(_renderer, editText->letters[i].image,
			   NULL, &editText->letters[i].pos);
	}
    }
}

void ui_draw(TextBox *textBox)
{
    if (textBox)
    {
	if (textBox->background)
	    SDL_RenderCopy(_renderer, textBox->background, NULL, &textBox->pos);
	
	if (textBox->text.image)
	    SDL_RenderCopy(_renderer, textBox->text.image, NULL, &textBox->text.pos);
    }
}

void ui_draw(DropDownMenu *dropDownMenu)
{
    if (dropDownMenu->itemCount > 0)
    {
	int x = dropDownMenu->items[0].pos.x;
	int y = dropDownMenu->items[0].pos.y;
	int itemHeight = dropDownMenu->items[0].pos.h;

	if (dropDownMenu->isOpen)
	{
	    dropDownMenu->items[0].background = dropDownMenu->selectionBox;
	    
	    for (int i = 0; i < dropDownMenu->itemCount; ++i)
	    {
		if (dropDownMenu->items[i].background != NULL)
		{
		    if (i == dropDownMenu->highlightedItem && i!=0)
		    {
			dropDownMenu->items[i].background = dropDownMenu->selectionBox;
		    }
		    else if (i != 0)
			dropDownMenu->items[i].background = dropDownMenu->unselectedTexture;
		    
		    dropDownMenu->items[i].setPosition({x, y + i*itemHeight});
		    
		    ui_draw(&dropDownMenu->items[i]);
		}
	    }
	}
	else
	{
	    if (dropDownMenu->highlightedItem == 0)
	    {
		dropDownMenu->items[0].background = dropDownMenu->selectionBox;
	    }
	    else
	    {
		dropDownMenu->items[0].background = dropDownMenu->unselectedTexture;
	    }
	    ui_draw(&dropDownMenu->items[0]);
	}
    }
}

void ui_draw(MenuBar *menuBar)
{
    int menuWidth = menuBar->botRight.x - menuBar->topLeft.x;
    int menuHeight = menuBar->botRight.y - menuBar->topLeft.y;
    
    SDL_Color colour = hexColourToRGBA(menuBar->colour);
    SDL_SetRenderDrawColor(_renderer, colour.r, colour.g, colour.b, colour.a);
    SDL_Rect rect = {menuBar->topLeft.x, menuBar->topLeft.y, menuWidth, menuHeight};
    SDL_RenderFillRect(_renderer, &rect);
    
    for (int i = 0; i < menuBar->menuCount; ++i)
    {
	ui_draw(&menuBar->menus[i]);
    }
}