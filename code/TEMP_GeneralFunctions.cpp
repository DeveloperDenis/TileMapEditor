#include "SDL_rect.h"
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
