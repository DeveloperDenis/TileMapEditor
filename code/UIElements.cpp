#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_ttf.h"

#include "UIElements.h"
#include "TEMP_GeneralFunctions.cpp"

/* TODO(denis):
 *   to dos:
 *     -potentially have multiple different fonts open and render for rendering
 *      and when you call a draw with a different font size or font name than is saved
 *      a new font is dynamically created and saved for later
 *     -have a "flush fonts" function that deletes any fonts currently open to prevent
 *      clutter (or maybe dynamically decide when to delete fonts by keeping  counter
 *      of how many calls since they were used?)
 */

static SDL_Renderer *globalRenderer;
static TTF_Font *globalFont;
static TextCursor globalCursor;
static bool editTextSelected;

static TexturedRect globalTextFields[100];
static int globalTextFieldCount;

static EditText globalEditTexts[100];
static int globalEditTextCount;

bool initUI(SDL_Renderer *renderer, char *fileName, int fontSize)
{
    bool result = false;

    if (TTF_Init() == 0 &&
	(globalFont = TTF_OpenFont(fileName, fontSize)))
    {
	globalRenderer = renderer;
	    
	globalCursor.pos.w = 5;
	globalCursor.pos.h = 15;
	globalCursor.flashRate = 300;
	globalCursor.visible = true;
	    
	result = true;
    }

    return result;
}

void destroyUI()
{
    TTF_CloseFont(globalFont);
    TTF_Quit();
}

void deleteAllEditTexts()
{
    for (int i = 0; i < globalEditTextCount; ++i)
    {
	EditText *text = &globalEditTexts[i];

	for (int j = 0; j < text->letterCount; ++j)
	{
	    SDL_DestroyTexture(text->letters[j].image);
	}
    }

    globalEditTextCount = 0;
}

void deleteAllTextFields()
{
    for (int i = 0; i < globalTextFieldCount; ++i)
    {
	SDL_DestroyTexture(globalTextFields[i].image);
    }

    globalTextFieldCount = 0;
}

static void resetTextCursorPosition(EditText*);
void uiHandleClicks(int mouseX, int mouseY, Uint8 button)
{
    editTextSelected = false;
    
    if (button == SDL_BUTTON_LEFT)
    {
	for (int i = 0; i < globalEditTextCount; i++)
	{
	    EditText *current = &globalEditTexts[i];
	    if (pointInRect(mouseX, mouseY, current->pos))
	    {
		current->selected = true;
		resetTextCursorPosition(current);
		globalCursor.flashCounter = 0;
		editTextSelected = true;
	    }
	    else
		current->selected = false;
	}
    }
}



/* NOTE(denis):
 *   TextField specific functions here!
 */

//TODO(denis): perhaps have multiple versions that do a different kind of
// TTF_RenderText, like Shaded and Solid
static TexturedRect createNewTextField(char* text, int x, int y, SDL_Color colour)
{
    TexturedRect result = {};
    
    SDL_Surface* tempSurf = TTF_RenderText_Blended(globalFont, text, colour);

    SDL_GetClipRect(tempSurf, &result.pos);
    result.pos.x = x;
    result.pos.y = y;

    result.image = SDL_CreateTextureFromSurface(globalRenderer, tempSurf);
    SDL_FreeSurface(tempSurf);

    return result;
}


//TODO(denis): have a setTextColour method instead of passing colours around?
void addNewTextField(char *text, int x, int y, SDL_Color colour)
{
    if (globalTextFieldCount < sizeof(globalTextFields)/sizeof(TexturedRect))
	globalTextFields[globalTextFieldCount++] = createNewTextField(text, x, y, colour);
    else
    {
	//TODO(denis): increase the size and copy elements into new array
    }
}

void drawTextFields()
{
    for (int i = 0; i < globalTextFieldCount; ++i)
    {
	SDL_RenderCopy(globalRenderer, globalTextFields[i].image, NULL,
		       &globalTextFields[i].pos);
    }
}



/* NOTE(denis):
 *   EditText specific functions here!
 */

static void removeLetter(EditText *editText)
{
    if (editText->letterCount > 0)
    {
	--editText->letterCount;

	editText->text[editText->letterCount] = 0;
	SDL_DestroyTexture(editText->letters[editText->letterCount].image);
    }
}

static void resetTextCursorPosition(EditText *editText)
{
    globalCursor.pos.y = editText->pos.y + editText->padding/2;
    globalCursor.pos.x = editText->padding + editText->pos.x + editText->letterCount*editText->letters[0].pos.w;

    if (globalCursor.pos.x > editText->pos.x + editText->pos.w)
	globalCursor.pos.x -= editText->letters[editText->letterCount-1].pos.w;
}

static void draw(EditText *obj)
{
    SDL_Color colour = obj->backgroundColour;
    SDL_SetRenderDrawColor(globalRenderer, colour.r, colour.g, colour.b, colour.a);
    SDL_RenderFillRect(globalRenderer, &obj->pos);

    for (int i = 0; i < obj->letterCount; ++i)
    {
	SDL_RenderCopy(globalRenderer, obj->letters[i].image, 0,
		       &obj->letters[i].pos);
    }
}

