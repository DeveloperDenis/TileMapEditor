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

    int getWidth() { return background.pos.w; };
    void setPosition(Vector2 newPos);
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

    int getWidth() { return this->pos.w; };
    void setPosition(Vector2 newPos);
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

    int getWidth() { return this->pos.w; };
    void setPosition(Vector2 newPos);
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
    void setPosition(Vector2 newPos);
    int getWidth() { return this->getRect().w; };
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
    bool onMouseUp(Vector2 mousePos, Uint8 button);
};

enum UIElementType
{
    //TODO(denis): add UI_UIGROUP here?
    UI_TEXTFIELD = 1,
    UI_EDITTEXT,
    UI_BUTTON,
    UI_TEXTBOX,
    UI_DROPDOWNMENU
};
struct UIElement
{
    UIElementType type;
    
    union
    {
	//TODO(denis): add "UIGroup *group" here?
	TexturedRect *textField;
	TextBox *textBox;
	EditText *editText;
	Button *button;
	DropDownMenu *dropDownMenu;
    };

    int getWidth();
    void setPosition(Vector2 newPos);
};

struct UIPanel
{
    LinkedList *panelElements;
    bool visible;
    TexturedRect panel;
};

bool ui_init(SDL_Renderer *renderer, char *fontName, int fontSize);
void ui_destroy();

//NOTE(denis): returns false if the font was not found
bool ui_setFont(char *fontName, int fontSize);

bool ui_processMouseDown(UIPanel *group, Vector2 mousePos, Uint8 button);
bool ui_processMouseUp(UIPanel *group, Vector2 mousePos, Uint8 button);
//TODO(denis): might want something like this
//void ui_processMouseMotion(Vector2 mousePos);

bool ui_wasClicked(Button button, Vector2 mouse);

void ui_addLetterTo(EditText *editText, char c);
void ui_processLetterTyped(char c, UIPanel *group);

//NOTE(denis): erases a letter from the currently selected EditText in the group
void ui_eraseLetter(UIPanel *panel);
//NOTE(denis): erases a letter from the given EditText
void ui_eraseLetter(EditText *editText);

TexturedRect ui_createTextField(char *text, int x, int y, SDL_Color colour);
TextBox ui_createTextBox(char *text, int minWidth, int minHeight, SDL_Color textColour,
			 Uint32 backgroundColour);
EditText ui_createEditText(int x, int y, int width, int height,
			   SDL_Color backgroundColour, int padding);
//NOTE(denis): if the text area is larger than the given width or height, the
// button will be made as big as needed to hold all the text
Button ui_createTextButton(char *text, SDL_Color textColour, int width, int height,
			   Uint32 backgroundColour);
Button ui_createImageButton(char *imageFileName);
DropDownMenu ui_createDropDownMenu(char *items[], int itemNum,
				   int itemWidth, int itemHeight,
				   SDL_Color textColour, Uint32 backgroundColour);
MenuBar ui_createMenuBar(int x, int y, int width, int height, Uint32 colour,
			 Uint32 textColour);
UIPanel ui_createPanel(int x, int y, int width, int height, Uint32 colour);
//NOTE(denis): if you don't pass a panel, it returns a new panel containing the item
// you added
UIPanel ui_addToPanel(EditText *editText);
void ui_addToPanel(EditText *editText, UIPanel *panel);
UIPanel ui_addToPanel(TexturedRect *textField);
void ui_addToPanel(TexturedRect *textField, UIPanel *panel);
UIPanel ui_addToPanel(Button *button);
void ui_addToPanel(Button *button, UIPanel *panel);
UIPanel ui_addToPanel(TextBox *textBox);
void ui_addToPanel(TextBox *textBox, UIPanel *panel);

//TODO(denis): templates?
UIElement ui_packIntoUIElement(EditText *editText);
UIElement ui_packIntoUIElement(TexturedRect *texturedRect);
UIElement ui_packIntoUIElement(Button *button);
UIElement ui_packIntoUIElement(TextBox *textBox);
UIElement ui_packIntoUIElement(DropDownMenu *dropDownMenu);

void ui_delete(UIPanel *panel);
void ui_delete(TexturedRect *TexturedRect);
void ui_delete(EditText *editText);
void ui_delete(Button *button);
void ui_delete(TextBox *textBox);
void ui_delete(DropDownMenu *dropDownMenu);

void ui_draw(UIPanel *panel);
void ui_draw(Button *button);
void ui_draw(TexturedRect *texturedRect);
void ui_draw(EditText *editText);
void ui_draw(TextBox *textBox);
void ui_draw(DropDownMenu *dropDownMenu);
void ui_draw(MenuBar *menuBar);

const SDL_Color COLOUR_WHITE = {255,255,255,255};
const SDL_Color COLOUR_BLACK = {0,0,0,255};

#endif
