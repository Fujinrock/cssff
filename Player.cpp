#include "Player.h"
#include "DemoParser.h"
#include "Weapons.h"
#include "Settings.h"
#include <assert.h>
#include <string>

// =====================================================================================================================================================================

Player::Player( const Player &other )
{
	CopyFrom( other );
}

// =====================================================================================================================================================================

void Player::CopyFrom( const Player &src )
{
	strcpy_s( name, sizeof(name), src.name);
	strcpy_s( guid, sizeof(guid), src.guid );
	strcpy_s( friendsName, sizeof(friendsName), src.friendsName );

	userID = src.userID;
	friendsID = src.friendsID;
	fakeplayer = src.fakeplayer;
	ishltv = src.ishltv;
	filesDownloaded = src.filesDownloaded;

	for( int i = 0; i < MAX_CUSTOM_FILES; ++i )
		customfiles[i] = src.customfiles[i];

	entityIndex = src.entityIndex;
	airstatus = src.airstatus;
	lastZ = src.lastZ;
}

// =====================================================================================================================================================================

void Player::AddKill(
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
	bool blind )
{
	kill_info_t info;
	info.tick = tick;
	info.team = team;
	info.weaponID = AliasToWeaponID( weapon_name );
	info.teamkill = teamkill;
	info.headshot = headshot;
	info.noscope = noscope;
	info.midair = midair;
	info.penetrated = penetrated != 0;
	info.flickshot = flickshot;
	info.distance = distance;
	info.position = position;
	info.spectated = spectated;
	info.blind = blind;

	roundkills.push_back( info );
}

// =====================================================================================================================================================================

int Player::GetNumKills( void ) const
{
	return roundkills.size();
}

// =====================================================================================================================================================================

const kill_info_t &Player::GetKill( uint32 index ) const
{
	assert( index < roundkills.size() );
	return roundkills[ index ];
}

// =====================================================================================================================================================================

kill_info_t &Player::GetKill( uint32 index )
{
	assert( index < roundkills.size() );
	return roundkills[ index ];
}

// =====================================================================================================================================================================

void Player::ResetKills( void )
{
	roundkills.clear();
}

// =====================================================================================================================================================================

void Player::AddPitchAngle( float pitch )
{
	angle_info_t *angles = GetAnglesByTick( GetCurrentTick() );
	assert( angles );
	angles->pitch = pitch;
}

// =====================================================================================================================================================================

void Player::AddYawAngle( float yaw )
{
	angle_info_t *angles = GetAnglesByTick( GetCurrentTick() );
	assert( angles );
	angles->yaw = yaw;
}

// =====================================================================================================================================================================

angle_info_t *Player::GetAnglesByTick( int tick )
{
	for( auto it = viewangles.begin(); it != viewangles.end(); ++it )
	{
		if( it->tick == tick )
			return &(*it);
	}

	// Delete expired angles
	const int max_duration = Settings()->GetMaxFlickshotDuration();
	const int flick_ticks = (int)ceil( GetTickRate() * (max_duration / 1000.f) );

	bool exceeds_max = false;
	for( auto it = viewangles.begin(); it != viewangles.end(); ++it )
	{
		// We leave one expired angle info, because the angles might not be updated every tick,
		// so the flick end tick might be between the last valid and the first expired infos
		if( exceeds_max )
		{
			viewangles.erase( it, viewangles.end() );
			break;
		}
		else if( GetCurrentTick() - it->tick >= flick_ticks )
		{
			exceeds_max = true;
		}
	}

	// Add new angles
	viewangles.emplace_front( tick );

	return &viewangles.front();
}

// =====================================================================================================================================================================

Player *DemoParser::FindPlayerByEntityIndex( int entityIndex )
{
	for( auto it = m_Players.begin(); it != m_Players.end(); ++it )
	{
		if( it->entityIndex == entityIndex )
		{
			return &(*it);
		}
	}

	return nullptr;
}

// =====================================================================================================================================================================

Player *DemoParser::FindPlayerByUserId( int userId )
{
	for( auto it = m_Players.begin(); it != m_Players.end(); ++it )
	{
		if( it->userID == userId )
		{
			return &(*it);
		}
	}

	return nullptr;
}

