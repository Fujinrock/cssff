#include "GameEvents.h"
#include "DemoParser.h"
#include "Entities.h"
#include "Errors.h"
#include "Settings.h"
#include "Weapons.h"
#include <string>

// =====================================================================================================================================================================

GameEvent::GameEvent( int id, const char *name )
{
	event_id = id;
	strncpy_s( event_name, sizeof(event_name), const_cast<char*>(name), sizeof(event_name) );
}

// =====================================================================================================================================================================

void GameEvent::AddField( const char *name, GameEventValueType type )
{
	event_fields.emplace_back( name, type );
}

// =====================================================================================================================================================================

const GameEvent::GameEventField &GameEvent::GetField( uint32 index ) const
{
	if( index >= event_fields.size() )
		throw ParsingError_t( "invalid game event field index" );

	return event_fields[ index ];
}

// =====================================================================================================================================================================

size_t GameEvent::GetFieldCount( void ) const
{
	return event_fields.size();
}

// =====================================================================================================================================================================

GameEvent::GameEventField::GameEventField( const char *name, GameEventValueType type )
{
	strncpy_s( field_name, sizeof(field_name), const_cast<char*>(name), sizeof(field_name) );
	field_type = type;
}

// =====================================================================================================================================================================

const GameEvent &DemoParser::GetGameEvent( uint32 id )
{
	// Event ID should be the same as the event's index in the vector
	if( !(id < m_GameEvents.size() && m_GameEvents[ id ].event_id == id) )
		throw ParsingError_t( "invalid game event ID" );

	return m_GameEvents[ id ];
}

// =====================================================================================================================================================================

void DemoParser::ParseGameEvent( bf_read &reader )
{
	int eventID = reader.ReadUBitLong( 9 );

	char name[ 32 ];
	if( !reader.ReadString( name, sizeof(name) ) )
		throw ParsingError_t( "invalid event name in SVC_GameEventList" );

	GameEvent game_event( eventID, name );

	GameEventValueType value_type = (GameEventValueType)reader.ReadUBitLong( 3 );

	while( value_type != VAL_NONE )
	{
		char fieldname[ 32 ];
		if( !reader.ReadString( fieldname, sizeof(fieldname) ) )
			throw ParsingError_t( "invalid event field name in SVC_GameEventList" );

		game_event.AddField( fieldname, value_type );

		value_type = (GameEventValueType)reader.ReadUBitLong( 3 );
	}

	m_GameEvents.push_back( game_event );
}

// =====================================================================================================================================================================

void ReadGameEventFieldValue( bf_read &reader, const GameEvent::GameEventField &field )
{
	switch( field.field_type )
	{
		case VAL_NONE:
		{
			break;
		}
		case VAL_STRING:
		{
			char data[ 128 ];
			reader.ReadString( data, sizeof(data) );
			break;
		}
		case VAL_FLOAT:
		{
			reader.ReadFloat();
			break;
		}
		case VAL_LONG:
		{
			reader.ReadLong();
			break;
		}
		case VAL_SHORT:
		{
			reader.ReadShort();
			break;
		}
		case VAL_BYTE:
		{
			reader.ReadByte();
			break;
		}
		case VAL_BOOL:
		{
			reader.ReadOneBit();
			break;
		}
		default:
		{
			throw ParsingError_t( "unknown field type in game event" );
			break;
		}
	}
}

// =====================================================================================================================================================================

void DemoParser::HandleGameEvent( bf_read &reader )
{
	int eventID = reader.ReadUBitLong( 9 );

	const GameEvent &event = GetGameEvent( eventID );

	const char *name = event.event_name;

	// Check if we have a special handler for this event

	if( strcmp( name, "player_death" ) == 0 )
	{
		// Ignore kills done at the very beginning of the demo, because some player info might still be missing
		if( m_iCurrentTick > (int)( m_iTickRate * 0.5 ) )
		{
			HandlePlayerDeathEvent( reader, event );
			return;
		}
	}
	else if( strcmp( name, "round_start" ) == 0 )
	{
		OnRoundStartEvent();
	}
	else if( strcmp( name, "player_disconnect" ) == 0 )
	{
		HandlePlayerDisconnectEvent( reader, event );
		return;
	}
	else if( m_bIsPOV && strcmp( name, "player_spawn" ) == 0 )
	{
		HandlePlayerSpawnEvent( reader, event );
		return;
	}

	// A generic event - handle it here

	size_t fields = event.GetFieldCount();

	for( size_t i = 0; i < fields; ++i )
	{
		const GameEvent::GameEventField &field = event.GetField( i );

		ReadGameEventFieldValue( reader, field );
	}
}

