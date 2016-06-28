/* TODO(denis):
 * in the beginning, check the resolution of the user's monitor, and don't allow them
 * to make tile maps that would be too big!

 * Additionally, I think it would be cool if when you pressed TAB, your selected 
 * EditText changed to the next logical EditText
 * same with hitting Shift+TAB, it should make you select the previous text box
 *
 * have a "paint tool" and "fill tool" for drawing the tile maps
 */

#include <SDL.h>
#include "SDL_ttf.h"
#include "windows.h"
#include <math.h>
#include "UIElements.h"
#include "TEMP_GeneralFunctions.cpp"

#define TITLE "Tile Map Editor"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

static TexturedRect createFilledTexturedRect(SDL_Renderer *renderer,
					     int width, int height, Uint32 colour)
{
    TexturedRect result = {};
    
    SDL_Surface *rectangle = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    SDL_FillRect(rectangle, NULL, colour);
    result.image = SDL_CreateTextureFromSurface(renderer, rectangle);
    SDL_GetClipRect(rectangle, &result.pos);
    SDL_FreeSurface(rectangle);

    return result;
}

static void fillBWithAConverted(float conversionFactor, EditText *A, EditText *B)
{
    int numberA = convertStringToInt(A->text, A->letterCount);
    int numberB = (int)(numberA*conversionFactor);
    
    while (B->letterCount > 0)
	editTextEraseLetter(B);

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
	editTextAddLetter(B, letter, COLOUR_BLACK);
    }
}

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_Window *window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED,
					  SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
					  WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
        
    if (window)
    {
	//TODO(denis): maybe add renderer flags?
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer && initUI(renderer, "LiberationMono-Regular.ttf", 16))
	{
	    bool running = true;
	    
	    int tileMapStartX = 5;
	    int tileMapStartY = 5;
	    
	    char *tileMapName = NULL;
	    int tileSize = 0;
	    int tileMapWidth = 0;
	    int tileMapHeight = 0;
	    TexturedRect *tileMap = NULL;
	    
	    const int MAP_NAME_TEXT = 0;
	    const int TILE_SIZE_TEXT = 1;
	    const int WIDTH_TEXT_PX = 2;
	    const int HEIGHT_TEXT_PX = 3;
	    const int WIDTH_TEXT_TILES = 4;
	    const int HEIGHT_TEXT_TILES = 5;

	    addNewTextField("Tile Map Name: ", 100, 100, COLOUR_WHITE);
	    addNewTextField("Tile Size: ", 100, 150, COLOUR_WHITE);
	    addNewTextField("Width: ", 100, 200, COLOUR_WHITE);
	    addNewTextField("pixels", 455, 200, COLOUR_WHITE);
	    addNewTextField("tiles", 760, 200, COLOUR_WHITE);
	    addNewTextField("Height: ", 100, 250, COLOUR_WHITE);
	    addNewTextField("pixels", 455, 250, COLOUR_WHITE);
	    addNewTextField("tiles", 760, 250, COLOUR_WHITE);

	    addNewEditText(250, 100, 200, 20, COLOUR_WHITE, 2, MAP_NAME_TEXT);
	    addNewEditText(250, 150, 200, 20, COLOUR_WHITE,  2, TILE_SIZE_TEXT);
	    addNewEditText(250, 200, 200, 20, COLOUR_WHITE, 2, WIDTH_TEXT_PX);
	    addNewEditText(550, 200, 200, 20, COLOUR_WHITE, 2, WIDTH_TEXT_TILES);
	    addNewEditText(250, 250, 200, 20, COLOUR_WHITE, 2, HEIGHT_TEXT_PX);
	    addNewEditText(550, 250, 200, 20, COLOUR_WHITE, 2, HEIGHT_TEXT_TILES);
	    
	    char numberChars[] =  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0};
	    
	    EditText *tileSizeText = getEditTextByID(TILE_SIZE_TEXT);
	    tileSizeText->allowedCharacters = numberChars;

	    EditText *widthTextPx = getEditTextByID(WIDTH_TEXT_PX);
	    widthTextPx->allowedCharacters = numberChars;

	    EditText *widthTextTiles = getEditTextByID(WIDTH_TEXT_TILES);
	    widthTextTiles->allowedCharacters = numberChars;

	    EditText *heightTextPx = getEditTextByID(HEIGHT_TEXT_PX);
	    heightTextPx->allowedCharacters = numberChars;
	    
	    EditText *heightTextTiles = getEditTextByID(HEIGHT_TEXT_TILES);
	    heightTextTiles->allowedCharacters = numberChars;

	    bool buttonVisible = true;
	    TexturedRect createNewButton =
		createFilledTexturedRect(renderer, 200, 100, 0x000000FF);
	    createNewButton.pos.x = 500;
	    createNewButton.pos.y = 500;
	    addNewTextField("Create New Map", createNewButton.pos.x + 20,
			    createNewButton.pos.y + createNewButton.pos.h/2,
			    COLOUR_WHITE);
	    
	    
	    while (running)
	    {
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
		    switch(event.type)
		    {
			case SDL_QUIT:
			{
			    running = false;
			} break;

			case SDL_WINDOWEVENT:
			{
			    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
			    {
				if (tileMap)
				{
				    int windowWidth = event.window.data1;
				    int windowHeight = event.window.data2;
				    
				    tileMapStartX = windowWidth/2 - tileSize*tileMapWidth/2;
				    tileMapStartY = windowHeight/2 - tileSize*tileMapHeight/2;

				    TexturedRect *row = tileMap;
				    for (int i = 0; i < tileMapHeight; ++i)
				    {
					TexturedRect *element = row;
					for (int j = 0; j < tileMapWidth; ++j)
					{
					    element->pos.x = tileMapStartX + j*tileSize;
					    element->pos.y = tileMapStartY + i*tileSize;
					    ++element;
					}
					row += tileMapWidth;
				    }
				}
			    }
			    
			} break;
			
			case SDL_MOUSEBUTTONUP:
			{
			    int x = event.button.x;
			    int y = event.button.y;

			    uiHandleClicks(x, y, event.button.button);

			    if (pointInRect(x, y, createNewButton.pos))
			    {
				EditText *nameText = getEditTextByID(MAP_NAME_TEXT);
				tileMapName = nameText->text;

				tileSize = convertStringToInt(tileSizeText->text, tileSizeText->letterCount);
				tileMapWidth = convertStringToInt(widthTextTiles->text, widthTextTiles->letterCount);
				tileMapHeight = convertStringToInt(heightTextTiles->text, heightTextTiles->letterCount);

				if (tileMapName && tileSize != 0 &&
				    tileMapWidth != 0 && tileMapHeight != 0)
				{
				    deleteAllEditTexts();
				    deleteAllTextFields();
				    buttonVisible = false;
				    createNewButton.pos = {};
				    SDL_DestroyTexture(createNewButton.image);
				    createNewButton.image = NULL;

				    int memorySize = sizeof(TexturedRect)*tileMapWidth*tileMapHeight;
				    
				    HANDLE heapHandle = GetProcessHeap();
				    tileMap = (TexturedRect*) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY,
						        memorySize);

				    if (tileMap)
				    {
					int windowHeight = 0;
					int windowWidth = 0;

					SDL_GetWindowSize(window, &windowWidth, &windowHeight);
					
					tileMapStartX = windowWidth/2 - tileSize*tileMapWidth/2;
					tileMapStartY = windowHeight/2 - tileSize*tileMapHeight/2;
					
					SDL_Surface *background = SDL_CreateRGBSurface(0, tileSize, tileSize, 32, 0, 0, 0, 0);
					SDL_FillRect(background, NULL, 0xFFFF00FF);
					
					//TODO(denis): dunno if there is any noticable
					// speed boost from not just calling
					//createFilledTexturedRect here
					TexturedRect *row = tileMap;
					for (int i = 0; i < tileMapHeight; ++i)
					{
					    TexturedRect *element = row;
					    for (int j = 0; j < tileMapWidth; ++j)
					    {
						SDL_Texture *texture;
						SDL_Rect rect;
						
						texture = SDL_CreateTextureFromSurface(renderer, background);
						SDL_GetClipRect(background, &rect);

						rect.x = tileMapStartX + j*tileSize;
						rect.y = tileMapStartY + i*tileSize;
						
					        element->image = texture;
						element->pos = rect;

						++element;
					    }
					    row += tileMapWidth;
					}

					SDL_FreeSurface(background);
				    }
				}
				else
				{
				    OutputDebugStringA("you gotta fill it all out!\n");
				}
			    }

			    if (tileMap &&
				pointInRect(x, y, {tileMapStartX, tileMapStartY, tileSize*tileMapWidth, tileSize*tileMapHeight}))
			    {
				int tileX = (x-tileMapStartX)/tileSize;
				int tileY = (y-tileMapStartY)/tileSize;

				TexturedRect *clicked = tileMap + tileX + tileY*tileMapWidth;
				TexturedRect temp = createFilledTexturedRect(renderer, tileSize,
									     tileSize, 0x0000FFFF);
				clicked->image = temp.image;
			    }
			    
			} break;

			case SDL_TEXTINPUT:
			{
			    char* theText = event.text.text;

			    editTextAddLetter(theText[0], COLOUR_BLACK);

			    int tileSize = convertStringToInt(tileSizeText->text, tileSizeText->letterCount);
			    
			    if (tileSizeText->selected)
			    {
			        fillBWithAConverted(1.0F/(float)tileSize, widthTextPx, widthTextTiles);
				fillBWithAConverted(1.0F/(float)tileSize, heightTextPx, heightTextTiles);
			    }
			    else if (tileSizeText->text[0] != 0)
			    {
				if (widthTextTiles->text[0] != 0 ||
				    widthTextPx->text[0] != 0)
				{
				    if (widthTextTiles->selected)
				    {
					fillBWithAConverted((float)tileSize, widthTextTiles, widthTextPx);
				    }
				    else if (widthTextPx->selected)
				    {
				        fillBWithAConverted(1.0F/(float)tileSize, widthTextPx, widthTextTiles);
				    }
				}
				if (heightTextTiles->text[0]!=0 ||
				    heightTextPx->text[0] != 0)
				{
				    if (heightTextTiles->selected)
				    {
					fillBWithAConverted((float)tileSize, heightTextTiles, heightTextPx);
				    }
				    else if (heightTextPx->selected)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, heightTextPx, heightTextTiles);
				    }
				}
			    }
			} break;

			case SDL_KEYDOWN:
			{			    
			    if (event.key.keysym.sym == SDLK_BACKSPACE)
			    {
				EditText *text = getSelectedEditText();
				editTextEraseLetter(text);
				
				if (text)
				{
				    int tileSize = convertStringToInt(
					tileSizeText->text, tileSizeText->letterCount);

				    if (text->id == TILE_SIZE_TEXT)
				    {
					fillBWithAConverted(1.0F/(float)tileSize, widthTextPx, widthTextTiles);
					fillBWithAConverted(1.0F/(float)tileSize, heightTextPx, heightTextTiles);
				    }
				    else
				    {
					switch(text->id)
					{
					    case WIDTH_TEXT_PX:
					    {
						fillBWithAConverted(1.0F/(float)tileSize, text, widthTextTiles);
					    } break;

					    case WIDTH_TEXT_TILES:
					    {
						fillBWithAConverted((float)tileSize, text, widthTextPx);
					    } break;

					    case HEIGHT_TEXT_PX:
					    {
						fillBWithAConverted(1.0F/(float)tileSize, text, heightTextTiles);
					    } break;

					    case HEIGHT_TEXT_TILES:
					    {
						fillBWithAConverted((float)tileSize, text, heightTextPx);
					    } break;
					}
				    }
				}
			    }
			} break;
		    }
		}

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderClear(renderer);

		if (buttonVisible)
		    SDL_RenderCopy(renderer, createNewButton.image, NULL, &createNewButton.pos);

		drawTextFields();
		drawEditTexts();

		if (tileMap && tileMapWidth != 0 && tileMapHeight != 0)
		{
		    TexturedRect *row = tileMap;
		    for (int i = 0; i < tileMapHeight; ++i)
		    {
			TexturedRect *element = row;
			for (int j = 0; j < tileMapWidth; ++j)
			{
			    SDL_RenderCopy(renderer, element->image, NULL, &element->pos);
			    ++element;
			}

			row += tileMapWidth;
		    }

		    SDL_SetRenderDrawColor(renderer, 255, 255,255,255);
		    SDL_RenderDrawLine(renderer, tileMapStartX, tileMapStartY, tileMapStartX, tileMapStartY+tileMapHeight*tileSize);
		}
		
		SDL_RenderPresent(renderer);
	    }

	    if (tileMap)
		HeapFree(GetProcessHeap(), 0, tileMap);
	}
	
	destroyUI();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
    }

    SDL_Quit();
    return 0;
}
