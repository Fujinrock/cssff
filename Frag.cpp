#include "Frag.h"
#include "DemoParser.h"
#include "Settings.h"
#include "Weapons.h"
#include <string>

// =====================================================================================================================================================================

bool TryToAddMultiKillFragDescriptor( Frag &frag, MultiKillFragType frag_type, const std::vector< const kill_info_t * > &kills, int cur_kill_idx, int min_frag_kills )
{
	// Check if there is already a better frag added
	MultiKillFragType existing_type = frag.GetMultiKillFragType();

	if( existing_type != FRAG_NONE )
	{
		// Can't overwrite 5k with lower type
		if( existing_type == FRAG_5K && frag_type != FRAG_5K )
			return false;

		// 3k can't overwrite 4k or 5k
		if( existing_type != FRAG_3K && frag_type == FRAG_3K )
			return false;
	}

	const int start_tick = kills[ cur_kill_idx - min_frag_kills ]->tick;
	const int end_tick = kills[ cur_kill_idx - 1 ]->tick;

	const float frag_time = GetTimeBetweenTicks( start_tick, end_tick );

	// Get a list of weapons and the number of headshots
	CSWeaponID weapons[ multi_kill_frag_descriptor_t::FRAG_MAX_WEAPONS ];
	short num_weapons = 0;
	short num_headshots = 0;
	bool contains_special_kill = false;

	float farthest_kill_distance = -1.f;
	Vector vecFragOrigin = kills[ cur_kill_idx - min_frag_kills ]->position;

	for( int i = cur_kill_idx - min_frag_kills; i < cur_kill_idx; ++i )
	{
		const kill_info_t *kill = kills[ i ];

		if( num_weapons < multi_kill_frag_descriptor_t::FRAG_MAX_WEAPONS )
			weapons[ num_weapons++ ] = kill->weaponID;

		if( kill->headshot )
			++num_headshots;

		if( kill->flickshot || kill->midair != ON_GROUND || kill->noscope || kill->penetrated || kill->blind )
			contains_special_kill = true;

		const float distance_to_start = (kill->position - vecFragOrigin).Length();

		if( distance_to_start > farthest_kill_distance )
			farthest_kill_distance = distance_to_start;
	}

	if( !Settings()->ShouldTickFrag( frag_type, GetWeaponCategory( weapons, num_weapons ), frag_time, farthest_kill_distance, num_headshots, contains_special_kill ) )
		return false;

	return frag.AddMultiKillFragDescriptor( frag_type, weapons, num_weapons, start_tick, end_tick, num_headshots );
}

// =====================================================================================================================================================================

float GetDeltaTimeToClosestKill( const Player *pPlayer, int killIdx )
{
	int tick = pPlayer->GetKill( killIdx ).tick;
	CSWeaponID weapon = pPlayer->GetKill( killIdx ).weaponID;
	int closestTickBefore = 999999999;
	int closestTickAfter = 999999999;
	int tickDeltaToBefore = 999999999;
	int tickDeltaToAfter = 999999999;

	// Check earlier kill
	int idxCopy = killIdx;
	while( --idxCopy >= 0 )
	{
		const kill_info_t &kill = pPlayer->GetKill( idxCopy );

		if( kill.teamkill )
			continue;

		if( kill.tick == tick && kill.weaponID == weapon )
			continue;

		closestTickBefore = kill.tick;
		tickDeltaToBefore = abs( tick - closestTickBefore );
		break;
	}

	// Check kill after
	idxCopy = killIdx;
	while( ++idxCopy < pPlayer->GetNumKills() )
	{
		const kill_info_t &kill = pPlayer->GetKill( idxCopy );

		if( kill.teamkill )
			continue;

		if( kill.tick == tick && kill.weaponID == weapon )
			continue;

		closestTickAfter = kill.tick;
		tickDeltaToAfter = abs( tick - closestTickAfter );
		break;
	}

	// Return shorter time
	if( tickDeltaToBefore < tickDeltaToAfter )
	{
		return GetTimeBetweenTicks( tick, closestTickBefore );
	}
	else
	{
		return GetTimeBetweenTicks( tick, closestTickAfter );
	}
}

