#pragma once

#include "Common.h"
#include <vector>

struct EntityEntry;

typedef std::vector< EntityEntry * > EntityVector;

// How many bits to use to encode an edict
#define	MAX_EDICT_BITS				11 // # of bits needed to represent max edicts
// Max # of edicts in a level
#define	MAX_EDICTS					( 1 << MAX_EDICT_BITS )

#define NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS	10

// SendProp::m_Flags.
#define SPROP_UNSIGNED			(1<<0)	// Unsigned integer data.

#define SPROP_COORD				(1<<1)	// If this is set, the float/vector is treated like a world coordinate.
										// Note that the bit count is ignored in this case.

#define SPROP_NOSCALE			(1<<2)	// For floating point, don't scale into range, just take value as is.

#define SPROP_ROUNDDOWN			(1<<3)	// For floating point, limit high value to range minus one bit unit

#define SPROP_ROUNDUP			(1<<4)	// For floating point, limit low value to range minus one bit unit

#define SPROP_NORMAL			(1<<5)	// If this is set, the vector is treated like a normal (only valid for vectors)

#define SPROP_EXCLUDE			(1<<6)	// This is an exclude prop (not excludED, but it points at another prop to be excluded).

#define SPROP_XYZE				(1<<7)	// Use XYZ/Exponent encoding for vectors.

#define SPROP_INSIDEARRAY		(1<<8)	// This tells us that the property is inside an array, so it shouldn't be put into the
										// flattened property list. Its array will point at it when it needs to.

#define SPROP_PROXY_ALWAYS_YES	(1<<9)	// Set for datatable props using one of the default datatable proxies like
										// SendProxy_DataTableToDataTable that always send the data to all clients.

#define SPROP_CHANGES_OFTEN		(1<<10)	// this is an often changed field, moved to head of sendtable so it gets a small index

#define SPROP_IS_A_VECTOR_ELEM	(1<<11)	// Set automatically if SPROP_VECTORELEM is used.

#define SPROP_COLLAPSIBLE		(1<<12)	// Set automatically if it's a datatable with an offset of 0 that doesn't change the pointer
										// (ie: for all automatically-chained base classes).
										// In this case, it can get rid of this SendPropDataTable altogether and spare the
										// trouble of walking the hierarchy more than necessary.

// =====================================================================================================================================================================

enum SendPropType
{
	DPT_Int = 0,
	DPT_Float,
	DPT_Vector,
	DPT_String,
	DPT_Array,					///< An array of the base types (can't be of datatables)
	DPT_DataTable,

	DPT_NUMSendPropTypes
};

// =====================================================================================================================================================================

class SendProp
{
public:
	char m_propName[256];
	SendPropType m_propType;
	int m_flags;
	float m_fLowValue;
	float m_fHighValue;
	int m_nBits;
	int m_nNumElements;			///< Used if this is an array prop
	char m_dtname[ 256 ];

	friend class SendTable;
};

// =====================================================================================================================================================================

struct FlattenedPropEntry
{
	FlattenedPropEntry( const SendProp *prop, const SendProp *arrayElementProp )
		: m_prop( prop )
		, m_arrayElementProp( arrayElementProp )
	{
	}

	const SendProp *m_prop;
	const SendProp *m_arrayElementProp;
};

// =====================================================================================================================================================================

struct ExcludeEntry
{
	ExcludeEntry( const char *pVarName, const char *pDTName, const char *pDTExcluding )
		: m_pVarName( pVarName )
		, m_pDTName( pDTName )
		, m_pDTExcluding( pDTExcluding )
	{
	}

	const char *m_pVarName;
	const char *m_pDTName;
	const char *m_pDTExcluding;
};

// =====================================================================================================================================================================

struct Prop_t
{
	Prop_t()
	{
	}

	Prop_t( SendPropType type )
		: m_type( type )
		, m_nNumElements( 0 )
	{
		// This makes all possible types init to 0's
		m_value.m_vector.Init();
	}

	~Prop_t()
	{
		if( m_type == DPT_String )
			delete[] m_value.m_pString;
	}

	SendPropType m_type;
	union value
	{
		value() { m_vector.Init(); };
		int m_int;
		float m_float;
		const char *m_pString;
		int64 m_int64;
		Vector m_vector;
	}
	m_value;
	int m_nNumElements;
};

// =====================================================================================================================================================================

struct PropEntry
{
	PropEntry( FlattenedPropEntry *pFlattenedProp, Prop_t *pPropValue )
		: m_pFlattenedProp( pFlattenedProp )
		, m_pPropValue( pPropValue )
	{
	}

	~PropEntry()
	{
		if( m_pFlattenedProp->m_prop->m_propType == DPT_Array )
			delete[] m_pPropValue;
		else
			delete m_pPropValue;
	}

	FlattenedPropEntry *m_pFlattenedProp;
	Prop_t *m_pPropValue;
};

// =====================================================================================================================================================================

struct EntityEntry
{
	EntityEntry( int nEntity, uint32 uClass, uint32 uSerialNum)
		: m_nEntity( nEntity )
		, m_uClass( uClass )
		, m_uSerialNum( uSerialNum )
	{
	}

	~EntityEntry()
	{
		for ( std::vector< PropEntry * >::iterator i = m_props.begin(); i != m_props.end(); ++i )
		{
			delete *i;
		}
	}

	PropEntry *FindProp( const char *pName )
	{
		for ( std::vector< PropEntry * >::iterator i = m_props.begin(); i != m_props.end(); ++i )
		{
			PropEntry *pProp = *i;
			if ( strcmp( pProp->m_pFlattenedProp->m_prop->m_propName, pName ) == 0 )
			{
				return pProp;
			}
		}
		return nullptr;
	}

	void AddOrUpdateProp( FlattenedPropEntry *pFlattenedProp, Prop_t *pPropValue )
	{
		PropEntry *pProp = FindProp( pFlattenedProp->m_prop->m_propName );
		if ( pProp )
		{
			if( pFlattenedProp->m_prop->m_propType == DPT_Array )
				delete[] pProp->m_pPropValue;
			else
				delete pProp->m_pPropValue;

			pProp->m_pPropValue = pPropValue;
		}
		else
		{
			pProp = new PropEntry( pFlattenedProp, pPropValue );
			m_props.push_back( pProp );
		}
	}

	int m_nEntity;
	uint32 m_uClass;
	uint32 m_uSerialNum;

	std::vector< PropEntry * > m_props;
};

// =====================================================================================================================================================================