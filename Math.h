#include <cmath>

struct vec2
{
	vec2() {}
	vec2( float x, float y ) : x(x), y(y) {}

	vec2 operator + ( const vec2& other ) const
	{
		vec2 result = *this;
		result.x += other.x;
		result.y += other.y;
		return result;
	}

	vec2 operator - ( const vec2& other ) const
	{
		vec2 result = *this;
		result.x -= other.x;
		result.y -= other.y;
		return result;
	}

	vec2 operator * ( float s ) const
	{
		vec2 result = *this;
		result.x *= s;
		result.y *= s;
		return result;
	}

	vec2& operator += ( const vec2& other )
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	vec2& operator -= ( const vec2& other )
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	vec2& operator *= ( float s )
	{
		x *= s;
		y *= s;
		return *this;
	}

	float x;
	float y;
};

float dot( const vec2& a, const vec2& b )
{
	return a.x * b.x + a.y * b.y;
}

float length( const vec2& v )
{
	return sqrtf( v.x * v.x + v.y * v.y );
}
