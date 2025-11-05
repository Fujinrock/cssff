#include "StringTables.h"
#include "DemoParser.h"
#include "Player.h"
#include "Errors.h"
#include <string>

#define SUBSTRING_BITS		5

// =====================================================================================================================================================================

enum
{
	MAX_USERDATA_BITS = 12,
	MAX_USERDATA_SIZE = (1 << MAX_USERDATA_BITS)
};

// =====================================================================================================================================================================

void DemoParser::CreateStringTable( const char *name, int max_entries, int user_data_size, int user_data_size_bits, bool user_data_fixed_size )
{
	if( m_iNumStringTables >= MAX_STRING_TABLES )
		throw ParsingError_t( "too many string tables" );

	strcpy_s( m_StringTables[ m_iNumStringTables ].szName, name );
	m_StringTables[ m_iNumStringTables ].nMaxEntries = max_entries;
	m_StringTables[ m_iNumStringTables ].nUserDataSize = user_data_size;
	m_StringTables[ m_iNumStringTables ].nUserDataSizeBits = user_data_size_bits;
	m_StringTables[ m_iNumStringTables ].bUserDataFixedSize = user_data_fixed_size;
	++m_iNumStringTables;
}

// =====================================================================================================================================================================

void DemoParser::ParseStringTableUpdate( bf_read &reader, int entries, int max_entries, int user_data_size, int user_data_size_bits, bool user_data_fixed_size, bool is_user_info )
{
	struct StringHistoryEntry
	{
		char string[ ( 1 << SUBSTRING_BITS ) ];
	};

	int lastEntry = -1;
	int lastDictionaryIndex = -1;

	// Perform integer log2() to set nEntryBits
	int nTemp = max_entries;
	int nEntryBits = 0;
	while (nTemp >>= 1) ++nEntryBits;

	if( ( 1 << nEntryBits ) != max_entries )
		throw ParsingError_t( "string table size not a power of two" );

	std::vector< StringHistoryEntry > history;

	for( int i = 0; i < entries; i++ )
	{
		int entryIndex = lastEntry + 1;

		if( !reader.ReadOneBit() )
		{
			entryIndex = reader.ReadUBitLong( nEntryBits );
		}

		lastEntry = entryIndex;

		if( entryIndex < 0 || entryIndex >= max_entries )
		{
			throw ParsingError_t( "bogus string index in ParseStringTableUpdate" );
		}

		const char *pEntry = nullptr;
		char entry[ 1024 ]; 
		char substr[ 1024 ];
		entry[ 0 ] = 0;

		if( reader.ReadOneBit() )
		{
			bool substringcheck = reader.ReadOneBit() ? true : false;

			if( substringcheck )
			{
				int index = reader.ReadUBitLong( 5 );
				if( size_t( index ) >= history.size() )
				{
					throw ParsingError_t( "invalid string history index in ParseStringTableUpdate" );
				}
				int bytestocopy = reader.ReadUBitLong( SUBSTRING_BITS );
				strncpy_s( entry, history[ index ].string, bytestocopy + 1 );
				reader.ReadString( substr, sizeof( substr ) );
				strcat_s( entry, substr );
			}
			else
			{
				reader.ReadString( entry, sizeof( entry ) );
			}

			pEntry = entry;
		}

		unsigned char tempbuf[ MAX_USERDATA_SIZE ];
		memset( tempbuf, 0, sizeof( tempbuf ) );
		const void *pUserData = nullptr;
		int nBytes = 0;

		// Read in the user data
		if( reader.ReadOneBit() )
		{
			if( user_data_fixed_size )
			{
				// Don't need to read length - it's fixed length and the length was networked down already
				nBytes = user_data_size;
				if( nBytes <= 0 )
					throw ParsingError_t( "fixed user data size <= 0 in ParseStringTableUpdate" );

				tempbuf[ nBytes - 1 ] = 0; // Be safe, clear last byte
				reader.ReadBits( tempbuf, user_data_size_bits );
			}
			else
			{
				nBytes = reader.ReadUBitLong( MAX_USERDATA_BITS );
				if( nBytes > sizeof( tempbuf ) )
				{
					throw ParsingError_t( "user data too large in ParseStringTableUpdate" );
				}

				reader.ReadBytes( tempbuf, nBytes );
			}

			pUserData = tempbuf;
		}

		if( !pEntry )
		{
			pEntry = "";
		}

#if defined _DEBUG_PRINT_STRINGTABLES
		std::cout << "       L " << entryIndex << ". " << pEntry << " (" << nBytes << " bytes) " << pUserData << std::endl;
#endif

		// Add/update players if this is the userinfo string table
		if( is_user_info && pUserData )
		{
			Player player( *(const Player *)pUserData );
			player.entityIndex = entryIndex + 1;
			player.airstatus = PL_ON_GROUND;
			
			if( m_bIsPOV && m_iPOVPlayerUserID < 0 )
			{
				if( !m_bServerInfoEncountered )
					throw ParsingError_t( "SVC_ServerInfo not encountered by user info update" );

				// Find POV player by entry index, which should be the same as the slot in SVC_ServerInfo
				if( entryIndex == m_iPOVPlayerSlot )
				{
					m_iPOVPlayerUserID = player.userID;
				}
			}

			Player *existing = FindPlayerByEntityIndex( player.entityIndex );

			if( !existing )
			{
				// New player joined an empty slot
				m_Players.push_back( player );
			}
			else
			{
				if( existing->userID != player.userID )
				{
					// A new player is taking the slot of another player who left earlier
					m_ExpiredUserIDs.push_back( existing->userID );
					existing->entityIndex = -1;
					m_Players.push_back( player );
				}
				else
				{
					// Already connected player's information was updated
					// Copy new information but keep old air status and Z just to be safe
					PlayerAirStatus_e oldStatus = existing->airstatus;
					float oldZ = existing->lastZ;

					existing->CopyFrom( player );

					existing->airstatus = oldStatus;
					existing->lastZ = oldZ;
				}
			}
		}

		if( history.size() > 31 )
		{
			history.erase( history.begin() );
		}

		StringHistoryEntry she;
		strncpy_s( she.string, pEntry, sizeof( she.string ) - 1 );
		history.push_back( she );
	}
}

// =====================================================================================================================================================================

const StringTableData_t *DemoParser::GetStringTableData( uint32 tableID )
{
	if( tableID >= m_iNumStringTables )
		return nullptr;

	return &m_StringTables[ tableID ];
}

// =====================================================================================================================================================================