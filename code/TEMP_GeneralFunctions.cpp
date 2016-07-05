#include "SDL_rect.h"
#include "SDL_image.h"
#include "denis_math.h"

static inline bool pointInRect(Vector2 point, SDL_Rect rect)
{
    return point.x > rect.x && point.x < rect.x+rect.w &&
	point.y > rect.y && point.y < rect.y+rect.h;
}

static inline int convertStringToInt(char string[], int size)
{
    int result = 0;

    for (int i = 0; i < size; ++i)
    {
	int num = (string[i]-'0') * exponent(10, size-1-i);
	result += num;
    }

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

static TexturedRect createFilledTexturedRect(SDL_Renderer *renderer,
					     int width, int height, Uint32 colour)
{
    TexturedRect result = {};

    Uint32 rmask, gmask, bmask, amask;
    amask = 0xFF000000;
    bmask = 0x00FF0000;
    gmask = 0x0000FF00;
    rmask = 0x000000FF;
    
    SDL_Surface *rectangle = SDL_CreateRGBSurface(0, width, height, 32,
						  rmask, gmask, bmask, amask);
    SDL_FillRect(rectangle, NULL, colour);
    result.image = SDL_CreateTextureFromSurface(renderer, rectangle);
    SDL_GetClipRect(rectangle, &result.pos);
    SDL_FreeSurface(rectangle);

    return result;
}

static TexturedRect loadImage(SDL_Renderer *renderer, char *fileName)
{
    TexturedRect result = {};
    
    SDL_Surface *tempSurf = IMG_Load(fileName);
    result.image = SDL_CreateTextureFromSurface(renderer, tempSurf);
    SDL_GetClipRect(tempSurf, &result.pos);
    SDL_FreeSurface(tempSurf);
    
    return result;
}
