#pragma once

#include <vector>
#include "Common.h"

struct GameEvent;

typedef std::vector< GameEvent > GameEventVector;

/**
 * Event data field value types
 */
enum GameEventValueType
{
	VAL_NONE	= 0,
	VAL_STRING	= 1,
	VAL_FLOAT	= 2,
	VAL_LONG	= 3,
	VAL_SHORT	= 4,
	VAL_BYTE	= 5,
	VAL_BOOL	= 6
};

/**
 * Holds information about the structure of a game event (how to parse it)
 */
struct GameEvent
{
	GameEvent( int id, const char *name );

	int event_id;
	char event_name[32];

	/**
	 * A value field contained within an event
	 */
	struct GameEventField
	{
		GameEventField( const char *name, GameEventValueType type );

		char field_name[32];
		GameEventValueType field_type;
	};

	void AddField( const char *name, GameEventValueType type );
	const GameEventField &GetField( uint32 index ) const;
	size_t GetFieldCount( void ) const;

private:
	std::vector< GameEventField > event_fields;
};