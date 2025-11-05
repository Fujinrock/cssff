#pragma once
#include "Entities.h"
#include "bitbuf.h"

static inline int Int_Decode( bf_read &reader, const SendProp *pSendProp )
{
	int flags = pSendProp->m_flags;

	if ( flags & SPROP_UNSIGNED )
	{
		return reader.ReadUBitLong( pSendProp->m_nBits );
	}
	else
	{
		return reader.ReadSBitLong( pSendProp->m_nBits );
	}
}

// Look for special flags like SPROP_COORD, SPROP_NOSCALE, and SPROP_NORMAL and
// decode if they're there. Fills in fVal and returns true if it decodes anything.
static inline bool DecodeSpecialFloat( bf_read &reader, const SendProp *pSendProp, float &fVal )
{
	int flags = pSendProp->m_flags;

	if ( flags & SPROP_COORD )
	{
		fVal = reader.ReadBitCoord();
		return true;
	}
	else if ( flags & SPROP_NOSCALE )
	{
		fVal = reader.ReadBitFloat();
		return true;
	}
	else if ( flags & SPROP_NORMAL )
	{
		fVal = reader.ReadBitNormal();
		return true;
	}

	return false;
}

static inline float Float_Decode( bf_read &reader, const SendProp *pSendProp )
{
	float fVal = 0.0f;
	unsigned long dwInterp;

	// Check for special flags..
	if( DecodeSpecialFloat( reader, pSendProp, fVal ) )
	{
		return fVal;
	}

	dwInterp = reader.ReadUBitLong( pSendProp->m_nBits );
	fVal = ( float )dwInterp / ( ( 1 << pSendProp->m_nBits ) - 1 );
	fVal = pSendProp->m_fLowValue + (pSendProp->m_fHighValue - pSendProp->m_fLowValue) * fVal;
	return fVal;
}

static inline void Vector_Decode( bf_read &reader, const SendProp *pSendProp, Vector &v )
{
	v.x = Float_Decode( reader, pSendProp );
	v.y = Float_Decode( reader, pSendProp );

	// Don't read in the third component for normals
	if ( ( pSendProp->m_flags & SPROP_NORMAL ) == 0 )
	{
		v.z = Float_Decode( reader, pSendProp );
	}
	else
	{
		int signbit = reader.ReadOneBit();

		float v0v0v1v1 = v.x * v.x + v.y * v.y;
		if (v0v0v1v1 < 1.0f)
		{
			v.z = sqrtf( 1.0f - v0v0v1v1 );
		}
		else
		{
			v.z = 0.0f;
		}

		if (signbit)
		{
			v.z *= -1.0f;
		}
	}
}

#define DT_MAX_STRING_BITS			9
#define DT_MAX_STRING_BUFFERSIZE	(1<<DT_MAX_STRING_BITS)	// Maximum length of a string that can be sent.

// Forward declaration
Prop_t *DecodeProp( bf_read &reader, FlattenedPropEntry *pFlattenedProp, uint32 uClass, int nFieldIndex );

static inline Prop_t *Array_Decode( bf_read &reader, FlattenedPropEntry *pFlattenedProp, int nNumElements, uint32 uClass, int nFieldIndex )
{
	int maxElements = nNumElements;
	int numBits = 1;
	while ( (maxElements >>= 1) != 0 )
	{
		numBits++;
	}

	int nElements = reader.ReadUBitLong( numBits );

	Prop_t *pResult = nullptr;
	pResult = new Prop_t[ nElements ];

	for ( int i = 0; i < nElements; i++ )
	{
		FlattenedPropEntry temp( pFlattenedProp->m_arrayElementProp, nullptr );
		Prop_t *pElementResult = DecodeProp( reader, &temp, uClass, nFieldIndex );
		pResult[ i ] = *pElementResult;
		delete pElementResult;
		pResult[ i ].m_nNumElements = nElements - i;
	}

	return pResult;
}

static inline const char *String_Decode( bf_read &reader, const SendProp *pSendProp )
{
	// Read it in.
	int len = reader.ReadUBitLong( DT_MAX_STRING_BITS );

	char *tempStr = new char[ len + 1 ];

	if ( len >= DT_MAX_STRING_BUFFERSIZE )
	{
		len = DT_MAX_STRING_BUFFERSIZE - 1;
	}

	reader.ReadBits( tempStr, len*8 );
	tempStr[len] = 0;

	return tempStr;
}

inline Prop_t *DecodeProp( bf_read &reader, FlattenedPropEntry *pFlattenedProp, uint32 uClass, int nFieldIndex )
{
	const SendProp *pSendProp = pFlattenedProp->m_prop;

	Prop_t *pResult = nullptr;
	if ( pSendProp->m_propType != DPT_Array && pSendProp->m_propType != DPT_DataTable )
	{
		pResult = new Prop_t( ( SendPropType )( pSendProp->m_propType ) );
	}

	switch ( pSendProp->m_propType )
	{
		case DPT_Int:
			pResult->m_value.m_int = Int_Decode( reader, pSendProp );
			break;
		case DPT_Float:
			pResult->m_value.m_float = Float_Decode( reader, pSendProp );
			break;
		case DPT_Vector:
			Vector_Decode( reader, pSendProp, pResult->m_value.m_vector );
			break;
		case DPT_String:
			pResult->m_value.m_pString = String_Decode( reader, pSendProp );
			break;
		case DPT_Array:
			pResult = Array_Decode( reader, pFlattenedProp, pSendProp->m_nNumElements, uClass, nFieldIndex );
			break;
		case DPT_DataTable:
			break;
	}

	return pResult;
}