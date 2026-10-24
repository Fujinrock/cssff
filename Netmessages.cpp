#include "Netmessages.h"
#include "DemoParser.h"
#include "Errors.h"
#include "Player.h"
#include "Settings.h"
#include <cstdio>
#include <cstdint>

// =====================================================================================================================================================================

void DemoParser::HandleNETDisconnect( bf_read &reader )
{
	char reason[ 1024 ];
	reader.ReadString( reason, sizeof( reason ) );
}

// =====================================================================================================================================================================

void DemoParser::HandleNETFile( bf_read &reader )
{
	const int transferid = reader.ReadLong();
	char filename[ 1024 ];
	reader.ReadString( filename, sizeof( filename ) );
	const bool filerequested = reader.ReadOneBit();
}

// =====================================================================================================================================================================

void DemoParser::HandleNETTick( bf_read &reader )
{
	const int tick = reader.ReadLong();
}

// =====================================================================================================================================================================

void DemoParser::HandleNETStringCmd( bf_read &reader )
{
	char command[ 1024 ];
	reader.ReadString( command, sizeof( command ) );
}

// =====================================================================================================================================================================

void DemoParser::HandleNETSetConVar( bf_read &reader )
{
	int num = reader.ReadByte();
	while ( num-- > 0 )
	{
		char convar[ 256 ];
		reader.ReadString( convar, sizeof( convar ) );
		char value[ 256 ];
		reader.ReadString( value, sizeof( value ) );
	}
}

// =====================================================================================================================================================================

