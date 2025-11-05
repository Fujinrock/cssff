#include "Entities.h"
#include "PropDecode.h"
#include "DemoParser.h"
#include "Errors.h"

// =====================================================================================================================================================================
/**
 * Used to classify entity update types in ProcessPacketEntities
 */
enum UpdateType
{
	EnterPVS = 0,	// Entity came back into PVS, create new entity if one doesn't exist

	LeavePVS,		// Entity left PVS

	DeltaEnt,		// There is a delta for this entity
	PreserveEnt,	// Entity stays alive but no delta ( could be LOD, or just unchanged )

	Finished,		// Finished parsing entities successfully
	Failed,			// Parsing error occured while reading entities
};

// =====================================================================================================================================================================
/**
 * Flags for delta encoding header
 */
enum
{
	FHDR_ZERO			= 0x0000,
	FHDR_LEAVEPVS		= 0x0001,
	FHDR_DELETE			= 0x0002,
	FHDR_ENTERPVS		= 0x0004,
};

// =====================================================================================================================================================================

enum
{
	ENTITY_SENTINEL = 9999	///< Larger number than any real entity number
};

// =====================================================================================================================================================================

EntityEntry *DemoParser::FindEntity( int nEntity )
{
	for ( auto i = m_Entities.begin(); i != m_Entities.end(); ++i )
	{
		if (  (*i)->m_nEntity == nEntity )
		{
			return *i;
		}
	}

	return nullptr;
}

// =====================================================================================================================================================================

EntityEntry *DemoParser::AddEntity( int nEntity, uint32 uClass, uint32 uSerialNum )
{
	// If entity already exists, then replace it, else add it
	EntityEntry *pEntity = FindEntity( nEntity );
	if ( pEntity )
	{
		pEntity->m_uClass = uClass;
		pEntity->m_uSerialNum = uSerialNum;
	}
	else
	{
		pEntity = new EntityEntry( nEntity, uClass, uSerialNum );
		m_Entities.push_back( pEntity );
	}

	return pEntity;
}

// =====================================================================================================================================================================

void DemoParser::RemoveEntity( int nEntity )
{
	for ( auto i = m_Entities.begin(); i != m_Entities.end(); ++i )
	{
		EntityEntry *pEntity = *i;
		if (  pEntity->m_nEntity == nEntity )
		{
			m_Entities.erase( i );
			delete pEntity;
			break;
		}
	}
}

// =====================================================================================================================================================================

bool DemoParser::ProcessPacketEntities( bf_read &reader, int maxentries, int updatedentries, int datalength, bool isdelta, int deltafrom, int baseline, bool updatebaseline )
{
	bool bAsDelta = isdelta;
	int nHeaderCount = updatedentries;
	int nBaseline = baseline;
	bool bUpdateBaselines = updatebaseline;
	int nHeaderBase = -1;
	int nNewEntity = -1;
	int UpdateFlags = 0;

	UpdateType updateType = PreserveEnt;

	while ( updateType < Finished )
	{
		--nHeaderCount;

		bool bIsEntity = ( nHeaderCount >= 0 ) ? true : false;

		if( bIsEntity )
		{
			UpdateFlags = FHDR_ZERO;

			nNewEntity = nHeaderBase + 1 + reader.ReadUBitVar();
			nHeaderBase = nNewEntity;

			// Since the entity updates come in order starting from lowest ent index,
			// we can just stop processing entities after all the players have been updated
			// (this can speed up demo parsing by over 20x)
			if( nNewEntity > m_iMaxClients )
				return true;

			// Leave PVS flag
			if ( reader.ReadOneBit() == 0 )
			{
				// Enter PVS flag
				if ( reader.ReadOneBit() != 0 )
				{
					UpdateFlags |= FHDR_ENTERPVS;
				}
			}
			else
			{
				UpdateFlags |= FHDR_LEAVEPVS;

				// Force delete flag
				if ( reader.ReadOneBit() != 0 )
				{
					UpdateFlags |= FHDR_DELETE;
				}
			}
		}

		for ( updateType = PreserveEnt; updateType == PreserveEnt; )
		{
			// Figure out what kind of an update this is
			if ( !bIsEntity || nNewEntity > ENTITY_SENTINEL)
			{
				updateType = Finished;
			}
			else
			{
				if ( UpdateFlags & FHDR_ENTERPVS )
				{
					updateType = EnterPVS;
				}
				else if ( UpdateFlags & FHDR_LEAVEPVS )
				{
					updateType = LeavePVS;
				}
				else
				{
					updateType = DeltaEnt;
				}
			}

			switch( updateType )
			{
				case EnterPVS:	
				{
					uint32 uClass = reader.ReadUBitLong( m_iServerClassBits );
					uint32 uSerialNum = reader.ReadUBitLong( NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS );

					EntityEntry *pEntity = AddEntity( nNewEntity, uClass, uSerialNum );
					if ( !ReadNewEntity( reader, pEntity ) )
					{
						throw ParsingError_t( "error reading entity in enter PVS" );
						return false;
					}
				}
				break;

				case LeavePVS:
				{
					if ( !bAsDelta )  // Should never happen on a full update
					{
						updateType = Failed;
						throw ParsingError_t( "leave PVS on full update" );
					}
					else if ( UpdateFlags & FHDR_DELETE )
					{
						RemoveEntity( nNewEntity );
					}
				}
				break;

				case DeltaEnt:
				{
					EntityEntry *pEntity = FindEntity( nNewEntity );

					if ( pEntity )
					{
						if ( !ReadNewEntity( reader, pEntity ) )
						{
							throw ParsingError_t( "error reading entity in delta entity" );
							return false;
						}
					}
					else
					{
						throw ParsingError_t( "entity not found on delta entity" );
					}
				}
				break;

				case PreserveEnt:
				{
					if ( !bAsDelta )  // Should never happen on a full update
					{
						updateType = Failed;
						throw ParsingError_t( "preserve entity on full update" );
					}
					else if ( nNewEntity >= MAX_EDICTS )
					{
						throw ParsingError_t( "new entity >= MAX_EDICTS in preserve entity" );
					}
				}
				break;

				default:
					if( updateType != Finished )
						AddWarning( m_pDemo->GetFileName(), INVALID_ENTITY_UPDATE_TYPE);
					break;
			}
		}
	}

	return true;
}