static EditText createNewEditText(int x, int y, int width, int height,
				  SDL_Color backgroundColour, int padding, int id)
{
    EditText result = {};
    result.backgroundColour = backgroundColour;
    result.pos = {x, y, width, height};
    result.padding = padding;
    result.id = id;
    
    return result;
}

//NOTE(denis): assumes the char array is capped by 0
static bool charInArray(char c, char array[])
{
    bool result = false;

    if (array != NULL)
    {
	for (int i = 0; array[i] != 0 && !result; ++i)
	{
	    if (array[i] == c)
		result = true;
	}
    }
    
    return result;
}

static void addNewLetter(EditText *editText, char letter, SDL_Colour colour)
{
    int textWidth = editText->pos.w;
    int textX = editText->pos.x;

    if (editText->allowedCharacters == NULL ||
	charInArray(letter, editText->allowedCharacters))
    {
	if ((editText->letterCount == 0 ||
	     (editText->letterCount >= 1 &&
	      textX+editText->letterCount*editText->letters[editText->letterCount-1].pos.w < textX + textWidth)))
	{
	    char letters[] = {letter, 0};
	    TexturedRect letterText =
		createNewTextField(letters, editText->pos.x + editText->letterCount*editText->letters[0].pos.w,
				   editText->pos.y, colour);

	    editText->text[editText->letterCount] = letter;
	    editText->letters[editText->letterCount++] = letterText;
	}
    }
}

EditText *getEditTextByID(int searchID)
{
    EditText *result = NULL;
    
    for (int i = 0; i < globalEditTextCount && !result; ++i)
    {
	if (globalEditTexts[i].id == searchID)
	    result = &globalEditTexts[i];
    }
    
    return result;
}

EditText *getSelectedEditText()
{
    EditText *result = NULL;
    
    for (int i = 0; i < globalEditTextCount && !result; ++i)
    {
	if (globalEditTexts[i].selected)
	    result = &globalEditTexts[i];
    }
    
    return result;
}

void addNewEditText(int x, int y, int width, int height, SDL_Color bgColour,
			 int padding, int id, char *defaultText)
{    
    if (globalEditTextCount < sizeof(globalEditTexts)/sizeof(EditText))
    {
	globalEditTexts[globalEditTextCount++] =
	    createNewEditText(x, y, width, height, bgColour, padding, id);
	
	if (defaultText != NULL)
	{
	    SDL_Color defaultTextColour = {80, 80, 80, 255};
	    if (bgColour.r != 0)
		defaultTextColour.r = bgColour.r/2;
	    if (bgColour.g != 0)
		defaultTextColour.g = bgColour.g/2;
	    if (bgColour.b != 0)
		defaultTextColour.b = bgColour.b/2;
	    
	    
	    for (int i = 0; defaultText[i] != '\0'; ++i)
	    {
		addNewLetter(&globalEditTexts[globalEditTextCount-1], defaultText[i],
			     defaultTextColour);
	    }
	}
    }
    else
    {
	//TODO(denis): create new bigger array
    }
}

void addNewEditText(int x, int y, int width, int height, SDL_Color bgColour,
			 int padding, int id)
{
    addNewEditText(x, y, width, height, bgColour, padding, id, NULL);
}

void drawEditTexts()
{
    for (int i = 0; i < globalEditTextCount; ++i)
    {
	EditText *text = &globalEditTexts[i];
        draw(text);

	if (text->selected)
	{
	    resetTextCursorPosition(text);
	}
    }

    if (editTextSelected)
    {
	
	
	if (globalCursor.flashCounter >= globalCursor.flashRate)
	{
	    //TODO(denis): don't hardcode this, probably
	    SDL_SetRenderDrawColor(globalRenderer, 140,140,140,255);
	    SDL_RenderFillRect(globalRenderer, &globalCursor.pos);
	    globalCursor.visible = false;
	}
	if (!globalCursor.visible
	    && globalCursor.flashCounter >= globalCursor.flashRate*2)
	{
	    globalCursor.visible = true;
	    globalCursor.flashCounter = 0;
	}
	++globalCursor.flashCounter;
    }
}

void editTextEraseLetter(EditText *editText)
{
    removeLetter(editText);
}   

void editTextEraseLetter()
{
    if (editTextSelected)
    {
	for (int i = 0; i < globalEditTextCount; i++)
	{
	    if (globalEditTexts[i].selected)
	    {
		removeLetter(&globalEditTexts[i]);
	    }
	}
    }
}

void editTextAddLetter(char letter, SDL_Color colour)
{
    if (editTextSelected)
    {
	for (int i = 0; i < globalEditTextCount; i++)
	{
	    if (globalEditTexts[i].selected)
	    {
		addNewLetter(&globalEditTexts[i], letter, colour);
	    }
	}
    }
}

void editTextAddLetter(EditText *editText, char letter, SDL_Color colour)
{
    addNewLetter(editText, letter, colour);
}