// =====================================================================================================================================================================

void DemoParser::FindRoundFrags( void )
{
	for( auto p = m_Players.begin(); p != m_Players.end(); ++p )
	{
		if( p->ishltv )
			continue;

		const int num_all_kills = p->GetNumKills();

		if( num_all_kills <= 0 )
			continue;

		// Make a list of all killed enemies
		std::vector< const kill_info_t * > enemy_kills;
		bool spectated = false;
		for( int k = 0; k < num_all_kills; ++k )
		{
			const kill_info_t &kill = p->GetKill( k );

			spectated = kill.spectated;

			if( !kill.teamkill )
			{
				enemy_kills.push_back( &kill );
			}
		}

		const int num_enemy_kills = enemy_kills.size();

		Frag player_frag( num_enemy_kills, p->GetKill( 0 ).team, spectated );

		// Check for 5/4/3k frags before any 1k/collat frags
		const int MIN_FRAG_KILLS = 3;
		for( int k = num_enemy_kills; k >= MIN_FRAG_KILLS; --k )
		{
			bool bDescAdded = false;

			if( k >= 5 )
			{
				bDescAdded = TryToAddMultiKillFragDescriptor( player_frag, FRAG_5K, enemy_kills, k, 5 );
			}
			if( k >= 4 && !bDescAdded )
			{
				bDescAdded = TryToAddMultiKillFragDescriptor( player_frag, FRAG_4K, enemy_kills, k, 4 );
			}
			if( k >= 3 && !bDescAdded )
			{
				bDescAdded = TryToAddMultiKillFragDescriptor( player_frag, FRAG_3K, enemy_kills, k, 3 );
			}
		}

		// Then check for collats and 1k frags
		for( int k = num_all_kills - 1; k >= 0; --k )
		{
			const kill_info_t *kill = &p->GetKill( k );
			unsigned short type_flags = 0;

			short kills_on_tick = 1;
			short teamkills = kill->teamkill? 1 : 0;
			short headshots = kill->headshot? 1 : 0;
			float time_to_closest_kill = (kill->penetrated? GetDeltaTimeToClosestKill( &(*p), k ) : 0.f);
			float longest_distance = kill->distance;
			bool blind = kill->blind;

			while( k >= 1 && kill->tick == p->GetKill( k - 1 ).tick && kill->weaponID == p->GetKill( k - 1 ).weaponID )
			{
				--k;
				++kills_on_tick;

				// Check flags from first kill in collat,
				// because in doubles, all other kills will always have penetrated flag on
				kill = &p->GetKill( k );

				if( kill->teamkill )
					++teamkills;

				if( kill->headshot )
					++headshots;

				// Keep track of longest distance in case this won't be ticked as a collat
				if( kill->distance > longest_distance )
					longest_distance = kill->distance;

				blind = kill->blind;
			}

			if( kills_on_tick > 1 )
			{
				if( kills_on_tick == 2 )
				{
					type_flags |= FL_KILL_DOUBLE;
				}
				else if( kills_on_tick == 3 )
				{
					type_flags |= FL_KILL_TRIPLE;
				}
				else if( kills_on_tick == 4 )
				{
					type_flags |= FL_KILL_QUADRO;
				}
				else
				{
					type_flags |= FL_KILL_PENTA;
				}
			}

			if( kill->flickshot )
				type_flags |= FL_KILL_FLICKSHOT;
			if( kill->midair == IN_AIR )
				type_flags |= FL_KILL_MIDAIR;
			else if( kill->midair == ON_LADDER )
				type_flags |= FL_KILL_LADDERSHOT;
			if( kill->noscope )
				type_flags |= FL_KILL_NOSCOPE;
			if( kill->penetrated )
				type_flags |= FL_KILL_WALLBANG;

			if( kill->weaponID == WEAPON_FLASHBANG )
				type_flags |= FL_KILL_FLASHKILL;
			else if( kill->weaponID == WEAPON_SMOKEGRENADE )
				type_flags |= FL_KILL_SMOKEKILL;

			if( type_flags != 0 )
			{
				if( blind )	// Don't tick if only this flag was set
					type_flags |= FL_KILL_BLIND;

				player_frag.AddFragDescriptor( kill->tick, type_flags, teamkills, headshots, kill->weaponID, longest_distance, kill->flickangle, time_to_closest_kill );
			}
		}

		if( player_frag.IsValidFrag() ) // Were there any actions to save?
		{
			m_Frags.push_back( player_frag );
			(m_Frags.end() - 1)->SetPlayername( p->name );
		}
	}
}

