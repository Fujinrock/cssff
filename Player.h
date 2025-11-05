#pragma once

#include "Common.h"
#include "vector"
#include <list>

enum CSWeaponID;
struct Player;
struct PostCheckData_t;

typedef std::list<Player> PlayerList;
typedef std::vector< PostCheckData_t > PostCheckDataVector;

#define SIGNED_GUID_LEN 32
#define MAX_CUSTOM_FILES 4

#define INVALID_VIEWANGLE	-1337.f

struct angle_info_t
{
	angle_info_t( int _tick )
		: tick( _tick ),
		pitch( INVALID_VIEWANGLE ),
		yaw( INVALID_VIEWANGLE ) {}

	float pitch;
	float yaw;
	int tick;
};

enum PlayerAirStatus_e
{
	PL_ON_GROUND,			///< Not in air
	PL_IN_AIR_STARTED,		///< FL_ONGROUND was removed
	PL_WENT_UP_IN_AIR		///< m_vecOrigin Z increased while FL_ONGROUND was not set
};

struct PostCheckData_t
{
	Player *pPlayer;
	int nKillTick;
	bool bFlickChecked;
};

/**
 * Information related to a single kill
 */
struct kill_info_t
{
	int tick;
	byte team;
	bool teamkill;
	bool headshot;
	bool noscope;
	char midair;
	bool penetrated;
	bool flickshot;
	CSWeaponID weaponID;
	float distance;
	float flickangle;
	Vector position;
	bool spectated;
	bool blind;
};

/**
 * Represents a player in the game
 */
struct Player
{
	// Reference: public/cdll_int.h
	char name[ MAX_PLAYER_NAME_LENGTH ];
	int userID;
	char guid[ SIGNED_GUID_LEN + 1 ];
	uint32 friendsID;
	char friendsName[ MAX_PLAYER_NAME_LENGTH ];
	bool fakeplayer;
	bool ishltv;
	CRC32_t customfiles[ MAX_CUSTOM_FILES ];
	unsigned char filesDownloaded;

	// Fields below are not read from the stringtable updates directly

	int entityIndex;							///< EntityID is this - 1
	PlayerAirStatus_e airstatus;				///< For jumpshot detection
	float lastZ;								///< Last Z value in m_vecOrigin

private:
	std::vector< kill_info_t > roundkills;		///< Kills done by this player on the current round
public:
	Player( const Player &other );				///< Copy constructor for creating new players from string table updates
	void CopyFrom( const Player &src );			///< Copy all the fields from src that are directly read from the demo

	std::list< angle_info_t > viewangles;		///< Latest view angles of this player

	struct flashinfo_t
	{
		flashinfo_t() : tick( -9999 ), time( 0.f ) {}
		int tick;
		float time;
	}
	flashinfo;

	void AddKill( 
		int tick,
		char team,
		const char *weapon_name,
		bool teamkill,
		bool headshot,
		bool noscope,
		char midair,
		byte penetrated,
		bool flickshot,
		float distance,
		const Vector &position,
		bool spectated,
		bool blind );

	int GetNumKills( void ) const;

	const kill_info_t &GetKill( uint32 index ) const;
	kill_info_t &GetKill( uint32 index );

	void ResetKills( void );

	void AddPitchAngle( float pitch );
	void AddYawAngle( float yaw );

private:
	angle_info_t *GetAnglesByTick( int tick );
};