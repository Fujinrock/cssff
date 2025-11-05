#include "DemoParser.h"
#include "Errors.h"
#include "bitbuf.h"
#include "Settings.h"
#include <conio.h>
#include <fstream>

DemoParser *gpParser = nullptr;

extern std::string g_ProgramDirectory;
extern std::string g_BatchOutput;
extern std::vector< ParsingWarning_t > g_WarningDemos;

DemoParser::DemoParser( DemoFile *pDemo )
{
	m_pDemo = pDemo;

	memset( &m_demoHeader, 0, sizeof( demoheader_t ) );

	m_iNumStringTables = 0;

	m_fTickInterval = -1.f;
	m_iTickRate = -1;
	m_bUse5BitStringTableIndices = true;

	m_bIsPOV = false;
	m_iPOVPlayerSlot = -1;
	m_iPOVPlayerUserID = -1;
	m_bPOVPlayerIsDead = false;
	m_iMaxClients = -1;

	m_iCurrentTick = 0;

	m_bServerInfoEncountered = false;
	m_bGameEventListEncountered = false;

	m_iServerClassBits = 0;
	m_iNumStringTables = 0;

	memset( &m_StringTables, 0, sizeof( m_StringTables ) );

	// Only one parser should exist at any given time, so make this the global parser
	gpParser = this;
}

// ==================================================================================================================

DemoParser::~DemoParser()
{
	gpParser = nullptr;
}

// ==================================================================================================================
/**
 * The main parsing function. Reads all the commands and prints the progress bar.
 */
bool DemoParser::Parse( void )
{
	if( !m_pDemo || !m_pDemo->IsValidDemo() )
		return false;

	if( !Settings()->BatchProcessingEnabled() )
	{
		printf( "%s: Parsing demo %s...\n\n", CSSFF_NAME, m_pDemo->GetFileName().c_str() );
		printf( "Press 'Q' to abort early\n\n" );
	}

	bf_read reader( m_pDemo->GetBuffer(), m_pDemo->GetFileSize() );

	// Read the header
	reader.ReadBytes( &m_demoHeader, sizeof( m_demoHeader ) );

	bool bAborted = false;	// Did the user abort parsing?
	bool bSynced = false;	// Was sync tick encountered yet?

	// Set up the progress bar
	const int numProgressDots = Settings()->BatchProcessingEnabled()? 5 : 10;
	const int printProgressTicks = (m_demoHeader.playback_ticks > 0)? (m_demoHeader.playback_ticks / numProgressDots) : (250000 / numProgressDots);
	int lastTick = 0;
	int progressTick = 0;
	int numPrinted = 0;


	// ===== The main parsing loop ===========================================================================

	while( true )
	{
		// Read command type
		byte cmd = reader.ReadByte();

		if( cmd < dem_firstcmd || cmd > dem_lastcmd || reader.IsOverflowed() )
			throw ParsingError_t( "invalid cmd number" );

		// Done parsing?
		if( cmd == dem_stop )
			break;

		// Read current tick
		m_iCurrentTick = reader.ReadLong();

		// Print dots for the progress bar
		if( bSynced )
		{
			progressTick += m_iCurrentTick - lastTick;
			lastTick = m_iCurrentTick;
			if( progressTick >= printProgressTicks )
			{
				printf( "." );
				++numPrinted;
				progressTick = 0;
			}
		}

		// Handle the command
		switch( cmd )
		{
			case dem_synctick:
			{
				if( !m_bServerInfoEncountered )
				{
					throw ParsingError_t( "SVC_ServerInfo not encountered by sync tick" );
				}

				bSynced = true;
				break;
			}

			case dem_stop:
			default:
				break;

			case dem_signon:
			case dem_packet:
			{
				HandleDemoPacket( reader );
				break;
			}

			case dem_consolecmd:
			{
				size_t datasize = reader.ReadLong();
				reader.SeekRelative( BYTES2BITS( datasize ) );
				break;
			}

			case dem_datatables:
			{
				// Fork the reader
				size_t datasize = reader.ReadLong();
				char *data = new char[ datasize ];
				reader.ReadBytes( data, datasize );
				bf_read forkedReader( data, datasize );

				ParseDataTables( forkedReader );
				break;
			}

			case dem_usercmd:
			{
				int32 outgoing_sequence = reader.ReadLong();
				size_t datasize = reader.ReadLong();
				reader.SeekRelative( BYTES2BITS( datasize ) );
				break;
			}
		}

		// Check if the user wants to abort parsing
		if( _kbhit() )
		{
			int ch = toupper( _getch() );

			if( ch == 'Q' )
			{
				bAborted = true;
				break;
			}
		}
	}

	// Print the ending for the progress bar
	const int numMinPrints = bAborted? 3 : numProgressDots;

	if( numPrinted < numMinPrints )
	{
		for( int i = 0; i < numMinPrints - numPrinted; ++i )
			printf( "." );
	}

	if( Settings()->BatchProcessingEnabled() )
	{
		if( bAborted )
			printf( " Parsing aborted by user" );
		else
			printf( " Successfully parsed" );
	}
	else
	{
		if( bAborted )
			printf( "Parsing aborted by user!\n\tNot all frags have necessarily been found.\n\n" );
		else
			printf( "Done parsing!\n\n" );
	}

	// Do post-parsing stuff
	DemoParser::OnParsingEnd();

	return !bAborted;
}