// =====================================================================================================================================================================

void frag_descriptor_t::GetStringRepresentation( char *buffer, size_t buffer_size, bool write_hs /*= true*/, bool write_weapon /*= true*/ ) const
{
	_snprintf_s( buffer, buffer_size, buffer_size, "" );

	if( type_flags == 0 )
	{
		return;
	}

	// Fields other than count shouldn't matter for flash/smoke kills
	if( type_flags & FL_KILL_FLASHKILL )
	{
		if( count > 1 )
			_snprintf_s( buffer, buffer_size, buffer_size, "%d flashkills", count );
		else
			_snprintf_s( buffer, buffer_size, buffer_size, "flashkill" );

		return;
	}
	else if( type_flags & FL_KILL_SMOKEKILL )
	{
		if( count > 1 )
			_snprintf_s( buffer, buffer_size, buffer_size, "%d smokekills", count );
		else
			_snprintf_s( buffer, buffer_size, buffer_size, "smokekill" );

		return;
	}

	if( count > 1 )
	{
		_snprintf_s( buffer, buffer_size, buffer_size, "%d ", count );
	}

	if( type_flags & FL_KILL_BLIND )
	{
		strcat_s( buffer, buffer_size, "flashed " );
	}

	if( write_weapon && weapon != WEAPON_HEGRENADE )
	{
		_snprintf_s( buffer, buffer_size, buffer_size, "%s%s ", buffer, WeaponIDToAlias( weapon ) );
	}

	if( !FragIsCollat( type_flags ) && teamkills == count )
	{
		strcat_s( buffer, buffer_size, "teamkill " );
	}

	if( type_flags & FL_KILL_NOSCOPE )
	{
		strcat_s( buffer, buffer_size, "noscope " );
	}
	if( type_flags & FL_KILL_FLICKSHOT )
	{
		// Write angle
		if( count == 1 && flick_angle != 0.f )
		{
			int angle = GetRoundedFlickAngle();
			char szAngle[8];
			_snprintf_s( szAngle, sizeof(szAngle), sizeof(szAngle), "%d‹ ", angle );
			strcat_s( buffer, buffer_size, szAngle );
		}

		// Don't write "shot" if we will write "headshot" afterwards
		if( write_hs && !FragIsCollat( type_flags ) && headshots == count )
		{
			strcat_s( buffer, buffer_size, "flick " );
		}
		else
		{
			strcat_s( buffer, buffer_size, "flickshot " );
		}
	}
	if( type_flags & FL_KILL_MIDAIR )
	{
		// Add in "kill" if nothing else comes after this
		if( !(type_flags & FL_KILL_WALLBANG)
			&& !FragIsCollat( type_flags )
			&& (!write_hs || headshots != count) )
		{
			strcat_s( buffer, buffer_size, "mid-air kill " );
		}
		else
		{
			strcat_s( buffer, buffer_size, "mid-air " );
		}
	}
	else if( type_flags & FL_KILL_LADDERSHOT )
	{
		strcat_s( buffer, buffer_size, "laddershot " );
	}
	if( type_flags & FL_KILL_WALLBANG )
	{
		strcat_s( buffer, buffer_size, "wallbang " );
	}

	// Because at least one flag had to be set for non-collat frags,
	// this might be the end of the description if "headshots" is not written later,
	// so add the plural suffix on the last flag descriptor
	if( count > 1 && !FragIsCollat( type_flags ) && (!write_hs || headshots != count) )
	{
		size_t len = strlen( buffer );
		buffer[ len-1 ] = 's';
		buffer[ len ] = ' ';
		buffer[ len+1 ] = '\0';
	}

	if( FragIsCollat( type_flags ) )
	{
		// Don't call HE kills doubles/triples etc.
		if( weapon == WEAPON_HEGRENADE )
		{
			if( type_flags & FL_KILL_DOUBLE )
			{
				strcat_s( buffer, buffer_size, "2k HE" );
			}
			else if( type_flags & FL_KILL_TRIPLE )
			{
				strcat_s( buffer, buffer_size, "3k HE" );
			}
			else if( type_flags & FL_KILL_QUADRO )
			{
				strcat_s( buffer, buffer_size, "4k HE" );
			}
			else if( type_flags & FL_KILL_PENTA )
			{
				strcat_s( buffer, buffer_size, "5k HE" );
			}

			if( count > 1 )
				strcat_s( buffer, buffer_size, "'s " );
			else
				strcat_s( buffer, buffer_size, " " );
		}
		// Collat done with bullets
		else
		{
			if( type_flags & FL_KILL_DOUBLE )
			{
				strcat_s( buffer, buffer_size, "double" );
			}
			else if( type_flags & FL_KILL_TRIPLE )
			{
				strcat_s( buffer, buffer_size, "triple" );
			}
			else if( type_flags & FL_KILL_QUADRO )
			{
				strcat_s( buffer, buffer_size, "quadro" );
			}
			else if( type_flags & FL_KILL_PENTA )
			{
				strcat_s( buffer, buffer_size, "penta" );
			}

			if( count > 1 )
				strcat_s( buffer, buffer_size, "s " );
			else
				strcat_s( buffer, buffer_size, " " );
		}
	}

	if( write_hs )
	{
		if( !FragIsCollat( type_flags ) && headshots == count )
		{
			if( count > 1 )
				strcat_s( buffer, buffer_size, "headshots " );
			else
				strcat_s( buffer, buffer_size, "headshot " );
		}
		else if( headshots > 0 )
		{
			char szHs[12];
			_snprintf_s( szHs, sizeof(szHs), sizeof(szHs), "(%dhs) ", headshots );
			strcat_s( buffer, buffer_size, szHs );
		}
	}

	if( teamkills > 0 && (FragIsCollat( type_flags ) || teamkills != count) )
	{
		char szTk[12];
		_snprintf_s( szTk, sizeof(szTk), sizeof(szTk), "(%dtk) ", teamkills );
		strcat_s( buffer, buffer_size, szTk );
	}

	// Remove last character, because the description always ends in a whitespace
	size_t len = strlen( buffer );
	buffer[ len-1 ] = '\0';
}

