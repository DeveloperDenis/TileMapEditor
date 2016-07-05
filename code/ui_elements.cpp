#include "main.h"
#include "ui_elements.h"
#include "SDL_render.h"
#include "TEMP_GeneralFunctions.cpp"

#include "denis_adt.h"

void Button::setPosition(Vector2 newPos)
{
    this->background.pos.x = newPos.x;
    this->background.pos.y = newPos.y;

    //TODO(denis): if centre/right/left aligned I would want to change this
    this->foreground.pos.x = newPos.x;
    this->foreground.pos.y = newPos.y;
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

#include "SDL_ttf.h"
/* TODO(denis):
 *   to dos:
 *     -potentially have multiple different fonts open and render for rendering
 *      and when you call a draw with a different font size or font name than is saved
 *      a new font is dynamically created and saved for later
 *     -have a "flush fonts" function that deletes any fonts currently open to prevent
 *      clutter (or maybe dynamically decide when to delete fonts by keeping  counter
 *      of how many calls since they were used?)
 */

static SDL_Renderer *_renderer;
static TTF_Font *_font;
static TextCursor _cursor;

static LinkedList _groups;
static int _currentID;

static void ui_delete(LinkedList *ll)
{
    Node *current = ll->front;

    while (current != NULL)
    {
	//TODO(denis): put this in a ui_delete(UIElement *) function?
	switch(current->data.type)
	{
	    case UI_LINKEDLIST:
		ui_delete(current->data.ll);
		break;

	    case UI_TEXTFIELD:
		ui_delete(current->data.textField);
		break;

	    case UI_EDITTEXT:
		ui_delete(current->data.editText);
		break;

	    case UI_BUTTON:
		ui_delete(current->data.button);
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

//NOTE(denis): only one EditText can be selected per group
static EditText* getSelectedEditText(int groupID)
{
    EditText *result = NULL;
    Node *current = _groups.front;

    while (current != NULL && current->data.ll->id != groupID)
    {
	current = current->next;
    }

    if (current != NULL)
    {
	current = current->data.ll->front;
	while (current != NULL && !result)
	{
	    if (current->data.type == UI_EDITTEXT)
	    {
		EditText *editText = current->data.editText;
		result = editText->selected ? editText : NULL;
	    }

	    current = current->next;
	}
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
	(_font = TTF_OpenFont(fontName, fontSize)))
    {
        _renderer = renderer;
	    
        _cursor.pos.w = 5;
        _cursor.pos.h = 15;
        _cursor.flashRate = 300;
        _cursor.visible = true;

	_currentID = 1;
	
	result = true;
    }

    return result;
}

void ui_destroy()
{
    ui_delete(&_groups);
    
    TTF_CloseFont(_font);
    _font = NULL;
    
    TTF_Quit();
}

bool ui_wasClicked(Button button, Vector2 mouse)
{
    return pointInRect(mouse, button.background.pos) && button.startedClick;
}

void ui_processMouseDown(Vector2 mousePos, Uint8 button)
{
    Node *currentRoot = _groups.front;
    Node *currentGroup = NULL;

    while (currentRoot != NULL)
    {
	currentGroup = currentRoot->data.ll->front;

	while (currentGroup != NULL)
	{
	    if (button == SDL_BUTTON_LEFT)
	    {
		if (currentGroup->data.type == UI_BUTTON)
		{
		    Button *data = currentGroup->data.button;
		    data->startedClick = pointInRect(mousePos, data->background.pos);
		}
	    }
	    
	    currentGroup = currentGroup->next;
	}

	currentRoot = currentRoot->next;
    }
}

void ui_processMouseUp(Vector2 mousePos, Uint8 button)
{
    Node *currentRoot = _groups.front;
    Node *currentGroup = NULL;
    
    while(currentRoot != 0)
    {
	currentGroup = currentRoot->data.ll->front;

	while (currentGroup != 0)
	{
	    if (button == SDL_BUTTON_LEFT)
	    {
		if (currentGroup->data.type == UI_EDITTEXT)
		{
		    EditText *data = currentGroup->data.editText;
		    data->selected = pointInRect(mousePos, data->pos);

		    if (data->selected)
		    {
			resetCursorPosition(data);
			_cursor.flashCounter = 0;
		    }
		}
	    }

	    currentGroup = currentGroup->next;
	}

	currentRoot = currentRoot->next;
    }
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

void ui_processLetterTyped(char c, int groupID)
{
    EditText *editText = getSelectedEditText(groupID);

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

void ui_eraseLetter(int groupID)
{
    EditText *editText = getSelectedEditText(groupID);
    
    if (editText)
	ui_eraseLetter(editText);
}

static int addToGroup(UIElement data, int groupID)
{
    int result = groupID;
    
    Node *current = _groups.front;
    
    while (current != NULL && current->data.type == UI_LINKEDLIST &&
	   current->data.ll->id != groupID)
    {
	current = current->next;
    }
    
    if (current != NULL && current->data.type == UI_LINKEDLIST)
    {
	adt_addTo(current->data.ll, data);
    }
    else
    {
	result = _currentID++;

	UIElement newLL = {};
	newLL.type = UI_LINKEDLIST;
	newLL.ll = (LinkedList *)malloc(sizeof(LinkedList));
        newLL.ll->front = NULL;
	newLL.ll->id = result;
	    
	adt_addTo(&_groups, newLL);
	adt_addTo(newLL.ll, data);
    }

    return result;
}

int ui_addToGroup(EditText *editText)
{
    return ui_addToGroup(editText, 0);
}

int ui_addToGroup(EditText *editText, int groupID)
{
    int result = groupID;
    
    UIElement data = {};
    data.type = UI_EDITTEXT;
    data.editText = editText;

    result = addToGroup(data, groupID);    

    return result;
}

int ui_addToGroup(TexturedRect *textField)
{
    return ui_addToGroup(textField, 0);
}
int ui_addToGroup(TexturedRect *textField, int groupID)
{
    int result = groupID;

    UIElement data = {};
    data.type = UI_TEXTFIELD;
    data.textField = textField;

    result = addToGroup(data, groupID);
    
    return result;
}

int ui_addToGroup(Button *button)
{
    return ui_addToGroup(button, 0);
}
int ui_addToGroup(Button *button, int groupID)
{
    UIElement data = {};
    data.type = UI_BUTTON;
    data.button = button;

    return addToGroup(data, groupID);
}

//TODO(denis): also have a "clear group" function that doesn't delete the group?
//TODO(denis): I don't think this deletes properly
void ui_deleteGroup(int groupID)
{
    Node *current = _groups.front;
    Node *prev = NULL;

    while (current != NULL && current->data.ll->id != groupID)
    {
	prev = current;
	current = current->next;
    }

    if (current != NULL)
    {
	if (prev)
	    prev->next = current->next;
	else
	    _groups = {};

	ui_delete(current->data.ll);
	free(current);
    }
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

//TODO(denis): perhaps have multiple versions that do a different kind of
// TTF_RenderText, like Shaded and Solid
//TODO(denis): exchange everything to 0xAABBGGRR ? dunno
// or have a setTextColour method instead of passing SDL_Color around
// have a colour bitfield?
TexturedRect ui_createTextField(char *text, int x, int y, SDL_Color colour)
{
    TexturedRect result = {};
    
    SDL_Surface* tempSurf = TTF_RenderText_Blended(_font, text, colour);

    SDL_GetClipRect(tempSurf, &result.pos);
    result.pos.x = x;
    result.pos.y = y;

    result.image = SDL_CreateTextureFromSurface(_renderer, tempSurf);
    SDL_FreeSurface(tempSurf);

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
    result.background = createFilledTexturedRect(_renderer, width, height, backgroundColour);

    //TODO(denis): add options for left align, centre aligned, and right aligned text
    result.foreground = ui_createTextField(text, result.background.pos.x,
					result.background.pos.y, textColour);
    
    return result;
}

Button ui_createImageButton(char *imageFileName)
{
    Button result = {};
    
    result.background = loadImage(_renderer, imageFileName);
    
    return result;
}


// NOTE(denis): all the drawing functions

void ui_draw()
{
    bool editTextSelected = false;
    
    Node *currentRoot = _groups.front;
    Node *currentGroup = NULL;
    
    while (currentRoot != 0)
    {
	currentGroup = currentRoot->data.ll->front;
	while (currentGroup != NULL)
	{
	    switch (currentGroup->data.type)
	    {
		case UI_TEXTFIELD:
		    ui_draw(currentGroup->data.textField);
		    break;

		case UI_EDITTEXT:
		    if (currentGroup->data.editText->selected)
		    {
			editTextSelected = true;
			resetCursorPosition(currentGroup->data.editText);
		    }
		    
		    ui_draw(currentGroup->data.editText);
		    break;

		case UI_BUTTON:
		    ui_draw(currentGroup->data.button);
		    break;
	    }
	    
	    currentGroup = currentGroup->next;
	}

	currentRoot = currentRoot->next;
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
