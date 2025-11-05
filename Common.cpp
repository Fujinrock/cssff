#include "Common.h"
#include "DemoParser.h"
#include <algorithm>
#include <Windows.h>
#include <assert.h>
#define _USE_MATH_DEFINES
#include <math.h>

double Log2( double n )
{
	// log( n ) / log( 2 ) == log2( n )
	return log( n ) / M_LN2;
}

void TrimStringLeft( std::string &input )
{
	input.erase( input.begin(), std::find_if_not( input.begin(), input.end(), []( unsigned char ch )
		{
			return std::isspace( ch );
		}
	) );
}

void TrimStringRight( std::string &input )
{
	input.erase( std::find_if_not( input.rbegin(), input.rend(), []( unsigned char ch )
		{
			return std::isspace( ch );
		}
	).base(), input.end() );
}

void TrimString( std::string &input )
{
	TrimStringLeft( input );	// Trim leading whitespaces
	TrimStringRight( input );	// Trim trailing whitespaces
}

void RemoveFileExtension( std::string &filename )
{
	size_t pos = filename.find_last_of( '.' );

	if( pos != std::string::npos )
	{
		filename.erase( pos );
	}
}

void RemoveFileNameFolders( std::string &filepath )
{
	size_t slash = filepath.find_last_of( '\\' );
	if( slash != std::string::npos )
		filepath.erase( 0, slash + 1 );
}

bool FileHasExtension( const std::string &filename, const std::string &extension )
{
	size_t pos = filename.find_last_of( '.' );

	if( pos != std::string::npos )
	{
		if( filename.substr( pos+1 ) == extension )
			return true;
	}

	return false;
}

bool IsValidDirectory( const char *szPath )
{
	DWORD attributes = GetFileAttributesA( szPath );

	return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

// The parser that is currently parsing a demo
// This can be used to get current tick or tick interval
extern DemoParser *gpParser;

float GetTimeBetweenTicks( int tick1, int tick2 )
{
	assert( gpParser );

	return gpParser->GetTimeBetweenTicks( tick1, tick2 );
}

int GetTotalTickCount( void )
{
	assert( gpParser );

	return gpParser->GetTickCount();
}

int GetCurrentTick( void )
{
	assert( gpParser );

	return gpParser->GetCurrentTick();
}

int GetTickRate( void )
{
	assert( gpParser );

	return gpParser->GetTickRate();
}

void OnParsingEnd( void )
{
	assert( gpParser );

	gpParser->OnParsingEnd();
}

// ===== QAngle ==========================================================================================

void QAngle::Init( void )
{
	x = y = z = 0.0f;
}

void QAngle::Init( float _x, float _y, float _z )
{
	x = _x;
	y = _y;
	z = _z;
}

// ===== Vector ==========================================================================================

Vector::Vector()
	: x(0), y(0), z(0)
{

}

Vector::Vector( float _x, float _y, float _z )
	: x( _x ), y( _y ), z( _z )
{

}

void Vector::Init( void )
{
	x = y = z = 0.0f;
}
void Vector::Init( float _x, float _y, float _z )
{
	x = _x;
	y = _y;
	z = _z;
}

Vector Vector::operator-( const Vector &other ) const
{
	Vector result;
	result.x = this->x - other.x;
	result.y = this->y - other.y;
	result.z = this->z - other.z;
	return result;
}

float Vector::Length( void ) const
{
	return sqrtf( x * x + y * y + z * z );
}

float& Vector::operator[](int i)
{
	assert( (i >= 0) && (i < 3) );
	return ((float*)this)[i];
}

float Vector::operator[](int i) const
{
	assert( (i >= 0) && (i < 3) );
	return ((float*)this)[i];
}