// =====================================================================================================================================================================

int frag_descriptor_t::GetRoundedFlickAngle( void ) const
{
	// Try to round towards every 180 degrees first
	const float leeway = 20.f;
	float angle = 180.f;
	float absFlickAngle = fabsf( flick_angle );
	while( flick_angle >= angle - leeway )
	{
		if( fabsf(flick_angle - angle) <= leeway )
		{
			return (int)(angle + 0.1f);
		}

		angle += 180.f;
	}

	// Just round to nearest 10
	return (((int)(absFlickAngle + 5.f)) / 10) * 10;
}

// =====================================================================================================================================================================

void multi_kill_frag_descriptor_t::GetStringRepresentation( char *buffer, size_t buffer_size ) const
{
	_snprintf_s( buffer, buffer_size, buffer_size, "%dk ", static_cast<int>(frag_type) );

	byte hs_print_mode = PRINT_HS_IN_MAIN;

	// If every kill in this frag is a headshot, don't print hs info in sub descriptors
	// This ignores teamkills though, since they're not included in the kill count
	if( headshots > 0 && headshots != static_cast<int>(frag_type) )
	{
		// If hs count in the sub descriptors equals the hs count of this descriptor, only print it in the sub descs
		int sub_headshots = 0;
		for( size_t i = 0; i < sub_descriptors.size(); ++i )
		{
			const frag_descriptor_t &sub_desc = sub_descriptors[ i ];

			// Skip teamkills, or just print in both if there's a collat with teamkills, since we don't know the hs count on enemy kills
			if( sub_desc.teamkills > 0 && sub_desc.headshots > 0 )
			{
				if( sub_desc.teamkills == 1 && sub_desc.headshots == 1 )
					continue;

				sub_headshots = -1;
				break;
			}

			sub_headshots += sub_descriptors[ i ].headshots;
		}

		if( sub_headshots == headshots )
		{
			hs_print_mode = PRINT_HS_IN_SUB;
		}
		else
		{
			hs_print_mode = PRINT_HS_IN_BOTH;
		}
	}

	// Print hs count
	if( headshots > 0 && hs_print_mode != PRINT_HS_IN_SUB )
	{
		char szHs[12];
		_snprintf_s( szHs, sizeof(szHs), sizeof(szHs), "(%dhs) ", headshots );
		strcat_s( buffer, buffer_size, szHs );
	}

	// Add used weapons
	for( int i = 0; i < num_weapons; ++i )
	{
		if( i )
			strcat_s( buffer, buffer_size, "/" );

		strcat_s( buffer, buffer_size, WeaponIDToAlias( weapons[ i ] ) );
	}

	strcat_s( buffer, buffer_size, " " );

	const int num_sub_descs = sub_descriptors.size();

	// Add sub descriptors
	if( num_sub_descs )
	{
		strcat_s( buffer, buffer_size, "with " );
		char sub_buffer[256];
		const int first_sub_desc = num_sub_descs - 1;

		// Add them in reverse order, since they're added to the vector in reverse order
		for( int i = first_sub_desc; i >= 0; --i )
		{
			if( i < first_sub_desc )
			{
				if( i == 0 )
					strcat_s( buffer, buffer_size, " & " );
				else 
					strcat_s( buffer, buffer_size, ", " );
			}

			sub_descriptors[ i ].GetStringRepresentation( sub_buffer, sizeof(sub_buffer), hs_print_mode != PRINT_HS_IN_MAIN, false );
			strcat_s( buffer, buffer_size, sub_buffer );
		}

		strcat_s( buffer, buffer_size, " " );
	}

	char szTime[32];
	_snprintf_s( szTime, sizeof(szTime), sizeof(szTime), "in %.02f seconds ", frag_length );
	strcat_s( buffer, buffer_size, szTime );

	// Remove last character, because the description always ends in a whitespace
	size_t len = strlen( buffer );
	buffer[ len-1 ] = '\0';
}