// =====================================================================================================================================================================

typedef char axis_t;
enum { PITCH, YAW };

bool AngleDeltaIsFlickOnAxis( axis_t axis, int iMaxFlickTicks, float fDistance, const std::list< angle_info_t > &angles, float *pFlickAmount = nullptr )
{
	auto it = angles.begin();

	if( pFlickAmount )
		*pFlickAmount = 0.f;

	if( it == angles.end() )
		return false;

	// Find the last valid angle on this axis (if it wasn't updated after, it's assumed to be the same)
	while( (axis == YAW? it->yaw : it->pitch) == INVALID_VIEWANGLE )
	{
		if( ++it == angles.end() )
			return false;
	}

	float fFlickshotAbsoluteMinAngle;		// Minimum angle can't be less than this
	float fFlickshotAbsoluteMaxMinAngle;	// Maximum angle can't be more than this
	float fMinFlickAngle;					// The minimum angle delta for this to be a flick
	float fFlickBaseAngle;
	int iLog2Multiplier;

	// Scale values per axis, since the range of movement is different
	if( axis == PITCH )
	{
		fFlickshotAbsoluteMinAngle = 25.f;
		fFlickshotAbsoluteMaxMinAngle = 110.f;
		fFlickBaseAngle = 90.f;

		iLog2Multiplier = 6;

		// Use shorter flick length on pitch axis
		iMaxFlickTicks = (int)ceil( iMaxFlickTicks * 0.65 );
	}
	else // axis == YAW
	{
		fFlickshotAbsoluteMinAngle = 35.f;
		fFlickshotAbsoluteMaxMinAngle = 180.f;
		fFlickBaseAngle = 180.f;

		// Distance scaling
		iLog2Multiplier = 17;
	}

	// Distance scaling of min angle
	fMinFlickAngle = fFlickBaseAngle - (float)Log2( fDistance/10.f ) * iLog2Multiplier;

	// Clamp min angle
	if( fMinFlickAngle < fFlickshotAbsoluteMinAngle )
		fMinFlickAngle = fFlickshotAbsoluteMinAngle;
	else if( fMinFlickAngle > fFlickshotAbsoluteMaxMinAngle )
		fMinFlickAngle = fFlickshotAbsoluteMaxMinAngle;

	float fPrevAngle = (axis == YAW)? it->yaw : it->pitch;

	// Reset to newest tick to count checked ticks correctly
	it = angles.begin();

	float fTotalAngleDelta = 0.f;	// How much this player has turned on this axis during checked ticks
	int iLastTick = it->tick;		// Last tick that was checked
	int iTicksChecked = 0;			// How many ticks' angle delta has been added to total angle delta

	// Check if the player's view moved enough to be a flick
	for( ++it; it != angles.end(); ++it )
	{
		float fNewAngle = (axis == YAW)? it->yaw : it->pitch;
		const int iTickDelta = iLastTick - it->tick; // How many ticks' angles are we adding on this update

		// Was there delta on this tick?
		if( fNewAngle == INVALID_VIEWANGLE )
		{
			iTicksChecked += iLastTick - it->tick;
			iLastTick = it->tick;
			continue;
		}

		float fDelta = fPrevAngle - fNewAngle;

		// Normalize the angle difference
		if( fDelta > 180.f )
			fDelta -= 360.f;
		else if( fDelta < -180.f )
			fDelta += 360.f;

		// If we went past max flick ticks, scale delta amount by the % of valid ticks
		if( iTicksChecked + iTickDelta > iMaxFlickTicks )
		{
			int iValidTicks = iMaxFlickTicks - iTicksChecked;

			fDelta *= ((float)iValidTicks / iTickDelta);
		}

		fTotalAngleDelta += fDelta;
		fPrevAngle = fNewAngle;

		iTicksChecked += iTickDelta;
		iLastTick = it->tick;

		if( iTicksChecked >= iMaxFlickTicks )
			break;
	}

	if( pFlickAmount )
		*pFlickAmount = fTotalAngleDelta;

	return fabs(fTotalAngleDelta) >= fMinFlickAngle;
}

// =====================================================================================================================================================================

