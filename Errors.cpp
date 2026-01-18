#include "Errors.h"
#include "DemoParser.h"
#include "Common.h"
#include "Settings.h"

std::vector< ParsingWarning_t > g_WarningDemos;		// Filenames and the warning numbers of demos where a warning was triggered

// Textual representation of the warnings
// These match the WarningType enum
const char *g_szWarnings[] = { 
	"read bytes don't match packet data length",
	"POV player not found by death event",
	"victim team not found on death event",
	"invalid string length while decoding string",
	"invalid entity update type",
	"spectated attacker team not found on death event" };

ParsingError_t::ParsingError_t( const char *msg )
	: error_msg( msg ), tick( GetCurrentTick() )
{
	if( GetTotalTickCount() > 0 )
	{
		constexpr int maxTicksLeft = 100;
		at_end_of_demo = GetTotalTickCount() - tick <= maxTicksLeft;
	}
	else
	{
		constexpr int maxBytesLeft = 1;
		at_end_of_demo = GetNumBytesLeft() <= maxBytesLeft;
	}

	if( !Settings()->BatchProcessingEnabled() )
	{
		if( at_end_of_demo )
			printf( "Done parsing!\n\n" );
		else
			printf( "Error encountered!\n\n" );
	}
	else
	{
		if( at_end_of_demo )
			printf( " Successfully parsed" );
		else
			printf( " Error encountered" );
	}

	OnParsingEnd();
}

ParsingWarning_t::ParsingWarning_t( const std::string &_demoname, WarningType _type )
	: demoname( _demoname ),
	type( _type ),
	count( 1 ),
	tick( GetCurrentTick() )
{

}

void ParsingWarning_t::GetString( std::string &buffer, bool prepend_demoname )
{
	buffer = "";

	if( prepend_demoname )
		buffer = demoname + ": ";

	buffer = buffer + g_szWarnings[type] + " on tick " + std::to_string((long long)tick);

	if( count > 1 )
		buffer += " (" + std::to_string( (long long)count ) + "x)";

	buffer += "\n";
}

void AddWarning( const std::string &demoname, WarningType type )
{
	for( size_t i = 0; i < g_WarningDemos.size(); ++i )
	{
		if( g_WarningDemos[ i ].type == type
		&& g_WarningDemos[ i ].demoname == demoname )
		{
			++g_WarningDemos[ i ].count;
			return;
		}
	}

	g_WarningDemos.emplace_back( demoname, type );
}