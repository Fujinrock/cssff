#pragma once

#if defined _WIN64
#error Build target should be set to Win32
#endif

#include <string>

#define	CSSFF_NAME					"CSSFF v2.1.16"

#define DEMO_HEADER_ID				"HL2DEMO"
#define DEMO_HEADER_ID_GOLDSRC		"HLDEMO"
#define DEMO_PROTOCOL				3			// 3 for almost all versions of CS:S
#define NETWORK_PROTOCOL_V34		7			// Always 7 in CS:S v34
#define NETWORK_PROTOCOL_NEW_MIN	14			// v77 minimum version
#define NETWORK_PROTOCOL_NEW_MAX	24			// Steam version, but unfortunately v77 had this version too
#define CS_GAMEDIR					"cstrike"	// Applies to CS:S and 1.6
#define CSGO_GAMEDIR				"csgo"		// For error messages

#define	FL_ONGROUND					1
#define MOVETYPE_LADDER				9
#define NOSCOPE_FOV					0

#define STATE_OBSERVER_MODE			6
#define OBS_MODE_IN_EYE				3			// 4 in new CS:S because they added freezecam

// Mid-air kill types
#define ON_GROUND					0
#define JUMPSHOT					1
#define LADDERSHOT					2

#define MAX_PLAYER_NAME_LENGTH		32

#ifndef MAX_PATH
#define MAX_PATH					260
#endif

#define BITS2BYTES( bits )			((bits+7)>>3)
#define BYTES2BITS( bytes )			(bytes<<3)

// For ofstream objects
#define WRITE_UTF8_BOM( file )		file.write( "\xEF\xBB\xBF", 3 )

typedef unsigned char				byte;
typedef __int32			 			int32;
typedef unsigned __int32			uint32;
typedef __int64						int64;
typedef unsigned long				CRC32_t;

enum ParsingResult;
struct ParsingError_t;

double Log2( double n );

void TrimString( std::string &s );
void RemoveFileExtension( std::string &filename );
void RemoveFileNameFolders( std::string &filepath );
bool FileHasExtension( const std::string &filename, const std::string &extension, bool allowPartial = false );
bool IsValidDirectory( const char *szPath );

// Utility from the current demo parser that other classes need as well
float GetTimeBetweenTicks( int, int );
int GetTotalTickCount( void );
int GetCurrentTick( void );
int GetTickRate( void );
int GetNumBytesLeft( void );
void OnParsingEnd( ParsingResult result, ParsingError_t *pError = nullptr );

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