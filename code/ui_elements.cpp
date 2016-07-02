#include "main.h"
#include "ui_elements.h"
#include "SDL_render.h"
#include "TEMP_GeneralFunctions.cpp"

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

//TODO(denis): have this be a LinkedList or some type that can hold an arbitrary
// number of group items
// also, have another array/linked list that holds all the groups
static EditText *_group[100];
static int _groupCount;

static void resetCursorPosition(EditText *editText)
{
    _cursor.pos.y = editText->pos.y + editText->padding/2;
    _cursor.pos.x = editText->padding + editText->pos.x + editText->letterCount*editText->letters[0].pos.w;

    if (_cursor.pos.x > editText->pos.x + editText->pos.w)
	_cursor.pos.x -= editText->letters[editText->letterCount-1].pos.w;
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
	    
	result = true;
    }

    return result; 
}

void ui_destroy()
{
    TTF_CloseFont(_font);
    _font = NULL;
    
    TTF_Quit();
}

void ui_processMouseDown(Vector2 mousePos, Uint8 button)
{
    //TODO(denis): handle buttons in groups
}

void ui_processMouseUp(Vector2 mousePos, Uint8 button)
{
    if (_groupCount > 0)
    {
	for (int i = 0; i < _groupCount; ++i)
	{
	    if (button == SDL_BUTTON_LEFT)
	    {
		_group[i]->selected = pointInRect(mousePos, _group[i]->pos);

		if (_group[i]->selected)
		{
		    resetCursorPosition(_group[i]);
		    _cursor.flashCounter = 0;
		}
	    }
	}
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

//TODO(denis): take an id of the group to add a letter to
// maybe have a way for mulitple groups to be called
void ui_processLetterTyped(char c)
{
    for (int i = 0; i < _groupCount; ++i)
    {
	if (_group[i]->selected)
	{
	    ui_addLetterTo(_group[i], c);
	}
    }
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

void ui_eraseLetter()
{
    for (int i = 0; i < _groupCount; ++i)
    {
	if (_group[i]->selected)
	{
	    ui_eraseLetter(_group[i]);
	}
    }
}

//TODO(denis): have this take a UI element type and the group to add it to (by id or
// something) or NULL for the group to add the element to a new group
// and return the group id or whatever
void ui_addToGroup(EditText *editText)
{
    _group[_groupCount++] = editText;
}

//TODO(denis): should take a group id
void ui_deleteGroup()
{
    for (int i = 0; i < _groupCount; ++i)
    {
	ui_delete(_group[i]);
    }

    _groupCount = 0;
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
    editText->text[0] = 0;
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


// NOTE(denis): all the drawing functions

void ui_draw()
{
    bool editTextSelected = false;
    
    for (int i = 0; i < _groupCount; ++i)
    {
	EditText *text = _group[i];
        ui_draw(text);

	if (text->selected)
	{
	    resetCursorPosition(text);
	    editTextSelected = true;
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
