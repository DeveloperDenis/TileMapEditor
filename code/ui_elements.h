#ifndef UI_ELEMENTS_H_
#define UI_ELEMENTS_H_

struct SDL_Renderer;
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

//TODO(denis): I might want something like this business in the future
enum
{
    UI_TEXTFIELD,
    UI_EDITTEXT,
    UI_BUTTON
};

struct UIElement
{
    int type;
    
    union
    {
	TexturedRect textField;
	EditText editText;
	Button button;
    };
};

bool ui_init(SDL_Renderer *renderer, char *fontName, int fontSize);
void ui_destroy();

void ui_processMouseDown(Vector2 mousePos, Uint8 button);
void ui_processMouseUp(Vector2 mousePos, Uint8 button);

void ui_addLetterTo(EditText *editText, char c);
void ui_processLetterTyped(char c);

//NOTE(denis): erases a letter from the currently selected EditText
void ui_eraseLetter();
//NOTE(denis): erases a letter from the given EditText
void ui_eraseLetter(EditText *editText);

TexturedRect ui_createTextField(char *text, int x, int y, SDL_Color colour);
EditText ui_createEditText(int x, int y, int width, int height,
			   SDL_Color backgroundColour, int padding);

//TODO(denis): want to support many different groups of different types of ui objects
void ui_addToGroup(EditText *editText);

void ui_deleteGroup();
void ui_delete(TexturedRect *TexturedRect);
void ui_delete(EditText *editText);

//NOTE(denis): general draw function that draws any elements in groups
void ui_draw();

void ui_draw(Button *button);
void ui_draw(TexturedRect *texturedRect);
void ui_draw(EditText *editText);

const SDL_Color COLOUR_WHITE = {255,255,255,255};
const SDL_Color COLOUR_BLACK = {0,0,0,255};

#endif
