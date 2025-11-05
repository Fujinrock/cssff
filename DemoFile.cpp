#include "DemoFile.h"
#include "Settings.h"
#include <fstream>
#include <assert.h>

DemoFile::DemoFile( const std::string &filename )
{
	m_filename = filename;
	RemoveFileNameFolders( m_filename );

	std::ifstream file( filename, std::ios::binary );

	if( !file.is_open() )
	{
		m_filebuffer = nullptr;
		m_filesize = 0;
		m_error = COULD_NOT_OPEN_FILE;
		return;
	}

	file.seekg( 0, std::ios_base::end );
	std::streamsize size = file.tellg();
	file.seekg( 0, std::ios_base::beg );

	if( size > 0 )
	{
		m_filebuffer = new char[ (uint32)size ];
	}
	else
	{
		m_filebuffer = nullptr;
		m_filesize = 0;
		m_error = FILE_TOO_SMALL;
		file.close();
		return;
	}

	file.read( m_filebuffer, size );

	file.close();

	m_filesize = (uint32)size;

	CheckValidity();
}

void DemoFile::CheckValidity( void )
{
	if( !m_filebuffer )
	{
		m_error = COULD_NOT_OPEN_FILE;
		return;
	}

	if( m_filesize < sizeof( demoheader_t ) )
	{
		m_error = FILE_TOO_SMALL;
		return;
	}

	demoheader_t *hdr = (demoheader_t *)m_filebuffer;

	if( strcmp( hdr->demofilestamp, DEMO_HEADER_ID ) )
	{
		m_error = INVALID_HDR_ID;
		return;
	}

	if( hdr->demoprotocol != DEMO_PROTOCOL )
	{
		m_error = INVALID_DEM_PROTOCOL;
		return;
	}

	if( strcmp( hdr->gamedirectory, CSS_GAMEDIR ) )
	{
		m_error = INVALID_GAMEDIR;
		return;
	}

	if( hdr->networkprotocol != NETWORK_PROTOCOL_V34 )
	{
		m_error = INVALID_NET_PROTOCOL;
		return;
	}

	m_error = DEMO_OK;
}

DemoFile::~DemoFile( void )
{
	if( m_filebuffer )
		delete[] m_filebuffer;
}

bool DemoFile::IsValidDemo( void ) const
{
	return m_error == DEMO_OK;
}

DemoError DemoFile::GetError( void ) const
{
	return m_error;
}

char *DemoFile::GetBuffer( void ) const
{
	return m_filebuffer;
}

std::string DemoFile::GetFileName( void ) const
{
	return m_filename;
}

uint32 DemoFile::GetFileSize( void ) const
{
	return m_filesize;
}

DemoFile &DemoFile::operator=( const DemoFile & )
{
	assert( false );
	return *this;
}