// =====================================================================================================================================================================

void DemoParser::HandlePlayerSpawnEvent( bf_read &reader, const GameEvent &event )
{
	size_t fields = event.GetFieldCount();

	for( size_t i = 0; i < fields; ++i )
	{
		const GameEvent::GameEventField &field = event.GetField( i );

		if( strcmp( field.field_name, "userid" ) == 0 )
		{
			int userid = reader.ReadShort();

			if( userid == m_iPOVPlayerUserID )
				m_bPOVPlayerIsDead = false;
		}
		else
		{
			ReadGameEventFieldValue( reader, field );
		}
	}
}

// =====================================================================================================================================================================

void DemoParser::HandlePlayerDeathEvent( bf_read &reader, const GameEvent &event )
{
	if( m_bIsPOV && m_iPOVPlayerUserID < 0 )
	{
		AddWarning( m_pDemo->GetFileName(), POV_PLAYER_NOT_FOUND );
	}

	int fields = event.GetFieldCount();

	char weaponName[ 24 ] = "unknown";

	kill_info_t killInfo;
	memset( &killInfo, 0, sizeof(killInfo) );

	Player *pAttacker = nullptr;
	Player *pVictim = nullptr;

	for( int i = 0; i < fields; ++i )
	{
		const GameEvent::GameEventField &field = event.GetField( i );

		if( strcmp( field.field_name, "userid" ) == 0 )
		{
			pVictim = FindPlayerByUserId( reader.ReadShort() );
		}
		else if( strcmp( field.field_name, "attacker" ) == 0 )
		{
			pAttacker = FindPlayerByUserId( reader.ReadShort() );
		}
		else if( strcmp( field.field_name, "weapon" ) == 0 )
		{
			reader.ReadString( weaponName, sizeof(weaponName) );
		}
		else if( strcmp( field.field_name, "headshot" ) == 0 )
		{
			killInfo.headshot = reader.ReadOneBit();
		}
		else if( strcmp( field.field_name, "penetrated" ) == 0 ) // Clientmod only
		{
			killInfo.penetrated = reader.ReadByte();
		}
		else if( strcmp( field.field_name, "noscope" ) == 0 ) // Clientmod only
		{
			killInfo.noscope = reader.ReadOneBit();
		}
		else // In Clientmod and new CS:S there are additional fields
		{
			ReadGameEventFieldValue( reader, field );
		}
	}

	if( !pVictim )
		throw ParsingError_t( "victim info not found in HandlePlayerDeathEvent" );

	if( m_bIsPOV && pVictim->userID == m_iPOVPlayerUserID )
	{
		m_bPOVPlayerIsDead = true;
	}

	bool bSpectatingAttacker = false;
	bool bSuicide = !pAttacker || pAttacker == pVictim;

#ifdef _DEBUG_AFK_KILLS
	if( !bSuicide && pVictim->activity == PL_AFK )
	{
		printf( "\nVictim %s was AFK at tick %d\n\n", pVictim->name, m_iCurrentTick );
	}
	else if( !bSuicide && pAttacker->activity == PL_AFK )
	{
		printf( "\nAttacker %s was AFK at tick %d\n\n", pAttacker->name, m_iCurrentTick );
	}
#endif

	// Ignore suicides and kills vs players who are AFK
	// Also ignore bot kills if the settings disallow them
	if( bSuicide
	|| pVictim->activity == PL_AFK
	|| (pVictim->fakeplayer && !Settings()->ShouldTickFragsVsBots())
	|| (pAttacker->fakeplayer && !Settings()->ShouldTickFragsByBots()) )
	{
		return;
	}

	// Check if the POV recorder is spectating the other player doing the kill
	if( m_bIsPOV && m_bPOVPlayerIsDead && pAttacker->userID != m_iPOVPlayerUserID )
	{
		Player *pPOVPlayer = FindPlayerByUserId( m_iPOVPlayerUserID );

		if( !pPOVPlayer )
			throw ParsingError_t( "POV player info not found in HandlePlayerDeathEvent" );

		EntityEntry *pPOVPlayerEntity = FindEntity( pPOVPlayer->entityIndex );

		if( !pPOVPlayerEntity )
			throw ParsingError_t( "POV player entity not found in HandlePlayerDeathEvent" );

		PropEntry *prop = pPOVPlayerEntity->FindProp( "m_iPlayerState" );

		if( !prop || prop->m_pPropValue->m_value.m_int == STATE_OBSERVER_MODE )
		{
			prop = pPOVPlayerEntity->FindProp( "m_iObserverMode" );

			int observer_mode = prop? prop->m_pPropValue->m_value.m_int : OBS_MODE_IN_EYE; // Assume in-eye if prop not found

			prop = pPOVPlayerEntity->FindProp( "m_hObserverTarget" );

			if( prop && observer_mode == OBS_MODE_IN_EYE )
			{
				int spectated_player_entidx = GetEntIndexFromEHandleInt( prop->m_pPropValue->m_value.m_int );

				Player *pSpectated = FindPlayerByEntityIndex( spectated_player_entidx );

				if( pSpectated && pSpectated->userID == pAttacker->userID )
					bSpectatingAttacker = true;
			}
		}
	}

	// Don't bother adding kills on POV demos that are not seen by the recorder
	if( m_bIsPOV && pAttacker->userID != m_iPOVPlayerUserID && !bSpectatingAttacker )
	{
		return;
	}

	// This is a valid kill - check for attributes and register it

	EntityEntry *pEntAttacker = FindEntity( pAttacker->entityIndex );
	EntityEntry *pEntVictim = FindEntity( pVictim->entityIndex );

	if( !pEntAttacker )
		throw ParsingError_t( "Attacker entity not found in HandlePlayerDeathEvent" );

	bool bullet_kill = WeaponUsesBullets( weaponName );

	// Add bullet kills to post-check list
	if( bullet_kill )
	{
		bool bAlreadyIn = false;
		for( size_t i = 0; i < m_PlayerPostCheckData.size(); ++i )
		{
			if( m_PlayerPostCheckData[ i ].pPlayer == pAttacker
			&& m_PlayerPostCheckData[ i ].nKillTick == m_iCurrentTick )
			{
				bAlreadyIn = true;
				break;
			}
		}

		if( !bAlreadyIn )
		{
			PostCheckData_t data;
			data.pPlayer = pAttacker;
			data.nKillTick = m_iCurrentTick;
			data.bFlickChecked = false;
			m_PlayerPostCheckData.push_back( data );
		}
	}

	PropEntry *prop;

	// ===== Teamkill check ====================
	prop = pEntAttacker->FindProp( "m_iTeamNum" );

	// Attacker team prop MUST be found
	if( !prop )
	{
		if( !bSpectatingAttacker )
		{
			throw ParsingError_t( "Attacker team prop not found in HandlePlayerDeathEvent" );
		}
		else // Sometimes spectated players are a bit buggy props-wise, so just add a warning
		{
			AddWarning( m_pDemo->GetFileName(), SPEC_ATTACKER_TEAM_NOT_FOUND );
			return;
		}
	}

	killInfo.team = prop->m_pPropValue->m_value.m_int;

	killInfo.teamkill = false;

	// Victim entity might be NULL, because it's possible the player got killed before ever entering the PVS
	if( pEntVictim )
	{
		prop = pEntVictim->FindProp( "m_iTeamNum" );
		if( prop )
		{
			killInfo.teamkill = (killInfo.team == prop->m_pPropValue->m_value.m_int);
		}
		else
		{
			AddWarning( m_pDemo->GetFileName(), VICTIM_TEAM_NOT_FOUND );
		}
	}

	// ===== Flashed check ====================
	killInfo.blind = false;

	// Grenade frags can't be flashed, because that's stupid
	// Also not knife frags, but it doesn't matter in those
	if( bullet_kill )
	{
		float timeSinceFlashed = GetTimeBetweenTicks( m_iCurrentTick, pAttacker->flashinfo.tick );
		constexpr float minRemainingFlashedTime = 2.3f;
		killInfo.blind = (pAttacker->flashinfo.time - timeSinceFlashed) >= minRemainingFlashedTime;
	}

	// ===== Mid-air check ====================
	// Check this here AND in player post-check, to make sure the player is properly in the air
	killInfo.midair_status = ON_GROUND;

	// Don't check for spectated mid-air kills in POV demos, because for some reason send props are not properly updated
	if( bullet_kill && !bSpectatingAttacker )
	{
		prop = pEntAttacker->FindProp( "m_fFlags" );

		// This CAN be NULL, if the flags never had to be updated before a kill was made
		if( prop )
		{
			uint32 flags = prop->m_pPropValue->m_value.m_int;

			if( !(flags & FL_ONGROUND) )
			{
				prop = pEntAttacker->FindProp( "movetype" );

				if( !prop )
				{
					killInfo.midair_status = (pAttacker->airstatus == PL_WENT_UP_IN_AIR) ? JUMPSHOT : ON_GROUND;
				}
				else if( prop->m_pPropValue->m_value.m_int == MOVETYPE_LADDER )
				{
					killInfo.midair_status = LADDERSHOT;
				}
				else
				{
					killInfo.midair_status = (pAttacker->airstatus == PL_WENT_UP_IN_AIR) ? JUMPSHOT : ON_GROUND;
				}
			}
		}
	}

	// ===== Noscope check ====================
	if( !killInfo.noscope && WeaponIsSniper( weaponName ) )
	{
		prop = pEntAttacker->FindProp( "m_iFOV" );

		// This can be NULL if the player noscopes someone before ever zooming in
		if( prop )
		{
			killInfo.noscope = prop->m_pPropValue->m_value.m_int == NOSCOPE_FOV;
		}
		// Assume it's a noscope if the prop wasn't updated yet
		// This could lead to false results if the player started recording while zoomed in
		else 
		{
			killInfo.noscope = true;
		}
	}
		
	// ===== Flickshot check ====================
	// Actually, this has to be checked after packet entities have been processed for this tick
	killInfo.flickshot = false;

	// ===== Distance check =====================
	killInfo.distance = 0;

	prop = pEntAttacker->FindProp( "m_vecOrigin" );

	if( !prop )
		throw ParsingError_t( "Attacker origin prop not found in HandlePlayerDeathEvent" );

	Vector vecAttacker = prop->m_pPropValue->m_value.m_vector;

	if( bullet_kill && pEntVictim )
	{
		prop = pEntVictim->FindProp( "m_vecOrigin" );

		if( !prop )
			throw ParsingError_t( "Victim origin prop not found in HandlePlayerDeathEvent" );

		Vector vecVictim = prop->m_pPropValue->m_value.m_vector;

		Vector to( vecVictim - vecAttacker );

		killInfo.distance = to.Length();
	}

	killInfo.tick = m_iCurrentTick;
	killInfo.victim_id = pVictim->userID;
	killInfo.weaponID = AliasToWeaponID( weaponName );
	killInfo.spectated = bSpectatingAttacker;
	killInfo.position = vecAttacker;

	pAttacker->AddKill( killInfo );
}

