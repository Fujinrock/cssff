#pragma once

// Comments from common/protocol.h

enum NetMessages
{
	NET_NOP = 0,					///< NOP command used for padding
	NET_Disconnect = 1,				///< Disconnect, last message in connection
	NET_File = 2,					///< File transmission message request/deny
	NET_Tick = 3,					///< Send last world tick
	NET_StringCmd = 4,				///< A string command
	NET_SetConVar = 5,				///< Sends one/multiple convar settings
	NET_SignOnState = 6				///< Signals current signon state
};

enum SvcMessages
{
	SVC_Print = 7,					///< Print text to console
	SVC_ServerInfo = 8,				///< First message from server about game, map etc
	SVC_SendTable = 9,				///< Sends a sendtable description for a game class
	SVC_ClassInfo = 10,				///< Info about classes (first byte is a CLASSINFO_ define).
	SVC_SetPause = 11,				///< Tells client if server paused or unpaused
	SVC_CreateStringTable = 12,		///< Inits shared string tables
	SVC_UpdateStringTable = 13,		///< Updates a string table
	SVC_VoiceInit = 14,				///< Inits used voice codecs & quality
	SVC_VoiceData = 15,				///< Voicestream data from the server
									//   HLTV control messages (16) is skipped
	SVC_Sounds = 17,				///< Starts playing sound
	SVC_SetView = 18,				///< Sets entity as point of view
	SVC_FixAngle = 19,				///< Sets/corrects players viewangle
	SVC_CrosshairAngle = 20,		///< Adjusts crosshair in auto aim mode to lock on traget
	SVC_BSPDecal = 21,				///< Add a static decal to the worl BSP
									//   Modification to the terrain/displacement (22) is skipped
	SVC_UserMessage = 23,			///< A game specific message
	SVC_EntityMessage = 24,			///< A message for an entity
	SVC_GameEvent = 25,				///< Global game event fired
	SVC_PacketEntities = 26,		///< Non-delta compressed entities
	SVC_TempEntities = 27,			///< Non-reliable event object
	SVC_Prefetch = 28,				///< Only sound indices for now
	SVC_Menu = 29,					///< Display a menu from a plugin
	SVC_GameEventList = 30,			///< List of known games events and fields
	SVC_GetCvarValue = 31,			///< Server wants to know the value of a cvar on the client.
	NumMessageTypes
};