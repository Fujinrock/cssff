#include "DataTables.h"
#include "DemoParser.h"
#include "Errors.h"

void DemoParser::ParseDataTables( bf_read &reader )
{
	while( reader.ReadOneBit() )
	{
		SendTable table;
		table.m_bNeedsDecoder = reader.ReadOneBit();
		reader.ReadString( table.m_tableName, sizeof(table.m_tableName) );
		table.m_nProps = reader.ReadUBitLong( 9 );

		for(int i = 0; i < table.m_nProps; ++i)
		{
			SendProp prop;
			prop.m_propType = (SendPropType)reader.ReadUBitLong( 5 );
			reader.ReadString( prop.m_propName, sizeof(prop.m_propName) );
			prop.m_flags = reader.ReadUBitLong( 13 );

			if( prop.m_propType == DPT_DataTable || (prop.m_flags & SPROP_EXCLUDE) > 0 )
			{
				reader.ReadString( prop.m_dtname, sizeof(prop.m_dtname) );
			}
			else
			{
				switch( prop.m_propType )
				{
					case DPT_String:
					case DPT_Int:
					case DPT_Float:
					case DPT_Vector:
					{
						prop.m_fLowValue = reader.ReadFloat();
						prop.m_fHighValue = reader.ReadFloat();
						prop.m_nBits = reader.ReadUBitLong( 6 );
						break;
					}
					case DPT_Array:
					{
						prop.m_nNumElements = reader.ReadUBitLong( 10 );
						break;
					}
					default:
						throw ParsingError_t( "invalid SendProp type in ParseDataTable" );
				}
			}

			table.m_props.push_back( prop );
		}

		m_DataTables.push_back( table );
	}

	short nServerClasses = reader.ReadShort();

	if( !nServerClasses )
		throw ParsingError_t( "no server classes in ParseDataTable" );

	for ( int i = 0; i < nServerClasses; i++ )
	{
		ServerClass_t entry;
		entry.nClassID = reader.ReadShort();
		if ( entry.nClassID >= nServerClasses )
		{
			throw ParsingError_t( "invalid class index in ParseDataTable" );
		}

		reader.ReadString( entry.strName, sizeof( entry.strName ) );
		reader.ReadString( entry.strDTName, sizeof( entry.strDTName ) );

		// Find the data table by name
		entry.nDataTable = -1;
		for ( size_t j = 0; j < m_DataTables.size(); ++j )
		{
			if ( strcmp( entry.strDTName, m_DataTables[ j ].m_tableName ) == 0 )
			{
				entry.nDataTable = j;
				break;
			}
		}

		if( entry.nDataTable == -1 )
			throw ParsingError_t( "data table for server class not found in ParseDataTable" );

		m_ServerClasses.push_back( entry );
	}

	for ( int i = 0; i < nServerClasses; ++i )
	{
		FlattenDataTable( i );
	}

	// Perform integer log2() to set server class bits
	int nTemp = nServerClasses;
	m_iServerClassBits = 0;
	while(nTemp >>= 1)
		++m_iServerClassBits;

	m_iServerClassBits++;
}

SendTable *DemoParser::GetTableByName( const char *pName )
{
	for ( size_t i = 0; i < m_DataTables.size(); i++ )
	{
		if( strcmp( m_DataTables[ i ].m_tableName, pName ) == 0 )
		{
			return &(m_DataTables[ i ]);
		}
	}
	return nullptr;
}

SendTable *DemoParser::GetTableByClassID( uint32 iClassID )
{
	for ( size_t i = 0; i < m_ServerClasses.size(); i++ )
	{
		if ( m_ServerClasses[ i ].nClassID == iClassID )
		{
			return &(m_DataTables[ m_ServerClasses[i].nDataTable ]);
		}
	}
	return nullptr;
}

void DemoParser::GatherExcludes( SendTable *pTable )
{
	for ( int iProp = 0; iProp < pTable->m_nProps; ++iProp )
	{
		const SendProp& sendProp = pTable->m_props[ iProp ];
		if ( sendProp.m_flags & SPROP_EXCLUDE )
		{
			m_currentExcludes.emplace_back( sendProp.m_propName, sendProp.m_dtname, pTable->m_tableName );
		}

		if ( sendProp.m_propType == DPT_DataTable )
		{
			SendTable *pSubTable = GetTableByName( sendProp.m_dtname );
			if ( pSubTable != nullptr )
			{
				GatherExcludes( pSubTable );
			}
		}
	}
}

bool DemoParser::IsPropExcluded( SendTable *pTable, const SendProp& checkSendProp )
{
	for ( size_t i = 0; i < m_currentExcludes.size(); ++i )
	{
		if( strcmp( pTable->m_tableName, m_currentExcludes[ i ].m_pDTName ) == 0
		&& strcmp( checkSendProp.m_propName, m_currentExcludes[ i ].m_pVarName ) == 0 )
		{
			return true;
		}
	}
	return false;
}

void DemoParser::GatherProps_IterateProps( SendTable *pTable, int nServerClass, std::vector< FlattenedPropEntry > &flattenedProps )
{
	for ( int iProp = 0; iProp < pTable->m_nProps; ++iProp )
	{
		const SendProp& sendProp = pTable->m_props[ iProp ];

		if ( ( sendProp.m_flags & SPROP_INSIDEARRAY )
		|| ( sendProp.m_flags & SPROP_EXCLUDE )
		|| IsPropExcluded( pTable, sendProp ) )
		{
			continue;
		}

		if ( sendProp.m_propType == DPT_DataTable )
		{
			SendTable *pSubTable = GetTableByName( sendProp.m_dtname );
			if ( pSubTable != nullptr )
			{
				if ( sendProp.m_flags & SPROP_COLLAPSIBLE )
				{
					GatherProps_IterateProps( pSubTable, nServerClass, flattenedProps );
				}
				else
				{
					GatherProps( pSubTable, nServerClass );
				}
			}
		}
		else
		{
			if ( sendProp.m_propType == DPT_Array )
			{
				flattenedProps.emplace_back( &sendProp, &(pTable->m_props[ iProp - 1 ]) );
			}
			else
			{
				flattenedProps.emplace_back( &sendProp, nullptr );
			}
		}
	}
}

void DemoParser::GatherProps( SendTable *pTable, int nServerClass )
{
	std::vector< FlattenedPropEntry > tempFlattenedProps;
	GatherProps_IterateProps( pTable, nServerClass, tempFlattenedProps );

	std::vector< FlattenedPropEntry > &flattenedProps = m_ServerClasses[ nServerClass ].flattenedProps;
	for ( size_t i = 0; i < tempFlattenedProps.size(); ++i )
	{
		flattenedProps.push_back( tempFlattenedProps[ i ] );
	}
}

void DemoParser::FlattenDataTable( int nServerClass )
{
	SendTable *pTable = &m_DataTables[ m_ServerClasses[ nServerClass ].nDataTable ];

	m_currentExcludes.clear();
	GatherExcludes( pTable );

	GatherProps( pTable, nServerClass );

	std::vector< FlattenedPropEntry > &flattenedProps = m_ServerClasses[ nServerClass ].flattenedProps;

	int start = 0;

	for ( size_t i = 0; i < flattenedProps.size(); ++i )
	{
		FlattenedPropEntry p = flattenedProps[i];

		if ( (p.m_prop->m_flags & SPROP_CHANGES_OFTEN) != 0 )
		{
			flattenedProps[i] = flattenedProps[start];
			flattenedProps[start] = p;
			++start;
		}
	}
}