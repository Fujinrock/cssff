#pragma once

#include "Common.h"

/**
 * Demo messages
 */
enum
{
	dem_signon	= 1,						// A startup message, process as fast as possible
	dem_packet,								// A normal network packet that we stored off
	dem_synctick,							// Sync client clock to demo tick
	dem_consolecmd,							// Console command
	dem_usercmd,							// User input command
	dem_datatables,							// Network data tables
	dem_stop,								// End of demo
	dem_firstcmd = dem_signon,				// First command
	dem_lastcmd = dem_stop,					// Last command
};

/**
 * Data written at the very beginning of a demo file
 */
struct demoheader_t
{
	char	demofilestamp[ 8 ];				// Should be HL2DEMO
	int32	demoprotocol;					// Should be 3
	int32	networkprotocol;				// Should be 7 (for v34)
	char	servername[ MAX_PATH ];			// Name of server
	char	clientname[ MAX_PATH ];			// Name of client who recorded the game
	char	mapname[ MAX_PATH ];			// Name of map
	char	gamedirectory[ MAX_PATH ];		// Name of game directory (com_gamedir)
	float	playback_time;					// Time of track
	int32   playback_ticks;					// # of ticks in track
	int32   playback_frames;				// # of frames in track
	int32	signonlength;					// Length of sigondata in bytes
};

/**
 * Data contained at the beginning of packets
 */
struct democmdinfo_t
{
	int32		flags;

	// Original origin/viewangles
	Vector		viewOrigin;
	QAngle		viewAngles;
	QAngle		localViewAngles;

	// Resampled origin/viewangles
	Vector		viewOrigin2;
	QAngle		viewAngles2;
	QAngle		localViewAngles2;
};

/**
 * Possible errors while trying to open a demo
 */
enum DemoError
{
	DEMO_OK	= 0,
	COULD_NOT_OPEN_FILE,
	FILE_TOO_SMALL,
	INVALID_HDR_ID,
	INVALID_DEM_PROTOCOL,
	INVALID_NET_PROTOCOL,
	INVALID_GAMEDIR,
};

/**
 * Holds the raw data of a demo file
 */
class DemoFile
{
public:
	DemoFile( const std::string &filename );
	~DemoFile( void );

	bool				IsValidDemo( void ) const;	///< Is this demo a valid CS:S v34 demo
	DemoError			GetError( void ) const;		///< Get the error ID if the demo is invalid
	char *				GetBuffer( void ) const;	///< Get the raw contents of the demo
	std::string			GetFileName( void ) const;	///< Get the file name without folders
	uint32				GetFileSize( void ) const;	///< Get the file size in bytes

private:
	// No copying allowed due to dynamic memory
	DemoFile( const DemoFile & );
	DemoFile &operator=( const DemoFile & );

	void				CheckValidity( void );

	DemoError			m_error;

	char *				m_filebuffer;
	std::string			m_filename;
	uint32				m_filesize;
};