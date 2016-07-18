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

static TexturedRect createFilledTexturedRect(SDL_Renderer *renderer,
					     int width, int height, uint32 colour)
{
    TexturedRect result = {};

    uint32 rmask, gmask, bmask, amask;
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

static inline TexturedRect loadImage(SDL_Renderer *renderer, char *fileName)
{
    TexturedRect result = {};
    
    SDL_Surface *tempSurf = IMG_Load(fileName);
    result.image = SDL_CreateTextureFromSurface(renderer, tempSurf);
    SDL_GetClipRect(tempSurf, &result.pos);
    SDL_FreeSurface(tempSurf);
    
    return result;
}

static inline SDL_Color hexColourToRGBA(uint32 hex)
{
    SDL_Color result = {};

    //NOTE(denis): hex colour format 0xAARRGGBB
    result.r = (hex >> 16) & 0xFF;
    result.g = (hex >> 8) & 0xFF;
    result.b = hex & 0xFF;
    result.a = hex >> 24;

    return result;
}

//TODO(denis): dunno if these belong here
static void initializeSelectionBox(SDL_Renderer *renderer,
				   TexturedRect *selectionBox, uint32 tileSize)
{
    *selectionBox = createFilledTexturedRect(renderer, tileSize, tileSize, 0x77FFFFFF);
}

static bool moveSelectionInRect(TexturedRect *selectionBox, Vector2 mousePos,
				SDL_Rect rect, uint32 tileSize)
{
    bool shouldBeDrawn = true;
    
    if (selectionBox->image != NULL)
    {
	if (pointInRect(mousePos, rect))
	{
	    Vector2 offset = {rect.x, rect.y};
	    Vector2 newPos = ((mousePos - offset)/tileSize) * tileSize + offset;
	    
	    selectionBox->pos.x = newPos.x;
	    selectionBox->pos.y = newPos.y;
	    selectionBox->pos.h = tileSize;
	    selectionBox->pos.w = tileSize;
	}
	else
	    shouldBeDrawn = false;
    }

    return shouldBeDrawn;    
}
