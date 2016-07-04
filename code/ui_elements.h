#ifndef UI_ELEMENTS_H_
#define UI_ELEMENTS_H_

#include "main.h"

struct SDL_Renderer;
struct LinkedList;
// TODO(denis): have a Button and a TextButton ?
struct Button
{
    bool startedClick;
    TexturedRect background;
    
    char *text;
    TexturedRect foreground;

    void setPosition(Vector2);
    void destroy();
};

struct TextCursor
{
    SDL_Rect pos;
    int flashCounter;
    int flashRate;
    bool visible;
};

struct EditText
{
    SDL_Color backgroundColour;
    SDL_Rect pos;

    char *allowedCharacters;

    //TODO(denis): don't use "magic numbers" here
    TexturedRect letters[100];
    int letterCount;
    char text[100];

    //TODO(denis): padding isn't used for anything
    int padding;
    bool selected;
};

enum
{
    UI_LINKEDLIST = 1,
    UI_TEXTFIELD,
    UI_EDITTEXT,
    UI_BUTTON
};
struct UIElement
{
    int type;
    
    union
    {
	LinkedList *ll;
	TexturedRect *textField;
	EditText *editText;
	Button *button;
    };
};

bool ui_init(SDL_Renderer *renderer, char *fontName, int fontSize);
void ui_destroy();

void ui_processMouseDown(Vector2 mousePos, Uint8 button);
void ui_processMouseUp(Vector2 mousePos, Uint8 button);

void ui_addLetterTo(EditText *editText, char c);
void ui_processLetterTyped(char c, int groupID);

//NOTE(denis): erases a letter from the currently selected EditText
void ui_eraseLetter(int groupID);
//NOTE(denis): erases a letter from the given EditText
void ui_eraseLetter(EditText *editText);

TexturedRect ui_createTextField(char *text, int x, int y, SDL_Color colour);
EditText ui_createEditText(int x, int y, int width, int height,
			   SDL_Color backgroundColour, int padding);
Button ui_createTextButton(char *text, SDL_Color textColour, int width, int height,
			   Uint32 backgroundColour);

//NOTE(denis): these functions return the id of the group the data was added to
// for functions with two parameters the id may differ from the passed in ID if that
// ID does not exist, in that case the returned ID is for new group made
//TODO(denis): maybe the id entered should be presevered even if it doesn't yet exist?
int ui_addToGroup(EditText *editText);
int ui_addToGroup(EditText *editText, int groupID);

void ui_deleteGroup(int groupID);
void ui_delete(TexturedRect *TexturedRect);
void ui_delete(EditText *editText);
void ui_delete(Button *button);

//NOTE(denis): general draw function that draws any elements in groups
void ui_draw();

void ui_draw(Button *button);
void ui_draw(TexturedRect *texturedRect);
void ui_draw(EditText *editText);

const SDL_Color COLOUR_WHITE = {255,255,255,255};
const SDL_Color COLOUR_BLACK = {0,0,0,255};

#endif
