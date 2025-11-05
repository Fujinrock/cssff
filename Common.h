#pragma once

#include <string>

#define	CSSFF_NAME					"CSSFF v2.0.0"

#define DEMO_HEADER_ID				"HL2DEMO"
#define DEMO_PROTOCOL				3			// 3 for all versions of CS:S
#define NETWORK_PROTOCOL_V34		7			// Always 7 in CS:S v34
#define NETWORK_PROTOCOL_NEW_MIN	14			// v77 minimum version
#define NETWORK_PROTOCOL_NEW_MAX	24			// Steam version, but unfortunately v77 had this version too
#define CSS_GAMEDIR					"cstrike"

#define	FL_ONGROUND					1
#define MOVETYPE_LADDER				9
#define NOSCOPE_FOV					0

#define STATE_OBSERVER_MODE			6
#define OBS_MODE_IN_EYE				3			// 4 in new CS:S because they added freezecam

// Mid-air kill types
#define ON_GROUND					0
#define IN_AIR						1
#define ON_LADDER					2

#define MAX_PLAYER_NAME_LENGTH		32

#ifndef MAX_PATH
#define MAX_PATH					260
#endif

#define BITS2BYTES( bits )			((bits+7)>>3)
#define BYTES2BITS( bytes )			(bytes<<3)

typedef unsigned char				byte;
typedef __int32			 			int32;
typedef unsigned __int32			uint32;
typedef __int64						int64;
typedef unsigned long				CRC32_t;

double Log2( double n );

void TrimString( std::string &s );
void RemoveFileExtension( std::string &filename );
void RemoveFileNameFolders( std::string &filepath );
bool FileHasExtension( const std::string &filename, const std::string &extension );
bool IsValidDirectory( const char *szPath );

// Utility from the current demo parser that other classes need as well
float GetTimeBetweenTicks( int, int );
int GetTotalTickCount( void );
int GetCurrentTick( void );
int GetTickRate( void );
void OnParsingEnd( void );

struct QAngle
{
	float x, y, z;
	void Init( void );

	void Init( float _x, float _y, float _z );
};

struct Vector
{
	Vector();
	Vector( float _x, float _y, float _z );

	void Init( void );
	void Init( float _x, float _y, float _z );

	Vector operator-( const Vector &other ) const;

	float Length( void ) const;

	float &operator[]( int i );
	float operator[]( int i ) const;

	float x, y, z;
};