bool KillIsFlickshot( const Player *player, kill_info_t &kill )
{
	CSWeaponCategory category = GetWeaponCategory( kill.weaponID );

	if( category == CATEGORY_GRENADE )
		return false;

	const int flick_duration_ms = Settings()->GetFlickshotDurationForCategory( category );

	if( flick_duration_ms <= 0 )
		return false;

	// Flick duration in ticks
	int flick_ticks = (int)ceil( GetTickRate() * (flick_duration_ms / 1000.f) );

	// Check on both axes
	if( AngleDeltaIsFlickOnAxis( YAW, flick_ticks, kill.distance, player->viewangles, &kill.flickangle ) )
		return true;

	if( AngleDeltaIsFlickOnAxis( PITCH, flick_ticks, kill.distance, player->viewangles, &kill.flickangle ) )
		return true;

	return false;
}

// =====================================================================================================================================================================

void DemoParser::DoPlayersPostCheck()
{
	for( auto it = m_PlayerPostCheckData.begin(); it != m_PlayerPostCheckData.end(); ++it )
	{
		PostCheckData_t &data = *it;
		Player *player = data.pPlayer;

		const int num_kills = player->GetNumKills();
		float deltatime = GetTimeBetweenTicks( data.nKillTick, m_iCurrentTick );

		if( deltatime < 0.0 )
			deltatime = 0.0;

		bool is_flickshot = false;
		char midair_status = ON_GROUND;
		bool bCheckedFlick = false;
		bool bCheckedMidAir = false;
		float flickangle = 0.f;
		float min_air_time = -1.f;
#ifdef _DEBUG
		CSWeaponCategory category = CATEGORY_NONE;
#endif

		// TODO: what if the player dies before the check is complete?
		for( int k = num_kills-1; k >= 0; --k )
		{
			kill_info_t &kill = player->GetKill( k );

			if( kill.tick < data.nKillTick )
				break;

			if( kill.tick != data.nKillTick )
				continue;

			CSWeaponCategory curCategory = GetWeaponCategory( kill.weaponID );

			if( curCategory == CATEGORY_GRENADE || curCategory == CATEGORY_KNIFE )
				continue;
#ifdef _DEBUG
			// Shouldn't be able to enter here with a different category than before
			assert( (category == CATEGORY_NONE) || (category == curCategory) );
			category = curCategory;
#endif
			if( kill.midair != ON_GROUND )
			{
				min_air_time = Settings()->GetMinPostKillAirTimeForCategory( curCategory );
				if( min_air_time < 0.0 )
					min_air_time = 0.0;
			}

			if( bCheckedMidAir )
			{
				kill.midair = midair_status;
			}
			else if( kill.midair != ON_GROUND && !kill.spectated ) // It had to be a mid-air kill previously, too
			{
				EntityEntry *pEntAttacker = FindEntity( player->entityIndex );
				PropEntry *prop = pEntAttacker->FindProp( "m_fFlags" );

				// This CAN be NULL, if the flags never had to be updated before a kill was made
				if( prop )
				{
					uint32 flags = prop->m_pPropValue->m_value.m_int;

					if( !(flags & FL_ONGROUND) )
					{
						// Don't change jumpshot to a ladderkill or the other way around
						midair_status = kill.midair;
					}
				}

				kill.midair = midair_status;
				bCheckedMidAir = true;
			}

			// Flicks are only checked/set on the first check
			if( !data.bFlickChecked )
			{
				if( is_flickshot )
				{
					kill.flickshot = true;
					kill.flickangle = flickangle;
					continue;
				}
				else if( !bCheckedFlick )
				{
					is_flickshot = KillIsFlickshot( player, kill );
					kill.flickshot = is_flickshot;
					flickangle = kill.flickangle;
					bCheckedFlick = true;
				}
			}
		}

		assert( !bCheckedMidAir || min_air_time >= 0.0 );

		data.bFlickChecked = true;

		if( midair_status == ON_GROUND || deltatime >= min_air_time )
		{
			it = m_PlayerPostCheckData.erase( it );

			if( it == m_PlayerPostCheckData.end() )
				break;
		}
	}
}


// =====================================================================================================================================================================
