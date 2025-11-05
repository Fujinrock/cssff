#include "DemoFile.h"
#include "DemoParser.h"
#include "Common.h"
#include "Errors.h"
#include "Settings.h"
#include <Windows.h>
#include <direct.h>
#include <stdio.h>
#include <vector>
#include <ctime>
#include <fstream>
#include <format>

std::string g_ProgramDirectory;						///< Program executable directory (with '\' in the end)
std::string g_BatchDirectory;						///< Directory of the batch to be processed (with '\' in the end)
std::string g_BatchOutput;							///< Buffer where all the found frags will be written during batch processing
static std::vector< std::string > s_DemosToParse;	///< Filenames of all the demos that will be parsed
static std::vector< std::string > g_FailedDemos;	///< Filenames of the demos that failed to parse and their error messages
extern std::vector< ParsingWarning_t > g_WarningDemos;

/**
 * Populates the parsing list with demos in the specified directory
 * @param sDirectory			path to the directory
 * @return						false if failed to open directory or find first file, true otherwise
 */
bool FindDemosInFolder( const std::string &sDirectory )
{
	std::string strSearchPath = sDirectory + "*.dem";

	WIN32_FIND_DATAA data;

	HANDLE hFile = FindFirstFileA( strSearchPath.c_str(), &data );

	if( hFile == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	do
	{
		if( !( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			s_DemosToParse.emplace_back( sDirectory + data.cFileName );
		}
	}
	while( FindNextFileA( hFile, &data ) != 0 );

	FindClose( hFile );

#ifdef _DEBUG_PRINT_DETAILS
	printf( "Found demo files:\n" );
	for( size_t i = 0; i < s_DemosToParse.size(); ++i )
	{
		printf( "%d. %s\n", i+1, s_DemosToParse[ i ].c_str() );
	}
#endif

	return true;
}

/**
 * Writes the results of the batch process to a file
 * @param bAborted				whether the process was aborted or not
 * @return						true if successfully written, false otherwise
 */
bool WriteBatchOutput( bool bAborted )
{
	tm timeinfo;
	time_t rawtime;
	time( &rawtime );
	localtime_s( &timeinfo, &rawtime );

	char szOutputFile[ MAX_PATH ];
	strftime( szOutputFile, sizeof(szOutputFile), "_cssff_batch_%y-%m-%d_%H%M%S", &timeinfo );
	if( !bAborted )
		_snprintf_s( szOutputFile, sizeof(szOutputFile), sizeof(szOutputFile), "%s_%d.txt", szOutputFile, s_DemosToParse.size() );
	else
		strcat_s( szOutputFile, sizeof(szOutputFile), "_aborted.txt" );

	std::ofstream file_output;

	// Where should the file be written to?
	if( Settings()->WriteOutputToDemoDirectory() )
	{
		// Open in the batch directory
		file_output.open( g_BatchDirectory + szOutputFile );
	}
	else
	{
		// Open in executable directory
		file_output.open( g_ProgramDirectory + szOutputFile );
	}

	if( !file_output.is_open() )
	{
		printf( "Failed to write results to file\n\n" );
		return false;
	}

	char szInfo[ 128 ];
	_snprintf_s( szInfo, sizeof(szInfo), sizeof(szInfo), "%s ", CSSFF_NAME );
	char szTime[ 128 ];
	strftime( szTime, sizeof(szTime), "batch output at %Y.%m.%d %H:%M:%S\n\n\n", &timeinfo );
	strcat_s( szInfo, sizeof(szInfo), szTime );
	file_output.write( szInfo, strlen(szInfo) );

	bool wrote_errors_or_warnings = false;

	if( g_FailedDemos.size() )
	{
		file_output.write( "DEMOS WITH PARSING ERRORS:\n", 27 );

		for( size_t i = 0; i < g_FailedDemos.size(); ++i )
		{
			char szFailBuffer[300];
			_snprintf_s( szFailBuffer, sizeof(szFailBuffer), sizeof(szFailBuffer), "%d. %s", i+1, g_FailedDemos[i].c_str() );
			file_output.write( szFailBuffer, strlen(szFailBuffer) );
		}

		wrote_errors_or_warnings = true;
	}

	if( g_WarningDemos.size() )
	{
		if( wrote_errors_or_warnings )
			file_output.write( "\n", 1 );

		file_output.write( "DEMOS WITH WARNINGS:\n", 21 );

		for( size_t i = 0; i < g_WarningDemos.size(); ++i )
		{
			std::string warning;
			g_WarningDemos[i].GetString( warning );

			char szFailBuffer[300];
			_snprintf_s( szFailBuffer, sizeof(szFailBuffer), sizeof(szFailBuffer), "%d. %s", i+1, warning.c_str() );
			file_output.write( szFailBuffer, strlen(szFailBuffer) );
		}

		wrote_errors_or_warnings = true;
	}

	if( wrote_errors_or_warnings )
		file_output.write( "\n\nFOUND FRAGS:\n\n", 16 );

	if( g_BatchOutput.length() )
		file_output.write( g_BatchOutput.c_str(), g_BatchOutput.length() );
	else
		file_output.write( "No frags found\n\n", 16 );

	file_output.write( "Batch end", 9 );

	if( bAborted )
		file_output.write( " (process aborted)", 18 );

	file_output.close();

	printf( "Results have been written to file %s in %s folder\n\n", szOutputFile, Settings()->WriteOutputToDemoDirectory()? "processed" : "program" );

	return true;
}

/**
* Prints the demo error in detail and adds it to the list of failed demos if need be
* @param demo				the demo containing the error
* @noreturn
*/
void HandleDemoError( const DemoFile &demo )
{
	assert( !demo.IsValidDemo() );

	std::string error;

	demoheader_t *hdr = (demoheader_t *)demo.GetBuffer();

	// Format the error message
	switch( demo.GetError() )
	{
		case COULD_NOT_OPEN_FILE:
		default:
		{
			error = std::format( "failed to open file \"{}\"", demo.GetFileName() );
			break;
		}
		case FILE_TOO_SMALL:
		{
			error = "file too small";
			break;
		}
		case INVALID_HDR_ID:
		{
			error = "invalid demo header ID";
			break;
		}
		case INVALID_DEM_PROTOCOL:
		{
			error = std::format( "demo protocol {} is invalid - expected {}", hdr->demoprotocol, DEMO_PROTOCOL );
			break;
		}
		case INVALID_GAMEDIR:
		{
			error = std::format( "game directory \"{}\" is invalid - expected \"{}\"", hdr->gamedirectory, CSS_GAMEDIR );
			break;
		}
		case INVALID_NET_PROTOCOL:
		{
			if( hdr->networkprotocol >= NETWORK_PROTOCOL_NEW_MIN && hdr->networkprotocol <= NETWORK_PROTOCOL_NEW_MAX )
			{
				error = std::format( "CS:S v77{} demo - currently unsupported", hdr->networkprotocol == NETWORK_PROTOCOL_NEW_MAX ? " or Steam CS:S" : "" );
			}
			else
			{
				error = std::format( "network protocol {} is invalid - expected {}", hdr->networkprotocol, NETWORK_PROTOCOL_V34 );
			}

			break;
		}
	}

	// Print the error and add it to the failed demos list if need be
	if( Settings()->BatchProcessingEnabled() )
	{
		printf( "Failed to parse (%s)\n", error.c_str() );

		std::string tempError = demo.GetFileName() + " (" + error + ")\n";
		g_FailedDemos.emplace_back( tempError );
	}
	else
	{
		printf( "%s: Failed to parse file %s (%s)\n\n", CSSFF_NAME, demo.GetFileName().c_str(), error.c_str() );
	}
}

// ========================================================================================================================================================

int main( int argc, char *argv[] )
{
	SetConsoleTitle( TEXT(CSSFF_NAME) );

#ifdef _DEBUG
	system( "pause" );
#endif

	const char *szSettingsArg = nullptr;
	const char *szBatchDirArg = nullptr;
	bool bUnrecognizedArgs = false;

	// Process the arguments
	for( int nArg = 1; nArg < argc; ++nArg )
	{
		const char *szArg = argv[ nArg ];

		if( FileHasExtension( szArg, "dem" ) )
			s_DemosToParse.emplace_back( szArg );
		else if( FileHasExtension( szArg, "ini" ) )
			szSettingsArg = szArg;
		else if( IsValidDirectory( szArg ) )
			szBatchDirArg = szArg;
		else
			bUnrecognizedArgs = true;
	}

	// Get program executable directory (suffixed with '\')
	g_ProgramDirectory = argv[0];
	size_t pos = g_ProgramDirectory.find_last_of( '\\' );
	if( pos == std::string::npos )
	{
		printf( "%s: Invalid executable directory", CSSFF_NAME );
		return 1;
	}
	g_ProgramDirectory = g_ProgramDirectory.substr( 0, pos+1 );

	// Load program settings
	Settings()->LoadSettings( szSettingsArg );

	// Check if we should search for demos from a different folder
	if( szBatchDirArg )
	{
		g_BatchDirectory = szBatchDirArg;
		g_BatchDirectory += "\\";
	}
	else
	{
		if( !s_DemosToParse.empty() )
		{
			// If there were demo args, then batch directory is the working directory of the program
			char szWorkingDir[ MAX_PATH ];
			_getcwd( szWorkingDir, sizeof(szWorkingDir) );

			g_BatchDirectory = szWorkingDir;
			g_BatchDirectory += "\\";
		}
		else
		{
			g_BatchDirectory = g_ProgramDirectory;
		}
	}

	// Should we batch process the folder?
	if( s_DemosToParse.empty() )
	{
		if( Settings()->BatchProcessingEnabled() && !bUnrecognizedArgs )
		{
			if( !FindDemosInFolder( g_BatchDirectory ) || s_DemosToParse.size() == 0 )
			{
				printf( "%s: No demos to parse found in the current folder!\n", CSSFF_NAME );
				system( "pause" );
				return 0;
			}
		}
		else
		{
			printf( "%s: No demo - drag and drop a demo onto the program to parse it!\n", CSSFF_NAME );
			system( "pause" );
			return 0;
		}
	}

	const int nDemosToParse = s_DemosToParse.size();
	int nParsedDemos = 0;

	// Disable batch processing if we don't have multiple demos to parse
	if( Settings()->BatchProcessingEnabled() && nDemosToParse <= 1 )
		Settings()->DisableBatchProcessing();

	if( Settings()->BatchProcessingEnabled() )
	{
		g_BatchOutput.reserve( 512 );
		printf( "%s: Batch processing %d demos...\n", CSSFF_NAME, nDemosToParse );
		printf( "Press 'Q' to abort the process\n\n" );
	}

	// ===== The main parsing loop ========================================
	bool bAborted = false;
	std::time_t start_time = std::time( nullptr );

	for( int nDemo = 0; nDemo < nDemosToParse; ++nDemo )
	{
		if( Settings()->BatchProcessingEnabled() )
		{
			printf( "Demo %d/%d: ", nDemo+1, nDemosToParse );
		}

		const char *szCurrentDemo = s_DemosToParse[ nDemo ].c_str();

		try // Try parsing the demo
		{
			DemoFile demo( szCurrentDemo );

			if( !demo.IsValidDemo() )
			{
				HandleDemoError( demo );
				continue;
			}

			DemoParser parser( &demo );

			bAborted = !parser.Parse();

			if( bAborted )
				break;

			++nParsedDemos;
		}
		catch( ParsingError_t error )
		{
			// Ignore errors at the end of a demo, since they often happen on map change etc.
			if( error.at_end_of_demo )
				continue;

			if( Settings()->BatchProcessingEnabled() )
			{
				printf( " L Error encountered on tick %d - parsing aborted (%s)\n", error.tick, error.error_msg );
				char szTempError[ MAX_PATH ];
				std::string strFilename = szCurrentDemo;
				RemoveFileNameFolders( strFilename );
				_snprintf_s( szTempError, sizeof(szTempError), sizeof(szTempError), "%s: %s on tick %d\n", strFilename.c_str(), error.error_msg, error.tick );
				g_FailedDemos.emplace_back( szTempError );
			}
			else
			{
				printf( "Error encountered on tick %d - parsing aborted (%s)\n\n", error.tick, error.error_msg );
			}
		}
	}

	// Print elapsed time
	std::time_t end_time = std::time( nullptr );
	double elapsed_seconds = (double)(end_time - start_time);
	int elapsed_minutes = ((int)elapsed_seconds) / 60;
	int remaining_seconds = ((int)elapsed_seconds) % 60;

	printf( "\nElapsed time: ");
	if( elapsed_minutes )
		printf( "%d minutes ", elapsed_minutes );
	printf( "%d seconds\n\n", remaining_seconds );

	// Write batch output
	if( Settings()->BatchProcessingEnabled() )
	{
		if( !bAborted )
			printf( "\nProcessing finished - %d/%d demos successfully parsed\n", nParsedDemos, nDemosToParse );
		else
			printf( "\nProcess aborted - %d/%d demos successfully parsed\n", nParsedDemos, nDemosToParse );

		WriteBatchOutput( bAborted );
	}

	system( "pause" );

	return 0;
}

// ========================================================================================================================================================