// =====================================================================================================================================================================

void multi_kill_frag_descriptor_t::AddWeaponIDs( const CSWeaponID *pWeapons, int num )
{
	for( int i = 0; i < num; ++i )
	{
		if( num_weapons >= FRAG_MAX_WEAPONS )
			return;

		CSWeaponID weaponID = pWeapons[ i ];

		if( weaponID < WEAPON_NONE || weaponID >= WEAPON_MAX )
			continue;

		// Don't add flash or smoke, because these won't be included in the weapon list in the string
		if( weaponID == WEAPON_FLASHBANG || weaponID == WEAPON_SMOKEGRENADE )
			continue;

		bool bAlreadyIn = false;
		for( int j = 0; j < num_weapons; ++j )
		{
			if( weapons[ j ] == weaponID )
			{
				bAlreadyIn = true;
				break;
			}
		}

		if( bAlreadyIn )
			continue;

		weapons[ num_weapons++ ] = weaponID;
	}
}

// =====================================================================================================================================================================

void multi_kill_frag_descriptor_t::ClearWeapons( void )
{
	num_weapons = 0;
}

// =====================================================================================================================================================================

bool multi_kill_frag_descriptor_t::IsValid( void ) const
{
	return frag_type != FRAG_NONE;
}

// =====================================================================================================================================================================

void multi_kill_frag_descriptor_t::Reset( void )
{
	frag_type = FRAG_NONE;
	headshots = -1;
	start_tick = -1;
	end_tick = -1;
	frag_length = -1;

	sub_descriptors.clear();

	ClearWeapons();
}

