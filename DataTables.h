#pragma once

#include <vector>
#include "Entities.h"

class SendTable
{
public:
	char m_tableName[256];
	bool m_bNeedsDecoder;
	int m_nProps;
	std::vector< SendProp > m_props;
};

struct ServerClass_t
{
	int nClassID;
	char strName[ 256 ];
	char strDTName[ 256 ];
	int nDataTable;

	std::vector< FlattenedPropEntry > flattenedProps;
};

typedef std::vector< ServerClass_t > ServerClassVector;
typedef std::vector< SendTable > SendTableVector;
typedef std::vector< ExcludeEntry > ExcludeEntryVector;