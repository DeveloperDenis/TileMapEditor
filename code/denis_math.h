#ifndef DENIS_MATH_H_
#define DENIS_MATH_H_

struct Vector2
{
    //TODO(denis): change these to floats?
    int x;
    int y;

    Vector2 operator+(const Vector2 & right)
    {
	Vector2 result = {};

	result.x = this->x + right.x;
	result.y = this->y + right.y;

	return result;
    }
    
    Vector2 operator-(const Vector2 & right)
    {
	Vector2 result = {};
	
	result.x = this->x - right.x;
	result.y = this->y - right.y;
	
	return result;
    }

    //TODO(denis): change into float math?
    Vector2 operator*(const int scalar)
    {
	Vector2 result = {};
	
	result.x = this->x * scalar;
	result.y = this->y * scalar;
	
	return result;
    }
    
    //TODO(denis): change this into float math and call operator* with 1/scalar?
    Vector2 operator/(const int scalar)
    {
	Vector2 result = {};
	
	result.x = this->x / scalar;
	result.y = this->y / scalar;
	
	return result;
    }

    Vector2 & operator=(const Vector2 & right)
    {
	this->x = right.x;
	this->y = right.y;

	return *this;
    }

    bool operator!=(const Vector2 right)
    {
	return this->x != right.x || this->y != right.y;
    }
};


inline int absValue(int value)
{
    return value >= 0? value:-value;
}

#endif