// =====================================================================================================================================================================

Frag::Frag( short total_kills, byte team, bool spectated )
	: m_nStartTick(INVALID_TICK), m_nTotalKills(total_kills), m_nTeam(team), m_bSpectated(spectated)
{
	m_szPlayername[0] = '\000';
}

// =====================================================================================================================================================================

void Frag::SetPlayername( const char *playername )
{
	strcpy_s( m_szPlayername, sizeof(m_szPlayername), playername );
}

// =====================================================================================================================================================================

MultiKillFragType Frag::GetMultiKillFragType( void ) const
{
	return m_multiKillDescriptor.frag_type;
}

// =====================================================================================================================================================================

bool Frag::AddMultiKillFragDescriptor( MultiKillFragType type, const CSWeaponID *weapons, short num_weapons, uint32 start_tick, uint32 end_tick, short headshots )
{
	assert( m_descriptors.size() == 0 ); // No other descriptors should have been added yet

	float frag_length = GetTimeBetweenTicks( start_tick, end_tick );

	if( frag_length <= 0 ) // If all kills were on 1 tick, it should be ticked as a collat
		return false;

	// Check if we can change an already existing descriptor
	if( m_multiKillDescriptor.IsValid() )
	{
		MultiKillFragType existingType = m_multiKillDescriptor.frag_type;

		// Don't allow changing 5k to a 4k or 3k, or a 4k to a 3k
		if( existingType == FRAG_5K && (type == FRAG_4K || type == FRAG_3K) )
			return false;

		if( existingType == FRAG_4K && type == FRAG_3K )
			return false;

		// Allow changing a lower type frag to a higher type
		if( (type == FRAG_5K && (existingType == FRAG_4K || existingType == FRAG_3K))
			|| (type == FRAG_4K && existingType == FRAG_3K) )
		{
			m_multiKillDescriptor.Reset();
		}
		else // Existing type is the same as the old type
		{
			// Is the new frag faster? if so, tick it
			if( m_multiKillDescriptor.frag_length > frag_length )
			{
				m_multiKillDescriptor.Reset();
			}
			else
			{
				return false;
			}
		}
	}

	assert( !m_multiKillDescriptor.IsValid() );

	m_multiKillDescriptor.frag_type = type;
	m_multiKillDescriptor.start_tick = start_tick;
	m_multiKillDescriptor.end_tick = end_tick;
	m_multiKillDescriptor.frag_length = frag_length;
	m_multiKillDescriptor.headshots = headshots;
	m_multiKillDescriptor.AddWeaponIDs( weapons, num_weapons );

	if( m_nStartTick == INVALID_TICK || start_tick < m_nStartTick )
		m_nStartTick = start_tick;

	return true;
}

// =====================================================================================================================================================================

