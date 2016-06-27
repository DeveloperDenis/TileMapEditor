#include "SDL_rect.h"

//TODO(denis): there is a faster way I'm pretty sure
inline int exponent(int base, int power)
{
    int result = 1;
    
    for (int i = 0; i < power; ++i)
    {
	result *= base;
    }

    return result;
}

inline bool pointInRect(int x, int y, SDL_Rect rect)
{
    return x >= rect.x && x <= rect.x+rect.w &&
	y >= rect.y && y <= rect.y+rect.h;
}

inline int convertStringToInt(char string[], int size)
{
    int result = 0;

    for (int i = 0; i < size; ++i)
    {
	int num = (string[i]-'0') * exponent(10, size-1-i);
	result += num;
    }

    return result;
}