// =====================================================================================================================================================================

void DemoParser::HandlePlayerDisconnectEvent( bf_read &reader, const GameEvent &event )
{
	size_t fields = event.GetFieldCount();

	for( size_t i = 0; i < fields; ++i )
	{
		const GameEvent::GameEventField &field = event.GetField( i );

		if( strcmp( field.field_name, "userid" ) == 0 )
		{
			Player *player = FindPlayerByUserId( reader.ReadShort() );

			if( player ) // Might not be found if the disconnect happens before string tables are updated
			{
				m_ExpiredUserIDs.push_back( player->userID );
				player->entityIndex = -1;
			}
		}
		else
		{
			ReadGameEventFieldValue( reader, field );
		}
	}
}

// =====================================================================================================================================================================

void DemoParser::OnRoundStartEvent( void )
{
	// Find the frags from the previous round
	FindRoundFrags();

	// Delete any expired players after we checked if they made any frags before leaving
	// Don't use size_t here as it is unsigned, and "i" has to go below 0
	for( int i = (int)m_ExpiredUserIDs.size() - 1; i >= 0; --i )
	{
		bool found = false;
		for( auto pl = m_Players.begin(); pl != m_Players.end(); ++pl )
		{
			if( pl->userID == m_ExpiredUserIDs[ i ] )
			{
				m_Players.erase( pl );
				found = true;
				break;
			}
		}

		assert( found );
	}

	m_ExpiredUserIDs.clear();
	m_PlayerPostCheckData.clear();

	m_iNewRoundTick = m_iCurrentTick;

	// Reset player kills for the next round
	for( auto it = m_Players.begin(); it != m_Players.end(); ++it )
	{
		it->ResetKills();

		// Set player statuses to AFK on STV demos, so we know to look for origin/viewangle updates
		// Don't do this on POV demos, because the player entities might not be updated before being killed
		if( !m_bIsPOV )
		{
			it->activity = PL_AFK;
		}
	}
}

// =====================================================================================================================================================================