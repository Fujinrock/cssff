#pragma once

#include "GameEvents.h"
#include "DemoFile.h"
#include "Frag.h"
#include "Player.h"
#include "StringTables.h"
#include "DataTables.h"
#include "bitbuf.h"

/**
 * Parses the raw data of a demo file
 */
class DemoParser
{
public:
	DemoParser( DemoFile *pDemo );
	~DemoParser();

	bool Parse( void );								///< Parses the demo

	float GetTimeBetweenTicks( int, int ) const;	///< Get the time in seconds between two chosen ticks
	int GetTickCount( void ) const;					///< Get the # of ticks in the demo
	int GetCurrentTick( void ) const;				///< Get the tick currently being parsed
	int GetTickRate( void ) const;					///< Get the # of ticks per second
	void OnParsingEnd( void );						///< Called when demo is successfully parsed or a parsing error is thrown

private:
	// ===== Frags =================================================================================================
	void FindRoundFrags( void );					///< Finds all the frags from the current round (called at round start)

	FragVector m_Frags;								///< Holds all the frags found on the demo being parsed

	// ===== Net messages ==========================================================================================
	void HandleDemoPacket( bf_read &reader );
	void HandleNETDisconnect( bf_read &reader );
	void HandleNETFile( bf_read &reader );
	void HandleNETTick( bf_read &reader );
	void HandleNETStringCmd( bf_read &reader );
	void HandleNETSetConVar( bf_read &reader );
	void HandleNETSignOnState( bf_read &reader );
	void HandleSVCPrint( bf_read &reader );
	void HandleSVCServerInfo( bf_read &reader );
	void HandleSVCSendTable( bf_read &reader );
	void HandleSVCClassInfo( bf_read &reader );
	void HandleSVCSetPause( bf_read &reader );
	void HandleSVCCreateStringTable( bf_read &reader );
	void HandleSVCUpdateStringTable( bf_read &reader );
	void HandleSVCVoiceInit( bf_read &reader );
	void HandleSVCVoiceData( bf_read &reader );
	void HandleSVCSounds( bf_read &reader );
	void HandleSVCSetView( bf_read &reader );
	void HandleSVCFixAngle( bf_read &reader );
	void HandleSVCCrosshairAngle( bf_read &reader );
	void HandleSVCBSPDecal( bf_read &reader );
	void HandleSVCUserMessage( bf_read &reader );
	void HandleSVCEntityMessage( bf_read &reader );
	void HandleSVCGameEvent( bf_read &reader );
	void HandleSVCPacketEntities( bf_read &reader );
	void HandleSVCTempEntities( bf_read &reader );
	void HandleSVCPrefetch( bf_read &reader );
	void HandleSVCMenu( bf_read &reader );
	void HandleSVCGameEventList( bf_read &reader );
	void HandleSVCGetCvarValue( bf_read &reader );


	// ===== String tables =========================================================================================
	void CreateStringTable( const char *name, int max_entries, int user_data_size, int user_data_size_bits, bool user_data_fixed_size );
	void ParseStringTableUpdate( bf_read &reader, int entries, int max_entries, int user_data_size, int user_data_size_bits, bool user_data_fixed_size, bool is_user_info );
	const StringTableData_t *GetStringTableData( uint32 tableID );

	uint32				m_iNumStringTables;
	StringTableData_t	m_StringTables[ MAX_STRING_TABLES ];

	
	// ===== Data tables ===========================================================================================
	void ParseDataTables( bf_read &reader );
	SendTable *GetTableByName( const char *pName );
	SendTable *GetTableByClassID( uint32 iClassID );
	void GatherExcludes( SendTable *pTable );
	void GatherProps( SendTable *pTable, int nServerClass );
	void GatherProps_IterateProps( SendTable *pTable, int nServerClass, std::vector< FlattenedPropEntry > &flattenedProps );
	bool IsPropExcluded( SendTable *pTable, const SendProp &checkSendProp );
	void FlattenDataTable( int nServerClass );

	int					m_iServerClassBits;				///< # of bits used to encode server class IDs
	ServerClassVector	m_ServerClasses;
	SendTableVector		m_DataTables;
	ExcludeEntryVector	m_currentExcludes;


	// ===== Entities ==============================================================================================
	bool ProcessPacketEntities( bf_read &reader, int maxentries, int updatedentries, int datalength, bool isdelta, int deltafrom, int baseline, bool updatebaseline );
	bool ReadNewEntity( bf_read &reader, EntityEntry *pEntity );
	EntityEntry *FindEntity( int nEntity );
	EntityEntry *AddEntity( int nEntity, uint32 uClass, uint32 uSerialNum );
	void RemoveEntity( int nEntity );
	FlattenedPropEntry *GetSendPropByIndex( uint32 uClass, uint32 uIndex );
	int GetEntIndexFromEHandleInt( int nEHandleInt );

	EntityVector		m_Entities;						///< Entities read from the demo, but actually this just holds the player entities


	// ===== Players ===============================================================================================
	PlayerList			m_Players;						///< List of all currently existing players on the demo
	std::vector< int >	m_ExpiredUserIDs;				///< All the players by user ID that will be removed at the start of a new round (after checking for frags)
	PostCheckDataVector m_PlayerPostCheckData;			///< Players whose props to check after each packet has been handled

	Player *FindPlayerByEntityIndex( int entityIndex );
	Player *FindPlayerByUserId( int userId );
	void DoPlayersPostCheck( void );					///< Checks if kills were flickshots and if jumpshots had enough air time


	// ===== Game events ===========================================================================================
	GameEventVector		m_GameEvents;					///< Game events on this demo as described by GameEventList

	const GameEvent &GetGameEvent( uint32 id );			///< Get the event's structure by its ID
	void ParseGameEvent( bf_read &reader );				///< Read and add a new event to the list of events from SVC_GameEventList
	void HandleGameEvent( bf_read &reader );			///< Read a game event according to the event descriptors from SVC_GameEvent

	// Special game event handlers invoked by HandleGameEvent
	void HandlePlayerSpawnEvent( bf_read &reader, const GameEvent &event );
	void HandlePlayerDeathEvent( bf_read &reader, const GameEvent &event );
	void HandlePlayerDisconnectEvent( bf_read &reader, const GameEvent &event );
	void HandleRoundStartEvent( bf_read &reader, const GameEvent &event );


	// =============================================================================================================
	// Useful information read from the demo file
	demoheader_t		m_demoHeader;					///< General demo info

	float				m_fTickInterval;				///< # of seconds between ticks
	int					m_iTickRate;					///< # of ticks per second
	int					m_iCurrentTick;					///< The tick the parser is currently at

	bool				m_bUse5BitStringTableIndices;	///< Really old demos have 4 bit string table indices

	bool				m_bIsPOV;						///< Was the demo recorded by the client or the server
	int					m_iPOVPlayerSlot;				///< Used to find the user ID of the recording client
	int					m_iPOVPlayerUserID;				///< User ID of the recording client
	bool				m_bPOVPlayerIsDead;				///< Is the recording client dead

	int					m_iMaxClients;					///< Highest player entity index

	// For early termination (these messages can appear again on map change at the end of a POV demo or if something went wrong)
	bool				m_bServerInfoEncountered;		///< Whether SVC_ServerInfo was encountered while parsing the demo
	bool				m_bGameEventListEncountered;	///< Whether SVC_GameEventList was encountered while parsing the demo

	DemoFile *			m_pDemo;						///< The demo being parsed
};