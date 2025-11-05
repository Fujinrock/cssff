#pragma once

#include <string>
#include "vector"

extern const char *g_szWarnings[];

// A predefined list of all possible warnings, because there can be multiple of these per file
enum WarningType
{
	BYTE_MISMATCH = 0,
	POV_PLAYER_NOT_FOUND,
	VICTIM_TEAM_NOT_FOUND,
	INVALID_STRING_LENGTH,
	INVALID_ENTITY_UPDATE_TYPE,
	SPEC_ATTACKER_TEAM_NOT_FOUND
};

struct ParsingError_t
{
	ParsingError_t( const char *msg );

	const char *error_msg;
	int tick;
	bool at_end_of_demo;
};

struct ParsingWarning_t
{
	ParsingWarning_t( const std::string &_demoname, WarningType _type );

	void GetString( std::string &buffer );

	std::string demoname;
	WarningType type;
	int count;
	int tick;
};

void AddWarning( const std::string &demoname, WarningType type );