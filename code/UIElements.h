#ifndef UIELEMENTS_H_
#define UIELEMENTS_H_

struct TextCursor
{
    SDL_Rect pos;
    int flashCounter;
    int flashRate;
    bool visible;
};

struct TexturedRect
{
    SDL_Texture* image;
    SDL_Rect pos;
};

struct EditText
{
    SDL_Color backgroundColour;
    SDL_Rect pos;

    char *allowedCharacters;
    
    TexturedRect letters[100];
    int letterCount;
    char text[100];

    int id;
    int padding;
    bool selected;
};

bool initUI(SDL_Renderer *renderer, char *fontName, int fontSize);
void destroyUI();

void addNewTextField(char *text, int x, int y, SDL_Color colour);
void drawTextFields();

void addNewEditText(int x, int y, int width, int height, SDL_Color bgColor,
		    int padding, int id);
void addNewEditText(int x, int y, int width, int height, SDL_Color bgColour,
		    int padding, int id, char *defaultText);

void drawEditTexts();
void editTextEraseLetter();
void editTextEraseLetter(EditText *editText);
void editTextAddLetter(char letter, SDL_Color colour);
void editTextAddLetter(EditText *editText, char letter, SDL_Color colour);
EditText *getEditTextByID(int searchID);
EditText *getSelectedEditText();

void deleteAllEditTexts();
void deleteAllTextFields();

void uiHandleClicks(int mouseX, int mouseY, Uint8 button);

const SDL_Color COLOUR_WHITE = {255,255,255,255};
const SDL_Color COLOUR_BLACK = {0,0,0,255};

#endif
