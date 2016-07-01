#include "SDL_rect.h"
#include "denis_math.h"

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

inline bool pointInRect(Vector2 point, SDL_Rect rect)
{
    return point.x > rect.x && point.x < rect.x+rect.w &&
	point.y > rect.y && point.y < rect.y+rect.h;
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
