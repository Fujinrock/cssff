#pragma once

#define MAX_STRING_TABLES	64

struct StringTableData_t
{
	char	szName[ 64 ];
	int		nMaxEntries;
	int		nUserDataSize;
	int		nUserDataSizeBits;
	bool	bUserDataFixedSize;
};