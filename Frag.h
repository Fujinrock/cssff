#pragma once

#include "Common.h"
#include <vector>

enum CSWeaponID;
class Frag;

typedef std::vector< Frag > FragVector;

// Frag type flags
#define FL_KILL_DOUBLE			(1<<0)
#define FL_KILL_TRIPLE			(1<<1)
#define FL_KILL_QUADRO			(1<<2)
#define FL_KILL_PENTA			(1<<3)
#define FL_KILL_FLASHKILL		(1<<4)
#define FL_KILL_SMOKEKILL		(1<<5)
#define FL_KILL_MIDAIR			(1<<6)
#define FL_KILL_FLICKSHOT		(1<<7)
#define FL_KILL_NOSCOPE			(1<<8)
#define FL_KILL_WALLBANG		(1<<9)
#define FL_KILL_BLIND			(1<<10)
#define FL_KILL_LADDERSHOT		(1<<11)

#define MASK_COLLATS			0xF
#define MASK_PROJECTILE_KILLS	0x30

#define FragIsCollat( flags )	((flags & MASK_COLLATS) != 0)

#define INVALID_TICK	~0

enum MultiKillFragType
{
	FRAG_NONE = 0,
	FRAG_3K = 3,
	FRAG_4K = 4,
	FRAG_5K = 5,
};

/**
 * Descriptor for all the 1k frags and collaterals
 */
struct frag_descriptor_t
{
	unsigned short type_flags;		///< Flags describing this frag (for example "noscope mid-air double")
	short headshots;				///< How many headshots are in this frag
	short teamkills;				///< How many teamkills are in this frag
	short count;					///< How many times this frag type was done (for example "2 doubles")
	CSWeaponID weapon;				///< What weapon was used to do this frag
	float flick_angle;				///< Only relevant if flick flag is set (for string output)

	// Get a description of this action in the form of text (for output)
	void GetStringRepresentation( char *buffer, size_t buffer_size, bool write_hs = true, bool write_weapon = true ) const;

private:
	int GetRoundedFlickAngle( void ) const;
};

/**
 * Descriptor for 3/4/5ks - a frag can only contain one of these
 */
struct multi_kill_frag_descriptor_t
{
	MultiKillFragType frag_type;	///< Is this a 3k, 4k or a 5k?
	short headshots;				///< How many headshots are in this frag
	uint32 start_tick;				///< Tick of the first kill done
	uint32 end_tick;				///< Tick of the last kill done
	float frag_length;				///< Frag length in seconds

	enum { FRAG_MAX_WEAPONS = 5 };
	CSWeaponID weapons[ FRAG_MAX_WEAPONS ];
	short num_weapons;

	// Extra descriptors for a more detailed description of this frag
	std::vector< frag_descriptor_t > sub_descriptors;

	multi_kill_frag_descriptor_t() { Reset(); }

private:
	enum { PRINT_HS_IN_SUB, PRINT_HS_IN_MAIN, PRINT_HS_IN_BOTH };
public:
	void GetStringRepresentation( char *buffer, size_t buffer_size ) const;

	void AddWeaponIDs( const CSWeaponID *pWeapons, int num );

	void ClearWeapons( void );

	bool IsValid( void ) const;

	void Reset( void );
};

/**
 * Representation of a single frag, which can consist of 1 or more kills
 */
class Frag
{
public:
	Frag( short total_kills, byte team, bool spectated );

	void SetPlayername( const char *playername );

	MultiKillFragType GetMultiKillFragType( void ) const;

	// Try to add a new 5/4/3k descriptor to this frag, possibly deleting the previous descriptor if one exists and this is a better frag
	bool AddMultiKillFragDescriptor( MultiKillFragType type, const CSWeaponID *weapons, short num_weapons, uint32 start_tick, uint32 end_tick, short headshots );

	// Add a descriptor for a 1k/collat frag
	void AddFragDescriptor( uint32 tick, unsigned short type_flags, short teamkills, short headshots, CSWeaponID weapon, float distance, float flickangle, float time_to_closest_kill );

	bool IsValidFrag( void ) const;

	void GetStringRepresentation( char *buffer, size_t buffer_size ) const;

private:
	// Get the amount of enemy kills that the descriptors in this frag imply
	int GetImpliedKillCount( void ) const;

	enum { TEAM_T = 2, TEAM_CT = 3 };

	const char *GetTeamString( void ) const;

	// Get a nice rounded tick for string
	int GetRoundedTick( void ) const;

	char m_szPlayername[ MAX_PLAYER_NAME_LENGTH ];			///< Name is copied in case the player leaves before the demo ends
	multi_kill_frag_descriptor_t m_multiKillDescriptor;		///< The 5/4/3k descriptor of this frag, if any
	std::vector< frag_descriptor_t > m_descriptors;			///< What kind of other smaller frags this frag contains that are not contained within the 5/4/3k
	uint32 m_nStartTick;									///< Tick pointing to the earliest kill of the frag
	short m_nTotalKills;									///< How many enemies the fragger killed during the round (this should not include teamkills)
	byte m_nTeam;											///< Team number of the fragger
	bool m_bSpectated;										///< Whether this frag was spectated by the POV player
};