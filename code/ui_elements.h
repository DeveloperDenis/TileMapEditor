#ifndef UI_ELEMENTS_H_
#define UI_ELEMENTS_H_

#include "main.h"

struct SDL_Renderer;
struct LinkedList;

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

struct TextBox
{
    SDL_Texture *background;
    TexturedRect text;
    SDL_Rect pos;

    void setPosition(Vector2);
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

struct DropDownMenu
{
    //TODO(denis): probably don't always want 10 max
    TextBox items[10];
    int itemCount;
    
    int highlightedItem;
    SDL_Texture *unselectedTexture;
    SDL_Texture *selectionBox;
    
    bool isOpen;
    bool startedClick;
    
    SDL_Rect getRect();
};

struct MenuBar
{
    DropDownMenu menus[6];
    int menuCount;

    Vector2 topLeft;
    Vector2 botRight;

    //TODO(denis): should this be a Texture or just a colour?
    Uint32 colour;
    Uint32 textColour;

    void addMenu(char *items[], int numItems, int menuWidth);

    void onMouseMove(Vector2 mousePos);
    void onMouseDown(Vector2 mousePos, Uint8 button);
    void onMouseUp(Vector2 mousePos, Uint8 button);
};

enum
{
    UI_LINKEDLIST = 1,
    UI_TEXTFIELD,
    UI_EDITTEXT,
    UI_BUTTON,
    UI_TEXTBOX,
    UI_DROPDOWNMENU
};
struct UIElement
{
    int type;
    
    union
    {
	LinkedList *ll;
	TexturedRect *textField;
	TextBox *textBox;
	EditText *editText;
	Button *button;
	DropDownMenu *dropDownMenu;
    };
};

bool ui_init(SDL_Renderer *renderer, char *fontName, int fontSize);
void ui_destroy();

//NOTE(denis): returns false if the font was not found
bool ui_setFont(char *fontName, int fontSize);

void ui_processMouseDown(Vector2 mousePos, Uint8 button);
void ui_processMouseUp(Vector2 mousePos, Uint8 button);
//TODO(denis): might want something like this
//void ui_processMouseMotion(Vector2 mousePos);

bool ui_wasClicked(Button button, Vector2 mouse);

void ui_addLetterTo(EditText *editText, char c);
void ui_processLetterTyped(char c, int groupID);

//NOTE(denis): erases a letter from the currently selected EditText
void ui_eraseLetter(int groupID);
//NOTE(denis): erases a letter from the given EditText
void ui_eraseLetter(EditText *editText);

TexturedRect ui_createTextField(char *text, int x, int y, SDL_Color colour);
TextBox ui_createTextBox(char *text, int minWidth, int minHeight, SDL_Color textColour,
			 Uint32 backgroundColour);
EditText ui_createEditText(int x, int y, int width, int height,
			   SDL_Color backgroundColour, int padding);
Button ui_createTextButton(char *text, SDL_Color textColour, int width, int height,
			   Uint32 backgroundColour);
Button ui_createImageButton(char *imageFileName);
DropDownMenu ui_createDropDownMenu(char *items[], int itemNum,
				   int itemWidth, int itemHeight,
				   SDL_Color textColour, Uint32 backgroundColour);
MenuBar ui_createMenuBar(int x, int y, int width, int height, Uint32 colour,
			 Uint32 textColour);

//NOTE(denis): these functions return the id of the group the data was added to
// for functions with two parameters the id may differ from the passed in ID if that
// ID does not exist, in that case the returned ID is for new group made
//TODO(denis): maybe the id entered should be presevered even if it doesn't yet exist?
int ui_addToGroup(EditText *editText);
int ui_addToGroup(EditText *editText, int groupID);
int ui_addToGroup(TexturedRect *textField);
int ui_addToGroup(TexturedRect *textField, int groupID);
int ui_addToGroup(Button *button);
int ui_addToGroup(Button *button, int groupID);
int ui_addToGroup(TextBox *textBox);
int ui_addToGroup(TextBox *textBox, int groupID);

void ui_deleteGroup(int groupID);
void ui_delete(TexturedRect *TexturedRect);
void ui_delete(EditText *editText);
void ui_delete(Button *button);
void ui_delete(TextBox *textBox);

//NOTE(denis): general draw function that draws any elements in groups
void ui_draw();

void ui_draw(Button *button);
void ui_draw(TexturedRect *texturedRect);
void ui_draw(EditText *editText);
void ui_draw(TextBox *textBox);
void ui_draw(DropDownMenu *dropDownMenu);
void ui_draw(MenuBar *menuBar);

const SDL_Color COLOUR_WHITE = {255,255,255,255};
const SDL_Color COLOUR_BLACK = {0,0,0,255};

#endif
