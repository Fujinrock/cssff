#include "DemoParser.h"
#include "Errors.h"
#include "bitbuf.h"
#include "Settings.h"
#include <conio.h>
#include <fstream>
#include <format>
#include <filesystem>

DemoParser *gpParser = nullptr;

extern std::string g_ProgramDirectory;
extern std::string g_BatchDirectory;
extern std::string g_BatchOutput;
extern std::vector< ParsingWarning_t > g_WarningDemos;
extern std::vector< std::string > g_FailedDemos;

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
	m_iNewRoundTick = 0;

	m_bServerInfoEncountered = false;
	m_bGameEventListEncountered = false;

	m_iServerClassBits = 0;
	m_iNumStringTables = 0;

	m_iMaxProgressBarDots = Settings()->BatchProcessingEnabled()? 5 : 10;
	m_iProgressBarDotsPrinted = 0;

	memset( &m_PropIndices, 0, sizeof( m_PropIndices ) );
	memset( &m_StringTables, 0, sizeof( m_StringTables ) );

	// Only one parser should exist at any given time, so make this the global parser
	assert( !gpParser );
	gpParser = this;
}

// ==================================================================================================================

DemoParser::~DemoParser()
{
	gpParser = nullptr;

	const size_t num_ents = m_Entities.size();
	for( size_t i = 0; i < num_ents; ++i )
	{
		delete m_Entities[ i ];
	}
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

	m_reader.StartReading( m_pDemo->GetBuffer(), m_pDemo->GetFileSize() );

	// Read the header
	m_reader.ReadBytes( &m_demoHeader, sizeof( m_demoHeader ) );

	bool bAborted = false;	// Did the user abort parsing?
	bool bSynced = false;	// Was sync tick encountered yet?

	// Set up the progress bar
	const int iProgressBarTickInterval = (m_demoHeader.playback_ticks > 0)? (m_demoHeader.playback_ticks / m_iMaxProgressBarDots) : (500'000 / m_iMaxProgressBarDots);
	int iLastTick = 0;
	int iTicksProgressed = 0;


	// ===== The main parsing loop ===========================================================================

	while( true )
	{
		// Read command type
		byte cmd = m_reader.ReadByte();

		if( cmd < dem_firstcmd || cmd > dem_lastcmd || m_reader.IsOverflowed() )
			throw ParsingError_t( "invalid cmd number" );

		// Done parsing?
		if( cmd == dem_stop )
			break;

		// Read current tick
		m_iCurrentTick = m_reader.ReadLong();

		// Print dots for the progress bar
		if( bSynced )
		{
			iTicksProgressed += m_iCurrentTick - iLastTick;
			iLastTick = m_iCurrentTick;
			if( iTicksProgressed >= iProgressBarTickInterval )
			{
				printf( "." );
				++m_iProgressBarDotsPrinted;
				iTicksProgressed = 0;
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
				HandleDemoPacket( m_reader );
				break;
			}

			case dem_consolecmd:
			{
				size_t datasize = m_reader.ReadLong();
				m_reader.SeekRelative( BYTES2BITS( datasize ) );
				break;
			}

			case dem_datatables:
			{
				// Fork the reader
				size_t datasize = m_reader.ReadLong();
				char *data = new char[ datasize ];
				m_reader.ReadBytes( data, datasize );
				bf_read forkedReader( data, datasize );

				ParseDataTables( forkedReader );
				delete[] data;
				break;
			}

			case dem_usercmd:
			{
				int32 outgoing_sequence = m_reader.ReadLong();
				size_t datasize = m_reader.ReadLong();
				m_reader.SeekRelative( BYTES2BITS( datasize ) );
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

	// Do post-parsing stuff
	DemoParser::OnParsingEnd( bAborted? ABORTED : DONE );

	return !bAborted;
}

// ==================================================================================================================

float DemoParser::GetTimeBetweenTicks( int tick1, int tick2 ) const
{
	if( tick1 == tick2 )
		return 0.f;

	int tickDelta = abs( tick1 - tick2 );

	if( m_fTickInterval <= 0.f )
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

int DemoParser::GetNumBytesLeft( void ) const
{
	return m_reader.GetNumBytesLeft();
}

// ==================================================================================================================

int DemoParser::GetNumFragsFound( void ) const
{
	return m_Frags.size();
}

// ==================================================================================================================
/**
 * Prints demo's found frags into the console and adds the frag description into the output buffer
 */
void DemoParser::OnParsingEnd( ParsingResult result, ParsingError_t *pError )
{
	// Check for frags again in case the demo ended mid-round
	FindRoundFrags();

	// Print the rest of the progress bar
	const int iMinProgressBarDots = (result != DONE) ? 3 : m_iMaxProgressBarDots;

	if( m_iProgressBarDotsPrinted < iMinProgressBarDots )
	{
		for( int i = 0; i < iMinProgressBarDots - m_iProgressBarDotsPrinted; ++i )
			printf( "." );
	}

	// Add this demo to the parsing error list if there was an error
	if( result == ERROR && pError )
	{
		std::string sError = std::format( "{}: {} on tick {}\n", m_pDemo->GetFileName(), pError->error_msg, pError->tick );
		g_FailedDemos.emplace_back( sError );
	}

	// Print the appropriate ending message
	if( Settings()->BatchProcessingEnabled() )
	{
		if( result == DONE )
			printf( " Successfully parsed" );
		else if( result == ERROR )
			printf( " Error encountered" );
		else
			printf( " Parsing aborted by user" );
	}
	else
	{
		if( result == DONE )
			printf( "Done parsing!\n\n\n" );
		else if( result == ERROR )
			printf( "Error encountered!\n\n\n" );
		else
			printf( "Parsing aborted by user!\n\tNot all frags have necessarily been found.\n\n\n" );
	}

	// Add frag info to batch output if we're batch processing
	if( Settings()->BatchProcessingEnabled() )
	{
		if( m_Frags.empty() )
		{
			printf( " (no frags found)\n" );
			return;
		}

		printf( " (%d frag%s found)\n", m_Frags.size(), m_Frags.size() > 1? "s":"" );
		g_BatchOutput 
			+= "========== "
			+ m_pDemo->GetFileName()
			+ " ("
			+ (m_bIsPOV ? "POV" : "STV")
			+ " @ "
			+ m_demoHeader.mapname
			+ ") ==========\n\n";

		for( const Frag &frag : m_Frags )
		{
			char szFragDescription[ 1024 ];
			frag.GetStringRepresentation( szFragDescription, sizeof(szFragDescription) );
			g_BatchOutput += szFragDescription;
		}
	}
	else // Not batch processing - print output to console and write to file if need be
	{
		// TODO: Make all the program's file writing logic uniform and in one place
		// (Header is not currently written)
		std::ofstream file_output;
		std::string filename;
		std::string buffer;
		const bool bDumpToFile = Settings()->DumpToFileEnabled() && (!g_WarningDemos.empty() || !g_FailedDemos.empty() || !m_Frags.empty());

		const bool bShouldWriteToDemoDir = bDumpToFile && Settings()->ShouldWriteOutputToDemoDirectory();
		bool bCantOverwriteFile = false;

		if( bDumpToFile )
		{
			filename = m_pDemo->GetFileName();
			RemoveFileExtension( filename );
			filename += ".txt";

			std::string filepath = (bShouldWriteToDemoDir ? g_BatchDirectory : g_ProgramDirectory) + filename;

			if( std::filesystem::exists( filepath ) )
			{
				std::ifstream file( filepath, std::ios::binary );

				if( file.is_open() )
				{
					std::string line;
					std::getline( file, line );

					if( line.find( "FOUND FRAGS:" ) != std::string::npos )
						file_output.open( filepath, std::ios::binary );
					else
						bCantOverwriteFile = true;

					file.close();
				}
				else
				{
					bCantOverwriteFile = true;
				}
			}
			else
			{
				file_output.open( filepath, std::ios::binary );
			}

			if( file_output.is_open() )
			{
				WRITE_UTF8_BOM( file_output );

				// Write the singular error in to the file
				if( result == ERROR && pError )
				{
					buffer = std::format( "PARSING ERROR: {} on tick {}\n\n", pError->error_msg, pError->tick );
					file_output.write( buffer.c_str(), buffer.length() );
				}
			}
		}

		// Print/write warnings
		if( !g_WarningDemos.empty() )
		{
			printf( "========== WARNINGS ==========\n\n" );

			if( bDumpToFile && file_output.is_open() )
			{
				buffer = "WARNINGS:\n";
				file_output.write( buffer.c_str(), buffer.length() );
			}

			for( const ParsingWarning_t &warning : g_WarningDemos )
			{
				std::string sWarning;
				warning.GetString( sWarning, false );

				buffer = std::format( "- {}", sWarning );
				printf( buffer.c_str() );

				if( bDumpToFile && file_output.is_open() )
				{
					file_output.write( buffer.c_str(), buffer.length() );
				}
			}

			printf( "\n\n" );

			if( bDumpToFile && file_output.is_open() )
				file_output << "\n";
		}

		printf( "========== FOUND FRAGS ==========\n\n" );

		if( bDumpToFile && file_output.is_open() )
			file_output << "FOUND FRAGS:\n\n";

		// Print/write frags
		if( m_Frags.empty() )
		{
			buffer = "No frags found with the used settings";

			if( bDumpToFile && file_output.is_open() )
				file_output.write( buffer.c_str(), buffer.length() );

			buffer += "\n\n";

			printf( buffer.c_str() );
		}
		else
		{
			for( const Frag &frag : m_Frags )
			{
				char szFragDescription[ 1024 ];
				frag.GetStringRepresentation( szFragDescription, sizeof(szFragDescription) );
				printf( szFragDescription );

				if( bDumpToFile && file_output.is_open() )
					file_output.write( szFragDescription, strlen( szFragDescription ) );
			}
		}

		if( bDumpToFile )
		{
			if( file_output.is_open() )
			{
				printf( "Info has been written to file \"%s\" in %s folder\n\n", filename.c_str(), bShouldWriteToDemoDir ? "demo's" : "program" );

				file_output.close();
			}
			else
			{
				printf( "Failed to write info to file%s\n\n", bCantOverwriteFile ? " (cannot overwrite existing file)" : "" );
			}
		}
	}

	// Write VDM if need be
	// This is written here regardless of if we're batch-processing or not
	if( Settings()->ShouldWriteVDM() )
	{
		WriteVDM();
	}
}

// ==================================================================================================================

void DemoParser::WriteVDM( void ) const
{
	if( m_Frags.empty() )
		return;

	const std::string vdmSubDir = Settings()->GetVDMDirectory();

	const bool bShouldWriteToDemoDir = Settings()->ShouldWriteOutputToDemoDirectory();

	std::string writeToDir = bShouldWriteToDemoDir ? g_BatchDirectory : g_ProgramDirectory;

	// Check whether we should create a folder for the VDM subdirectory
	if( !vdmSubDir.empty() )
	{
		size_t pos = writeToDir.length() < 2 ? std::string::npos : writeToDir.find_last_of( "/\\", writeToDir.length() - 2 );

		if( pos == std::string::npos || _stricmp( vdmSubDir.c_str(), (writeToDir.substr( pos + 1, (writeToDir.length() - 1) - (pos + 1) )).c_str() ) != 0 )
		{
			if( !std::filesystem::exists( writeToDir + vdmSubDir ) )
			{
				std::filesystem::create_directories( writeToDir + vdmSubDir );
			}

			writeToDir += (vdmSubDir + '\\');
		}
	}

	std::string filename = m_pDemo->GetFileName();
	RemoveFileExtension( filename );
	const std::string filepath = writeToDir + filename + ".vdm";

	// Check if we dare to overwrite an already existing file by this name
	if( std::filesystem::exists( filepath ) )
	{
		std::ifstream vdm( filepath, std::ios::binary );

		if( !vdm.is_open() )
		{
			printf( "%sFailed to check existing VDM file - writing aborted!\n", Settings()->BatchProcessingEnabled() ? " L " : "" );
			return;
		}

		std::string line;
		std::getline( vdm, line );
		vdm.close();

		if( line != "demoactions" )
		{
			printf( "%sCannot overwrite existing VDM file!\n", Settings()->BatchProcessingEnabled() ? " L " : "" );
			return;
		}
	}

	std::ofstream vdm_output( filepath, std::ios::binary );

	if( !vdm_output.is_open() )
	{
		printf( "%sFailed to write VDM file!\n", Settings()->BatchProcessingEnabled() ? " L " : "" );
		return;
	}

	vdm_output << "demoactions\n{\n";

	// Action number can be the same for every action, which makes it easier to edit the file later on
	const int actionNumber = 1;
	const int specSetupTicks = 15;

	// This will be used when chaining
	vdm_output << "\t\"" << actionNumber << "\"\n\t{\n"
		<< "\t\tfactory \"PlayCommands\"\n"
		<< "\t\tname \"_ondemostart\"\n"
		<< "\t\tstarttick \"" << specSetupTicks << "\"\n"
		<< "\t\tcommands \"alias gotonextfrag echo nonext";

	if( !m_bIsPOV )
	{
		vdm_output << ";spec_menu hide;spec_mode;hidepanel all";
	}
	
	vdm_output << "\"\n\t}\n";
	
	int gotoNextFragTick = specSetupTicks + 5;
	bool bWroteCfg = false;

	for( size_t iFrag = 0; iFrag < m_Frags.size(); ++iFrag )
	{
		const Frag &frag = m_Frags[ iFrag ];

		const int nextFragTick = std::max( frag.GetStartTick() - 3 * GetTickRate(), specSetupTicks );

		if( nextFragTick > gotoNextFragTick + GetTickRate() )
		{
			vdm_output << "\t\"" << actionNumber << "\"\n\t{\n"
				<< "\t\tfactory \"PlayCommands\"\n"
				<< "\t\tname \"_gotonextfrag\"\n"
				<< "\t\tstarttick \"" << gotoNextFragTick << "\"\n"
				<< "\t\tcommands \"demo_gototick " << nextFragTick << "\"\n\t}\n";
		}
		
		gotoNextFragTick = frag.GetEndTick() + 2 * GetTickRate();

		// GetTickCount can return 0, so in those cases m_iCurrentTick will hopefully contain the last actual tick
		// If there will be problems, another variable might be required that saves the highest reached tick,
		// in case m_iCurrentTick goes back down at the end of the demo
		const int lastTick = GetTickCount() == 0 ? m_iCurrentTick : GetTickCount();
		assert( lastTick > 50 );

		// Make sure next gototick doesn't happen after the demo ends
		if( gotoNextFragTick > lastTick )
		{
			gotoNextFragTick = std::max( frag.GetEndTick(), lastTick - 15 );
		}

		std::string cfgName = "__" + filename + (iFrag > 0 ? ("_" + std::to_string(iFrag + 1)) : "") + ".cfg";

		vdm_output << "\t\"" << actionNumber << "\"\n\t{\n"
			<< "\t\tfactory \"PlayCommands\"\n"
			<< "\t\tname \"_onfragstart\"\n"
			<< "\t\tstarttick \"" << nextFragTick + specSetupTicks << "\"\n"
			<< "\t\tcommands \"alias gotonextfrag " << (iFrag < m_Frags.size() - 1 ? ("demo_gototick " + std::to_string( gotoNextFragTick + 10 )) : "echo nonext");

		bool bUseCfg = false;
		if( !m_bIsPOV )
		{
			bool bNameHasSpaces = StrHasSpaces( frag.GetPlayername() );

			if( Settings()->ShouldWriteVDMConfig() && bNameHasSpaces )
			{
				vdm_output << ";exec " << cfgName << ((vdmSubDir.empty() || !_stricmp( vdmSubDir.c_str(), "cstrike" )) ? "" : (" */" + vdmSubDir));
				bUseCfg = true;
			}
			else
			{
				if( bNameHasSpaces )
					vdm_output << ";spec_player \\\"" << frag.GetPlayername() << "\\\"";
				else
					vdm_output << ";spec_player " << frag.GetPlayername();
			}
		}
		
		vdm_output << "\"\n\t}\n";

		// If this is the last frag, write a dummy command at the end so we can chain this easily later
		if( iFrag == m_Frags.size() - 1 )
		{
			vdm_output << "\t\"" << actionNumber << "\"\n\t{\n"
				<< "\t\tfactory \"PlayCommands\"\n"
				<< "\t\tname \"_onlastfragend\"\n"
				<< "\t\tstarttick \"" << std::min(gotoNextFragTick + GetTickRate(), lastTick - 15) << "\"\n"
				<< "\t\tcommands \"\"\n\t}\n";
		}

		// Write the cfg file
		if( bUseCfg )
		{
			if( !std::filesystem::exists( writeToDir + "cfg" ) )
			{
				std::filesystem::create_directories( writeToDir + "cfg" );
			}

			std::ofstream cfg_output( writeToDir + "cfg\\" + cfgName, std::ios::binary );

			if( !cfg_output.is_open() )
			{
				printf( "%sFailed to write VDM config file for frag %d!\n", Settings()->BatchProcessingEnabled() ? " L " : "", iFrag + 1 );
				continue;
			}

			cfg_output << "spec_player \"" << frag.GetPlayername() << "\"";

			cfg_output.close();

			bWroteCfg = true;
		}
	}

	vdm_output << '}';

	vdm_output.close();

	if( !Settings()->BatchProcessingEnabled() )
	{
		const std::string writeDir = bShouldWriteToDemoDir ? "demo's" : "program";
		if( vdmSubDir.empty() )
		{
			printf( "VDM file %s been written to %s folder\n\n", bWroteCfg ? "and its configs have" : "has", writeDir.c_str() );
		}
		else
		{
			printf( "VDM file %s been written to \"%s\" in %s folder\n\n", bWroteCfg ? "and its configs have" : "has", vdmSubDir.c_str(), writeDir.c_str() );
		}
	}
}

// ==================================================================================================================