// ==================================================================================================================

float DemoParser::GetTimeBetweenTicks( int tick1, int tick2 ) const
{
	if( tick1 == tick2 )
		return -1;

	int tickDelta = abs( tick1 - tick2 );

	if( m_fTickInterval <= 0 )
		throw ParsingError_t( "invalid tick interval" );

	return (float)tickDelta * m_fTickInterval;
}

// ==================================================================================================================

int DemoParser::GetTickCount( void ) const
{
	return m_demoHeader.playback_ticks;
}

// ==================================================================================================================

int DemoParser::GetCurrentTick( void ) const
{
	return m_iCurrentTick;
}

// ==================================================================================================================

int DemoParser::GetTickRate( void ) const
{
	return m_iTickRate;
}

// ==================================================================================================================
/**
 * Prints demo's found frags into the console and adds the frag description into the output buffer
 */
void DemoParser::OnParsingEnd( void )
{
	// Check for frags again in case the demo ended mid-round
	FindRoundFrags();

	if( !Settings()->BatchProcessingEnabled() )
	{
		if( g_WarningDemos.size() )
		{
			printf( "\n========== WARNINGS ==========\n\n" );

			for( size_t i = 0; i < g_WarningDemos.size(); ++i )
			{
				std::string warning;
				g_WarningDemos[i].GetString( warning );

				char szFailBuffer[300];
				_snprintf_s( szFailBuffer, sizeof( szFailBuffer ), sizeof( szFailBuffer ), "%d. %s\n", i + 1, warning.c_str() );
				printf( szFailBuffer );
			}

			printf( "\n" );
		}
	}

	if( m_Frags.size() > 0 )
	{
		if( Settings()->BatchProcessingEnabled() )
		{
			printf( " (%d frag%s found)\n", m_Frags.size(), m_Frags.size() > 1? "s":"" );
			g_BatchOutput += "========== " + m_pDemo->GetFileName() + ( m_bIsPOV ? " (POV)" : " (STV)" ) + " ==========\n\n";
		}

		std::ofstream file_output;
		std::string filename;

		if( Settings()->DumpToFileEnabled() && !Settings()->BatchProcessingEnabled() )
		{
			filename = m_pDemo->GetFileName();
			RemoveFileExtension( filename );
			filename += ".txt";

			if( Settings()->WriteOutputToDemoDirectory() )
			{
				file_output.open( filename );
			}
			else
			{
				file_output.open( g_ProgramDirectory + filename );
			}
		}

		if( !Settings()->BatchProcessingEnabled() )
			printf( "\n========== FOUND FRAGS ==========\n\n" );

		for( uint32 i = 0; i < m_Frags.size(); ++i )
		{
			char szFragDescription[ 1024 ];
			m_Frags[ i ].GetStringRepresentation( szFragDescription, sizeof(szFragDescription) );

			if( !Settings()->BatchProcessingEnabled() )
			{
				printf( szFragDescription );

				if( Settings()->DumpToFileEnabled() && file_output.is_open() )
					file_output.write( szFragDescription, strlen( szFragDescription ) );
			}
			else
			{
				g_BatchOutput += szFragDescription;
			}
		}

		if( Settings()->DumpToFileEnabled() && !Settings()->BatchProcessingEnabled() && file_output.is_open() )
		{
			if( !Settings()->BatchProcessingEnabled() )
				printf( "Output has been written to file %s in %s folder\n\n", filename.c_str(), Settings()->WriteOutputToDemoDirectory()? "demo's" : "program" );

			file_output.close();
		}
	}
	else
	{
		if( Settings()->BatchProcessingEnabled() )
		{
			printf( " (no frags found)\n" );
		}
		else
		{
			printf( "\nNo frags found with the current settings.\n\n" );
		}
	}
}

// ==================================================================================================================