void Frag::AddFragDescriptor( uint32 tick, unsigned short type_flags, short teamkills, short headshots, CSWeaponID weapon, float distance, float flickangle, float time_to_closest_kill )
{
	assert( (type_flags & ~FL_KILL_BLIND) != 0 );

	// Update frag start tick
	if( m_nStartTick == INVALID_TICK || tick < m_nStartTick )
		m_nStartTick = tick;

	// Check if this descriptor is part of a bigger descriptor
	bool in_multikill_frag = false;
	if( m_multiKillDescriptor.IsValid() )
	{
		if( tick >= m_multiKillDescriptor.start_tick && tick <= m_multiKillDescriptor.end_tick )
		{
			in_multikill_frag = true;
		}
	}

	// Descriptors that are inside 5/4/3ks are always added, since they're included inside a frag anyways,
	// but separate descriptors are not necessarily added depending on the settings
	if( !in_multikill_frag )
	{
		if( !Settings()->ShouldTickFrag( type_flags, GetWeaponCategory( weapon ), distance, headshots, time_to_closest_kill ) )
			return;
	}

	// Figure out with which flag to search for other descriptors of this type
	int searchFlags = 0;

	do
	{
		// Go in order of importance, so start with collats
		searchFlags = type_flags & MASK_COLLATS;
		if( searchFlags )
			break;

		// No collats, check for projectile kills
		searchFlags = type_flags & MASK_PROJECTILE_KILLS;
		if( searchFlags )
			break;

		// Check for mid-air
		searchFlags = type_flags & FL_KILL_MIDAIR;
		if( searchFlags )
			break;

		// Check for laddershots
		searchFlags = type_flags & FL_KILL_LADDERSHOT;
		if( searchFlags )
			break;

		// Check for flickshot
		searchFlags = type_flags & FL_KILL_FLICKSHOT;
		if( searchFlags )
			break;

		// Check for wallbang
		searchFlags = type_flags & FL_KILL_WALLBANG;
		if( searchFlags )
			break;

		// Finally, check for noscope
		searchFlags = type_flags & FL_KILL_NOSCOPE;
	}
	while( 0 );

	std::vector< frag_descriptor_t > *pDescs = in_multikill_frag? &(m_multiKillDescriptor.sub_descriptors) : &m_descriptors;

	// Check if this kind of a frag has already been added
	for( auto it = pDescs->begin(); it != pDescs->end(); ++it )
	{
		if( (it->type_flags & searchFlags) != 0 )
		{
			// Don't allow removing collat flags with 1k frags
			// but let 1k frags remove each others' flags if need be
			if( FragIsCollat( it->type_flags ) && !FragIsCollat( type_flags ) )
				continue;

			// Only increase count if the previous instance was done with the same weapon OR this is in a multikill frag,
			// because weapons don't matter in those. Don't combine HE doubles with doubles done with other weapons, though
			if( (in_multikill_frag && (it->weapon == WEAPON_HEGRENADE && weapon == WEAPON_HEGRENADE || it->weapon != WEAPON_HEGRENADE && weapon != WEAPON_HEGRENADE))
				|| it->weapon == weapon )
			{
				++it->count;

				// Reset headshots on collats because it's hard to make a note of those with multiple instances of same frag
				if( FragIsCollat( type_flags ) )
					it->headshots = 0;
				else
					it->headshots += headshots;

				// Keep track of teamkills, though
				it->teamkills += teamkills;

				// Reset flick angle, too
				it->flick_angle = 0.0;

				// If the flags aren't exactly the same, remove all the extra flags to keep the description accurate for all instances of this frag
				if( it->type_flags != type_flags )
				{
					it->type_flags &= searchFlags;
				}

				// Count increased, we are done here
				return;
			}
		}
	}

	// This is the first instance of this kind of frag, add it
	frag_descriptor_t descriptor;
	descriptor.count = 1;
	descriptor.headshots = headshots;
	descriptor.teamkills = teamkills;
	descriptor.type_flags = type_flags;
	descriptor.weapon = weapon;
	descriptor.flick_angle = flickangle;
	pDescs->push_back( descriptor );
}

// =====================================================================================================================================================================

bool Frag::IsValidFrag( void ) const
{
	return m_multiKillDescriptor.IsValid() || m_descriptors.size() > 0;
}

// =====================================================================================================================================================================

