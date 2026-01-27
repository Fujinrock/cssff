//====== Copyright (c) 2014, Valve Corporation, All rights reserved. ========//
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
// THE POSSIBILITY OF SUCH DAMAGE.
//===========================================================================//

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// NOTE: bf_read is guaranteed to return zeros if it overflows.

#ifndef BITBUF_H
#define BITBUF_H

#ifdef _WIN32
#pragma once
#endif

#include <assert.h>

//-----------------------------------------------------------------------------
// Forward declarations.
//-----------------------------------------------------------------------------

struct Vector;
struct QAngle;


//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------

inline int BitByte( int bits )
{
	// return PAD_NUMBER( bits, 8 ) >> 3;
	return (bits + 7) >> 3;
}


//-----------------------------------------------------------------------------
// Used for unserialization
//-----------------------------------------------------------------------------

class bf_read
{
public:
					bf_read();

					// nMaxBits can be used as the number of bits in the buffer. 
					// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
					bf_read( const void *pData, int nBytes, int nBits = -1 );
					bf_read( const char *pDebugName, const void *pData, int nBytes, int nBits = -1 );

	// Start reading from the specified buffer.
	// pData's start address must be dword-aligned.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartReading( const void *pData, int nBytes, int iStartBit = 0, int nBits = -1 );

	// Restart buffer reading.
	void			Reset();

	// This can be set to assign a name that gets output if the buffer overflows.
	const char*		GetDebugName();
	void			SetDebugName( const char *pName );


// Bit functions.
public:
	
	// Returns 0 or 1.
	int				ReadOneBit();


protected:

	unsigned int	CheckReadUBitLong(int numbits);		// For debugging.
	int				ReadOneBitNoCheck();				// Faster version, doesn't check bounds and is inlined.
	bool			CheckForOverflow(int nBits);


public:

	// Get the base pointer.
	const unsigned char*	GetBasePointer() const	{ return m_pData; }
		
	// Read a list of bits in..
	bool			ReadBits(void *pOut, int nBits);
	
	float			ReadBitAngle( int numbits );

	unsigned int	ReadUBitLong( int numbits );
	unsigned int	PeekUBitLong( int numbits );
	int				ReadSBitLong( int numbits );

	// reads an unsigned integer with variable bit length
	unsigned int	ReadUBitVar();
	
	// You can read signed or unsigned data with this, just cast to 
	// a signed int if necessary.
	unsigned int	ReadBitLong(int numbits, bool bSigned);
	
	float			ReadBitCoord();
	float			ReadBitFloat();
	float			ReadBitNormal();
	void			ReadBitVec3Coord( Vector& fa );
	void			ReadBitVec3Normal( Vector& fa );
	void			ReadBitAngles( QAngle& fa );


// Byte functions (these still read data in bit-by-bit).
public:
	
	int				ReadChar();
	int				ReadByte();
	int				ReadShort();
	int				ReadWord();
	long			ReadLong();
	float			ReadFloat();
	bool			ReadBytes(void *pOut, int nBytes);

	// Returns false if bufLen isn't large enough to hold the
	// string in the buffer.
	//
	// Always reads to the end of the string (so you can read the
	// next piece of data waiting).
	//
	// If bLine is true, it stops when it reaches a '\n' or a null-terminator.
	//
	// pStr is always null-terminated (unless bufLen is 0).
	//
	// pOutNumChars is set to the number of characters left in pStr when the routine is 
	// complete (this will never exceed bufLen-1).
	//
	bool			ReadString( char *pStr, int bufLen, bool bLine=false, int *pOutNumChars=nullptr );

// Status.
public:
	int				GetNumBytesLeft() const;
	int				GetNumBytesRead() const;
	int				GetNumBitsLeft() const;
	int				GetNumBitsRead() const;

	// Has the buffer overflowed?
	inline bool		IsOverflowed() const {return m_bOverflow;}

	bool			Seek(int iBit);					// Seek to a specific bit.
	bool			SeekRelative(int iBitDelta);	// Seek to an offset from the current position.

	// Called when the buffer is overflowed.
	inline void		SetOverflowFlag();


public:

	// The current buffer.
	const unsigned char*	m_pData;
	int						m_nDataBytes;
	int						m_nDataBits;
	
	// Where we are in the buffer.
	int				m_iCurBit;


private:	

	// Errors?
	bool			m_bOverflow;

	const char		*m_pDebugName;
};

//-----------------------------------------------------------------------------
// Inlines.
//-----------------------------------------------------------------------------

inline int bf_read::GetNumBytesRead() const
{
	return BitByte(m_iCurBit);
}

inline int bf_read::GetNumBitsLeft() const
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_read::GetNumBytesLeft() const
{
	return GetNumBitsLeft() >> 3;
}

inline int bf_read::GetNumBitsRead() const
{
	return m_iCurBit;
}

inline void bf_read::SetOverflowFlag()
{
	m_bOverflow = true;
}

// Seek to an offset from the current position.
inline bool	bf_read::SeekRelative(int iBitDelta)		
{
	return Seek(m_iCurBit+iBitDelta);
}	

inline bool bf_read::CheckForOverflow(int nBits)
{
	if( m_iCurBit + nBits > m_nDataBits )
	{
		SetOverflowFlag();
	}

	return m_bOverflow;
}

inline int bf_read::ReadOneBitNoCheck()
{
	int value = m_pData[m_iCurBit >> 3] & (1 << (m_iCurBit & 7));
	++m_iCurBit;
	return !!value;
}

inline int bf_read::ReadOneBit()
{
	return (!CheckForOverflow(1)) ?	ReadOneBitNoCheck() : 0;
}

inline float bf_read::ReadBitFloat()
{
	long val;

	assert(sizeof(float) == sizeof(long));
	assert(sizeof(float) == 4);

	if(CheckForOverflow(32))
		return 0.0f;

	int bit = m_iCurBit & 0x7;
	int byte = m_iCurBit >> 3;
	val = m_pData[byte] >> bit;
	val |= ((int)m_pData[byte + 1]) << (8 - bit);
	val |= ((int)m_pData[byte + 2]) << (16 - bit);
	val |= ((int)m_pData[byte + 3]) << (24 - bit);
	if (bit != 0)
		val |= ((int)m_pData[byte + 4]) << (32 - bit);
	m_iCurBit += 32;
	void *bits = &val;
	return *reinterpret_cast<float *>(bits);
}


inline unsigned int bf_read::ReadUBitLong( int numbits )
{
	extern unsigned long g_ExtraMasks[32];

	if ( (m_iCurBit+numbits) > m_nDataBits )
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		return 0;
	}

	assert( numbits > 0 && numbits <= 32 );

	// Read the current dword.
	int idword1 = m_iCurBit >> 5;
	unsigned int dword1 = ((unsigned int*)m_pData)[idword1];
	dword1 >>= (m_iCurBit & 31); // Get the bits we're interested in.

	m_iCurBit += numbits;
	unsigned int ret = dword1;

	// Does it span this dword?
	if ( (m_iCurBit-1) >> 5 == idword1 )
	{
		if(numbits != 32)
			ret &= g_ExtraMasks[numbits];
	}
	else
	{
		int nExtraBits = m_iCurBit & 31;
		unsigned int dword2 = ((unsigned int*)m_pData)[idword1+1] & g_ExtraMasks[nExtraBits];
		
		// No need to mask since we hit the end of the dword.
		// Shift the second dword's part into the high bits.
		ret |= (dword2 << (numbits - nExtraBits));
	}

	return ret;
}


#endif // BITBUF_H