// =====================================================================================================================================================================

bool DemoParser::ReadNewEntity( bf_read &reader, EntityEntry *pEntity )
{
	int index = -1;

	SendTable *pTable = GetTableByClassID( pEntity->m_uClass );

	while( reader.ReadOneBit() )
	{
		index += reader.ReadUBitVar() + 1;

		FlattenedPropEntry *pSendProp = GetSendPropByIndex( pEntity->m_uClass, index );
		if ( pSendProp )
		{
			Prop_t *pProp = DecodeProp( reader, pSendProp, pEntity->m_uClass, index );
			pEntity->AddOrUpdateProp( pSendProp, pProp );

			// TODO: save the index for these props when server classes are added for faster checking
			if( !strcmp( pSendProp->m_prop->m_propName, "m_angEyeAngles[0]" ) ) // Pitch
			{
				Player *pPlayer = FindPlayerByEntityIndex( pEntity->m_nEntity );

				assert( pPlayer );

				pPlayer->AddPitchAngle( pProp->m_value.m_float );
			}
			else if( !strcmp( pSendProp->m_prop->m_propName, "m_angEyeAngles[1]" ) ) // Yaw
			{
				Player *pPlayer = FindPlayerByEntityIndex( pEntity->m_nEntity );

				assert( pPlayer );

				pPlayer->AddYawAngle( pProp->m_value.m_float );
			}
			else if( !strcmp( pSendProp->m_prop->m_propName, "m_flFlashDuration" ) )
			{
				Player *pPlayer = FindPlayerByEntityIndex( pEntity->m_nEntity );

				assert( pPlayer );

				pPlayer->flashinfo.tick = m_iCurrentTick;
				pPlayer->flashinfo.time = pProp->m_value.m_float;
			}
			else if( !strcmp( pSendProp->m_prop->m_propName, "m_fFlags" ) )
			{
				Player *pPlayer = FindPlayerByEntityIndex( pEntity->m_nEntity );

				assert( pPlayer );

				if( !(pProp->m_value.m_int & FL_ONGROUND) )
				{
					if( pPlayer->airstatus == PL_ON_GROUND )
						pPlayer->airstatus = PL_IN_AIR_STARTED;
				}
				else
				{
					pPlayer->airstatus = PL_ON_GROUND;
				}
			}
			else if( !strcmp( pSendProp->m_prop->m_propName, "m_vecOrigin" ) )
			{
				Player *pPlayer = FindPlayerByEntityIndex( pEntity->m_nEntity );

				assert( pPlayer );

				if( pPlayer->airstatus == PL_IN_AIR_STARTED
					&& pProp->m_value.m_vector.z > (pPlayer->lastZ + (m_bIsPOV ? 2.5f : 7.5f) ))
					pPlayer->airstatus = PL_WENT_UP_IN_AIR;

				pPlayer->lastZ = pProp->m_value.m_vector.z;
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}

// =====================================================================================================================================================================

FlattenedPropEntry *DemoParser::GetSendPropByIndex( uint32 uClass, uint32 uIndex )
{
	if ( uIndex < m_ServerClasses[ uClass ].flattenedProps.size() )
	{
		return &m_ServerClasses[ uClass ].flattenedProps[ uIndex ];
	}
	return nullptr;
}

// =====================================================================================================================================================================

#define NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS	10
#define NUM_NETWORKED_EHANDLE_BITS					(MAX_EDICT_BITS + NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS)
#define INVALID_NETWORKED_EHANDLE_VALUE				((1 << NUM_NETWORKED_EHANDLE_BITS) - 1)
#define INVALID_EHANDLE_INDEX	0xFFFFFFFF

int DemoParser::GetEntIndexFromEHandleInt( int nEHandleInt )
{
	if( nEHandleInt == INVALID_NETWORKED_EHANDLE_VALUE )
	{
		return INVALID_EHANDLE_INDEX;
	}
	else
	{
		int iEntity = nEHandleInt & ((1 << MAX_EDICT_BITS) - 1);
		return iEntity;
	}
}

// =====================================================================================================================================================================