void Frag::GetStringRepresentation( char *buffer, size_t buffer_size ) const
{
	if( !IsValidFrag() )
	{
		_snprintf_s( buffer, buffer_size, buffer_size, "" );
		return;
	}

	_snprintf_s( buffer, buffer_size, buffer_size, "Tick: %d    Player: %s (%s)%s\nFrag: ", GetRoundedTick(), m_szPlayername, GetTeamString(), m_bSpectated? " (*SPEC*)" : "" );

	int implied_kills = GetImpliedKillCount();

	if( m_nTotalKills > implied_kills )
	{
		char prefixbuffer[20];
		_snprintf_s( prefixbuffer, sizeof(prefixbuffer), sizeof(prefixbuffer), "%dk %s ", m_nTotalKills, m_multiKillDescriptor.IsValid()? "including" : "with" );
		strcat_s( buffer, buffer_size, prefixbuffer );
	}

	if( m_multiKillDescriptor.IsValid() )
	{
		char mkbuffer[512];
		m_multiKillDescriptor.GetStringRepresentation( mkbuffer, sizeof(mkbuffer) );
		strcat_s( buffer, buffer_size, mkbuffer );

		if( m_descriptors.size() )
		{
			strcat_s( buffer, buffer_size, " and " );
		}
	}

	// Concatenate descriptors in reverse order, since they're added to the vector in reverse order
	const int first_descriptor = m_descriptors.size() - 1;
	for( int i = first_descriptor; i >= 0; --i )
	{
		char descbuffer[256] = "\0";
		m_descriptors[ i ].GetStringRepresentation( descbuffer, sizeof(descbuffer) );

		// Capitalize first letter if this is the first descriptor
		if( i == first_descriptor
			&& m_nTotalKills == implied_kills
			&& !m_multiKillDescriptor.IsValid()
			&& m_descriptors[ i ].count == 1
			&& descbuffer[0] != '\0' )
		{
			descbuffer[0] = toupper( descbuffer[0] );
		}

		if( i < first_descriptor )
		{
			if( i == 0 )
			{
				strcat_s( buffer, buffer_size, " & " );
			}
			else
			{
				strcat_s( buffer, buffer_size, ", " );
			}
		}

		strcat_s( buffer, buffer_size, descbuffer );
	}

	strcat_s( buffer, buffer_size, "\n\n\n" );
}

// =====================================================================================================================================================================

int Frag::GetImpliedKillCount( void ) const
{
	int implied_kills = 0;

	if( m_multiKillDescriptor.IsValid() )
		implied_kills += static_cast<int>(m_multiKillDescriptor.frag_type);

	for( size_t i = 0; i < m_descriptors.size(); ++i )
	{
		int kills = 0;
		uint32 flags = m_descriptors[ i ].type_flags;

		if( FragIsCollat( flags ) )
		{
			if( flags & FL_KILL_DOUBLE )
				kills = 2;
			else if( flags & FL_KILL_TRIPLE )
				kills = 3;
			else if( flags & FL_KILL_QUADRO )
				kills = 4;
			else if( flags & FL_KILL_PENTA )
				kills = 5;
		}
		else
		{
			kills = 1;
		}

		// Don't count team kills
		kills -= m_descriptors[ i ].teamkills;

		implied_kills += kills;
	}

	return implied_kills;
}

// =====================================================================================================================================================================

enum { TEAM_T = 2, TEAM_CT = 3 };

const char *Frag::GetTeamString( void ) const
{
	if( m_nTeam == TEAM_T )
		return "T";

	if( m_nTeam == TEAM_CT )
		return "CT";

	return "??";
}

// =====================================================================================================================================================================

int Frag::GetRoundedTick( void ) const
{
	const int seg_size = ((GetTickRate() * 5 + 25) / 50) * 50; // Round to nearest 50
	int num_of_seg = m_nStartTick / seg_size;
	int remainder = m_nStartTick % seg_size;

	int rounded_tick = num_of_seg * seg_size;

	const int min_preceding_ticks = GetTickRate() * 3;
	if( remainder < min_preceding_ticks )
		rounded_tick -= seg_size;

	// Round up to the next 100 on tickrates above 100
	if( GetTickRate() > 100 && ((rounded_tick % 100) != 0) )
		rounded_tick += (100 - (rounded_tick % 100));

	// Some extra rounding on higher tickrates
	if( GetTickRate() >= 100 )
	{
		// Round up to the next 1000 if we're at 900
		if( ( rounded_tick % 1000 ) >= 900 )
		{
			rounded_tick += ( 1000 - ( rounded_tick % 1000 ) );
		}
		// Round down to previous 1000 if we're at 100
		else if( ( rounded_tick % 1000 ) <= 100 )
		{
			rounded_tick -= ( rounded_tick % 1000 );
		}
	}

	if( rounded_tick < 0 )
		rounded_tick = 0;

	return rounded_tick;
}

// =====================================================================================================================================================================