void DemoParser::HandleNETSignOnState( bf_read &reader )
{
	const int signonstate = reader.ReadByte();
	const int spawncount = reader.ReadLong();
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCPrint( bf_read &reader )
{
	char message[ 1024 ];
	reader.ReadString( message, sizeof( message ) );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCServerInfo( bf_read &reader )
{
	if ( m_bServerInfoEncountered )
	{
		throw ParsingError_t( "SVC_ServerInfo second encounter" );
	}

	m_bServerInfoEncountered = true;

	const int protocol = reader.ReadShort();
	const int servercount = reader.ReadLong();
	const bool ishltv = reader.ReadOneBit();
	const bool isdedicated = reader.ReadOneBit();
	const int clientcrc = reader.ReadLong();
	const int maxclasses = reader.ReadShort();
	const int mapcrc = reader.ReadLong();
	const int playerslot = reader.ReadByte();
	const int maxclients = reader.ReadByte();
	const float tickinterval = reader.ReadFloat();
	const char platform = reader.ReadChar();

	// Save useful info for later
	m_iMaxClients = maxclients;
	m_fTickInterval = tickinterval;
	m_iTickRate = (int)floor( 1 / m_fTickInterval );
	m_bIsPOV = !ishltv;
	m_iPOVPlayerSlot = playerslot;

	if ( platform != 'w' && platform != 'l' )
		m_bUse5BitStringTableIndices = false;

	char gamedir[ 256 ];
	reader.ReadString( gamedir, sizeof( gamedir ) );

	char mapname[ 256 ];
	reader.ReadString( mapname, sizeof( mapname ) );

	char skyname[ 256 ];
	reader.ReadString( skyname, sizeof( skyname ) );

	char hostname[ 256 ];
	reader.ReadString( hostname, sizeof( hostname ) );

	// Print demo info if this is the only demo being parsed
	if ( !Settings()->BatchProcessingEnabled() )
	{
		float demoLength = m_demoHeader.playback_time;
		const int demoTicks = m_demoHeader.playback_ticks;

		printf( "\n========== DEMO INFO ==========\n\n" );
		printf( "\t%s demo\n", ishltv ? "STV" : "POV" );
		printf( "\tProtocol version: %d\n", protocol );
#ifdef _DEBUG
		printf( "\tUsing %d bit string table indices\n", m_bUse5BitStringTableIndices ? 5 : 4 );
#endif
		if ( !demoTicks && !demoLength )
		{
			printf( "\tDemo length: unknown\n" );
			printf( "\tDemo ticks: unknown\n" );
		}
		else
		{
			demoLength += 0.5f;
			const int hours = (int)demoLength / 60 / 60;
			const int minutes = (int)demoLength / 60 - hours * 60;
			const int seconds = (int)fmodf( demoLength, 60 );

			printf( "\tDemo length: " );
			if ( hours )
			{
				printf( "%d:%02d:%02d\n", hours, minutes, seconds );
			}
			else
			{
				printf( "%d:%02d\n", minutes, seconds );
			}

			printf( "\tDemo ticks: %d\n", demoTicks );
		}

		printf( "\tTickrate: %d\n", m_iTickRate );
		printf( "\tMap name: %s\n", mapname );
		if ( !ishltv )
			printf( "\tClient name: %s\n", m_demoHeader.clientname );

		printf( "\tServer name: %s\n", ishltv ? m_demoHeader.servername : hostname );
		if ( !ishltv )
			printf( "\tServer IP: %s\n", m_demoHeader.servername );
		else
			printf( "\tSourceTV name: %s\n", hostname );
		printf( "\tServer type: %s server\n", isdedicated ? "dedicated" : "listen" );
#ifndef _DEBUG
		printf( "\tServer OS: %s\n", tolower( platform ) == 'w' ? "Windows" : "Linux" );
#else
		printf( "\tServer OS: %c\n", platform );
#endif
		printf( "\tServer max clients: %d\n", maxclients );
		printf( "\n\n\tParsing in progress" );
	}
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCSendTable( bf_read &reader )
{
	const bool needsdecoder = reader.ReadOneBit();
	const int datasize = reader.ReadShort();
	reader.SeekRelative( datasize );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCClassInfo( bf_read &reader )
{
	const int num = reader.ReadShort();
	const bool createonclient = reader.ReadOneBit();
	if ( !createonclient )
	{
		throw ParsingError_t( "class info not created on client" );

		/*while( num-- > 0 )
		{
			int classid = 0;
			reader.ReadBits( &classid, (int)Log2( num ) + 1 );
			char classname[ 256 ];
			reader.ReadString( classname, sizeof(classname) );
			char datatablename[ 256 ];
			reader.ReadString( datatablename, sizeof(datatablename) );
		}*/
	}
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCSetPause( bf_read &reader )
{
	const bool paused = reader.ReadOneBit();
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCCreateStringTable( bf_read &reader )
{
	char name[ 512 ];
	reader.ReadString( name, sizeof( name ) );

	const int max_entries = reader.ReadShort();
	const int num_entries = reader.ReadUBitLong( (int)( Log2( max_entries ) + 1 ) );
	const int datasize = reader.ReadUBitLong( 20 ); // In bits

	const bool user_data_fixed_size = reader.ReadOneBit();
	int user_data_size = 0;
	int user_data_size_bits = 0;

	if ( user_data_fixed_size )
	{
		reader.ReadBits( &user_data_size, 12 );
		reader.ReadBits( &user_data_size_bits, 4 );
	}

	if ( !name || strcmp( name, "" ) == 0 )
	{
		throw ParsingError_t( "tried to create bogus string table" );
	}

	const StringTableData_t *pTable = CreateStringTable( name, max_entries, user_data_size, user_data_size_bits, user_data_fixed_size );

	ParseStringTableUpdate( reader, num_entries, pTable );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCUpdateStringTable( bf_read &reader )
{
	const int tableID = reader.ReadUBitLong( m_bUse5BitStringTableIndices ? 5 : 4 );

	const bool multiple_changed_entries = reader.ReadOneBit();
	const int num_changed_entries = multiple_changed_entries ? reader.ReadShort() : 1;

	const int datasize = reader.ReadWord(); // In bits

	const StringTableData_t *pTable = GetStringTableData( tableID );

	if ( pTable && pTable->nMaxEntries > num_changed_entries )
	{
		ParseStringTableUpdate( reader, num_changed_entries, pTable );
	}
	else
	{
		throw ParsingError_t( "bad string table update" );
	}
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCVoiceInit( bf_read &reader )
{
	char codec[ 256 ];
	reader.ReadString( codec, sizeof( codec ) );
	const int quality = reader.ReadByte();
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCVoiceData( bf_read &reader )
{
	const int client = reader.ReadByte();
	const int datalength = reader.ReadWord();
	reader.SeekRelative( datalength );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCSounds( bf_read &reader )
{
	const bool reliablesound = reader.ReadOneBit();
	const int numsounds = reliablesound ? 1 : reader.ReadByte();
	const int datalength = reliablesound ? reader.ReadByte() : reader.ReadShort();
	reader.SeekRelative( datalength );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCSetView( bf_read &reader )
{
	const int entityindex = reader.ReadUBitLong( 11 );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCFixAngle( bf_read &reader )
{
	const bool relative = reader.ReadOneBit();
	QAngle angle;
	angle.x = reader.ReadBitAngle( 16 );
	angle.y = reader.ReadBitAngle( 16 );
	angle.z = reader.ReadBitAngle( 16 );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCCrosshairAngle( bf_read &reader )
{
	QAngle angle;
	angle.x = reader.ReadBitAngle( 16 );
	angle.y = reader.ReadBitAngle( 16 );
	angle.z = reader.ReadBitAngle( 16 );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCBSPDecal( bf_read &reader )
{
	Vector pos;
	reader.ReadBitVec3Coord( pos );

	const int decaltextureindex = reader.ReadUBitLong( 9 );
	const bool onentity = reader.ReadOneBit();

	int entityindex = 0;
	int modelindex = 0;

	if ( onentity )
	{
		// These are untested (unseen) and might be wrong
		entityindex = reader.ReadUBitLong( 11 );
		modelindex = reader.ReadUBitLong( 11 );
	}

	const bool lowpriority = reader.ReadOneBit();
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCUserMessage( bf_read &reader )
{
	const int msgtype = reader.ReadByte();
	const int datalength = reader.ReadUBitLong( 11 );
	reader.SeekRelative( datalength );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCEntityMessage( bf_read &reader )
{
	const int entityindex = reader.ReadUBitLong( 11 );
	const int classid = reader.ReadUBitLong( 9 );
	const int datalength = reader.ReadUBitLong( 11 );

	reader.SeekRelative( datalength );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCGameEvent( bf_read &reader )
{
	const int datalength = reader.ReadUBitLong( 11 );

	HandleGameEvent( reader );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCPacketEntities( bf_read &reader )
{
	const int maxentries = reader.ReadUBitLong( 11 );
	const bool isdelta = reader.ReadOneBit();
	const int deltafrom = isdelta ? reader.ReadLong() : -1;
	const int baseline = reader.ReadUBitLong( 1 );
	const int updatedentries = reader.ReadUBitLong( 11 );
	const int datalength = reader.ReadUBitLong( 20 );
	const bool updatebaseline = reader.ReadOneBit();

	// Fork the reader
	int databytes = BITS2BYTES( datalength );
	char *data = new char[ databytes ];
	reader.ReadBits( data, datalength );
	bf_read forkedReader( data, databytes );

	ProcessPacketEntities( forkedReader, updatedentries, isdelta );

	delete[] data;
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCTempEntities( bf_read &reader )
{
	const int numentries = reader.ReadByte();
	const int datalength = reader.ReadUBitLong( 17 );

	reader.SeekRelative( datalength );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCPrefetch( bf_read &reader )
{
	const int soundindex = reader.ReadUBitLong( 13 );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCMenu( bf_read &reader )
{
	const int menutype = reader.ReadShort();
	const int datalength = reader.ReadWord();
	reader.SeekRelative( BYTES2BITS( datalength ) );
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCGameEventList( bf_read &reader )
{
	if ( m_bGameEventListEncountered )
		throw ParsingError_t( "SVC_GameEventList second encounter" );

	m_bGameEventListEncountered = true;

	int events = reader.ReadUBitLong( 9 );
	const int datalength = reader.ReadUBitLong( 20 );

	const int dataBytes = BITS2BYTES( datalength );
	char *data = new char[ dataBytes ];
	reader.ReadBits( data, datalength );
	bf_read eventdata( data, dataBytes );

	while ( events-- > 0 )
	{
		ParseGameEvent( eventdata );
	}

	delete[] data;
}

// =====================================================================================================================================================================

void DemoParser::HandleSVCGetCvarValue( bf_read &reader )
{
	const int cookie = reader.ReadLong();
	char cvarname[ 512 ];
	reader.ReadString( cvarname, sizeof( cvarname ) );
}

// =====================================================================================================================================================================

void DemoParser::HandleDemoPacket( bf_read &reader )
{
	// Skip reading unneeded information at the beginning of the packet (demo cmd info + nSeqNrIn & nSeqNrOut)
	reader.SeekRelative( BYTES2BITS( ( sizeof( democmdinfo_t ) + 2 * sizeof( int32_t ) ) ) );

	const int datasize = reader.ReadLong(); // In bytes

	// Fork the reader to avoid problems with read bytes mismatch
	char *data = new char[ datasize ];
	reader.ReadBytes( data, datasize );
	bf_read packetreader( data, datasize );

	while ( packetreader.GetNumBytesRead() < datasize )
	{
		const byte msg = packetreader.ReadUBitLong( 5 );

		if ( msg < 0 || msg >= NumMessageTypes || packetreader.IsOverflowed() )
		{
			throw ParsingError_t( "invalid NET/SVC message type encountered" );
		}

		switch ( msg )
		{
			default:
			{
				throw ParsingError_t( "invalid NET/SVC message type encountered" );
			}
			case NET_NOP:
			{
				break;
			}
			case NET_Disconnect:
			{
				HandleNETDisconnect( packetreader );
				break;
			}
			case NET_File:
			{
				HandleNETFile( packetreader );
				break;
			}
			case NET_Tick:
			{
				HandleNETTick( packetreader );
				break;
			}
			case NET_StringCmd:
			{
				HandleNETStringCmd( packetreader );
				break;
			}
			case NET_SetConVar:
			{
				HandleNETSetConVar( packetreader );
				break;
			}
			case NET_SignOnState:
			{
				HandleNETSignOnState( packetreader );
				break;
			}
			case SVC_Print:
			{
				HandleSVCPrint( packetreader );
				break;
			}
			case SVC_ServerInfo:
			{
				HandleSVCServerInfo( packetreader );
				break;
			}
			case SVC_SendTable:
			{
				HandleSVCSendTable( packetreader );
				break;
			}
			case SVC_ClassInfo:
			{
				HandleSVCClassInfo( packetreader );
				break;
			}
			case SVC_SetPause:
			{
				HandleSVCSetPause( packetreader );
				break;
			}
			case SVC_CreateStringTable:
			{
				HandleSVCCreateStringTable( packetreader );
				break;
			}
			case SVC_UpdateStringTable:
			{
				HandleSVCUpdateStringTable( packetreader );
				break;
			}
			case SVC_VoiceInit:
			{
				HandleSVCVoiceInit( packetreader );
				break;
			}
			case SVC_VoiceData:
			{
				HandleSVCVoiceData( packetreader );
				break;
			}
			case SVC_Sounds:
			{
				HandleSVCSounds( packetreader );
				break;
			}
			case SVC_SetView:
			{
				HandleSVCSetView( packetreader );
				break;
			}
			case SVC_FixAngle:
			{
				HandleSVCFixAngle( packetreader );
				break;
			}
			case SVC_CrosshairAngle:
			{
				HandleSVCCrosshairAngle( packetreader );
				break;
			}
			case SVC_BSPDecal:
			{
				HandleSVCBSPDecal( packetreader );
				break;
			}
			case SVC_UserMessage:
			{
				HandleSVCUserMessage( packetreader );
				break;
			}
			case SVC_EntityMessage:
			{
				HandleSVCEntityMessage( packetreader );
				break;
			}
			case SVC_GameEvent:
			{
				HandleSVCGameEvent( packetreader );
				break;
			}
			case SVC_PacketEntities:
			{
				HandleSVCPacketEntities( packetreader );
				break;
			}
			case SVC_TempEntities:
			{
				HandleSVCTempEntities( packetreader );
				break;
			}
			case SVC_Prefetch:
			{
				HandleSVCPrefetch( packetreader );
				break;
			}
			case SVC_Menu:
			{
				HandleSVCMenu( packetreader );
				break;
			}
			case SVC_GameEventList:
			{
				HandleSVCGameEventList( packetreader );
				break;
			}
			case SVC_GetCvarValue:
			{
				HandleSVCGetCvarValue( packetreader );
				break;
			}
		}
	}

	delete[] data;

	// Do flickshot/jumpshot checks after packet entities have been processed
	DoPlayersPostCheck();

	// Add a warning if # of bytes read doesn't match the supposed size of the packet
	if ( datasize != packetreader.GetNumBytesRead() )
	{
		AddWarning( m_pDemo->GetFileName(), BYTE_MISMATCH );
	}
}

// =====================================================================================================================================================================