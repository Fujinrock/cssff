#include "Settings.h"
#include "Common.h"
#include "Weapons.h"
#include "Frag.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fstream>
#include <vector>

extern std::string g_ProgramDirectory;
extern std::string g_BatchDirectory;

// Keys used in reading the settings file and in finding settings per category
//
// If you add more keys, you need to:
// 1. add a default value to it in the constructor of SettingsManager
// 2. add it to LoadSettings
// 3. implement the logic for it
#define KEY_DUMP_TO_FILE						"dump_to_file"
#define KEY_WRITE_FILE_TO_DEMO_DIR				"write_output_to_demo_directory"
#define KEY_ENABLE_BATCH_PROCESSING				"enable_batch_processing"
#define KEY_TICK_5KS							"tick_5ks"
#define KEY_TICK_4KS							"tick_4ks"
#define KEY_TICK_3KS							"tick_3ks"
#define KEY_TICK_DOUBLES						"tick_doubles"
#define KEY_TICK_TRIPLES						"tick_triples"
#define KEY_TICK_QUADROS						"tick_quadros"
#define KEY_TICK_PENTAS							"tick_pentas"
#define KEY_TICK_FLASH_SMOKE_KILLS				"tick_flash_smoke_kills"
#define KEY_TICK_JUMPSHOTS						"tick_jumpshots"
#define KEY_TICK_NOSCOPES						"tick_noscopes"
#define KEY_TICK_FLICKSHOTS						"tick_flickshots"
#define KEY_TICK_WALLBANGS						"tick_wallbangs"
#define KEY_TICK_FRAGS_VS_BOTS					"tick_frags_vs_bots"
#define KEY_TICK_FRAGS_BY_BOTS					"tick_frags_by_bots"
#define KEY_5K_MAX_TIME							"5k_max_time"
#define KEY_4K_MAX_TIME							"4k_max_time"
#define KEY_3K_MAX_TIME							"3k_max_time"
#define KEY_TICK_SLOW_STATIONARY_5KS			"tick_slow_stationary_5ks"
#define KEY_TICK_SLOW_STATIONARY_4KS			"tick_slow_stationary_4ks"
#define KEY_TICK_SLOW_STATIONARY_3KS			"tick_slow_stationary_3ks"
#define KEY_SLOW_5K_MAX_RANGE					"slow_5k_max_range"
#define KEY_SLOW_4K_MAX_RANGE					"slow_4k_max_range"
#define KEY_SLOW_3K_MAX_RANGE					"slow_3k_max_range"
#define	KEY_5K_MIN_HEADSHOTS					"5k_min_headshots"
#define	KEY_4K_MIN_HEADSHOTS					"4k_min_headshots"
#define	KEY_3K_MIN_HEADSHOTS					"3k_min_headshots"
#define KEY_5K_MUST_INCLUDE_SP_KILL				"5k_must_include_special_kill"
#define KEY_4K_MUST_INCLUDE_SP_KILL				"4k_must_include_special_kill"
#define KEY_3K_MUST_INCLUDE_SP_KILL				"3k_must_include_special_kill"
#define KEY_5K_SP_KILL_EXTRA_MAX_TIME			"5k_special_kill_extra_max_time"
#define KEY_4K_SP_KILL_EXTRA_MAX_TIME			"4k_special_kill_extra_max_time"
#define KEY_3K_SP_KILL_EXTRA_MAX_TIME			"3k_special_kill_extra_max_time"
#define KEY_DOUBLE_MIN_HEADSHOTS				"double_min_headshots"
#define KEY_TRIPLE_MIN_HEADSHOTS				"triple_min_headshots"
#define KEY_QUADRO_MIN_HEADSHOTS				"quadro_min_headshots"
#define KEY_PENTA_MIN_HEADSHOTS					"penta_min_headshots"
#define KEY_SP_DOUBLE_IGNORES_MIN_HS			"special_double_ignores_min_hs"
#define KEY_SP_TRIPLE_IGNORES_MIN_HS			"special_triple_ignores_min_hs"
#define KEY_SP_QUADRO_IGNORES_MIN_HS			"special_quadro_ignores_min_hs"
#define KEY_SP_PENTA_IGNORES_MIN_HS				"special_penta_ignores_min_hs"
#define KEY_NOSCOPE_MIN_DISTANCE				"noscope_min_distance"
#define KEY_NOSCOPE_MIN_DISTANCE_HS_MOD			"noscope_min_distance_hs_modifier"
#define KEY_NOSCOPE_MIN_DISTANCE_WB_MOD			"noscope_min_distance_wb_modifier"
#define KEY_JUMPSHOT_MIN_POSTKILL_AIR_TIME		"jumpshot_min_post_kill_air_time"
#define KEY_JUMPSHOT_MIN_DISTANCE				"jumpshot_min_distance"
#define KEY_JUMPSHOT_MIN_DISTANCE_HS_MOD		"jumpshot_min_distance_hs_modifier"
#define KEY_JUMPSHOT_MIN_DISTANCE_WB_MOD		"jumpshot_min_distance_wb_modifier"
#define KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE		"jumpshot_always_tick_multiple"
#define KEY_JUMPSHOT_MULTIPLE_MAX_DT			"jumpshot_multiple_max_delta_time"
#define KEY_WALLBANG_HEADSHOT_ONLY				"wallbang_headshot_only"
#define KEY_WALLBANG_REQUIRE_TWO				"wallbang_require_two"
#define KEY_WALLBANG_ANOTHER_WB_MAX_DT			"wallbang_another_wallbang_max_delta_time"
#define KEY_FLICKSHOT_MAX_DURATION				"flickshot_max_duration"
#define KEY_FLICKSHOT_HEADSHOT_ONLY				"flickshot_headshot_only"
#define KEY_FLICKSHOT_MIN_DISTANCE				"flickshot_min_distance"
#define KEY_FLICKSHOT_MIN_ANGLE_MOD				"flickshot_min_angle_modifier"

// Category names for settings
#define CAT_NAME_GENERAL						"General"
#define CAT_NAME_KNIFE							"Knife"
#define CAT_NAME_PISTOLS						"Pistols"
#define CAT_NAME_SHOTGUNS						"Shotguns"
#define CAT_NAME_SMGS							"Smgs"
#define CAT_NAME_RIFLES							"Rifles"
#define CAT_NAME_SNIPERS						"Snipers"
#define CAT_NAME_AUTOSNIPERS					"AutoSnipers"
#define CAT_NAME_GRENADES						"Grenades"

#define DEFAULT_SETTINGS_FILE		"cssff_settings.ini"
#define SettingEnabled( value )		(value == "1" || !_stricmp(value.c_str(), "true"))
#define SettingDisabled( value )	(value == "0" || !_stricmp(value.c_str(), "false"))

#define PrintInvalidValueWarning( key, value, type )\
	printf("Warning: key \"%s\" has invalid value \"%s\" (expected %s)\n", key, value.c_str(), #type)

// Use these macros in LoadSettings
#define SetKeyValueBool( key, value )\
	int __nValue = -1;\
	if( SettingEnabled( value ) )\
		__nValue = 1;\
	else if( SettingDisabled( value ) )\
		__nValue = 0;\
	if( __nValue != -1 ){\
		for(size_t i=0;i<current_categories.size();++i)\
			m_weaponSettings[ current_categories[i] ][ key ].m_bool = __nValue == 1;\
	}\
	else { PrintInvalidValueWarning(key, value, boolean); }

#define SetKeyValueFloat( key, value )\
	try { float __fValue = stof( value );\
			for(size_t i=0;i<current_categories.size();++i)\
				m_weaponSettings[ current_categories[i] ][ key ].m_float = __fValue; }\
	catch( ... ) { PrintInvalidValueWarning(key, value, decimal); }

#define SetKeyValueInt( key, value )\
	try { int __nValue = stoi( value );\
			for(size_t i=0;i<current_categories.size();++i)\
				m_weaponSettings[ current_categories[i] ][ key ].m_int = __nValue; }\
	catch( ... ) { PrintInvalidValueWarning(key, value, integer); }

// Macro for a function that returns the value for a settings field for a specific weapon
// Checks for weapon settings, weapon category settings and finally general settings
// Value type has to be found in setting_value union
#define ReturnSettingsValueForWeapon( key, valuetype )\
	if( m_weaponSettings.find( weapon ) != m_weaponSettings.end() ){\
		if( m_weaponSettings[ weapon ].find( key ) != m_weaponSettings[ weapon ].end() )\
			return m_weaponSettings[ weapon ][ key ].m_##valuetype;\
	}\
	CSWeaponCategory category = GetWeaponCategory( weapon );\
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];\
	if( category_settings.find( key ) != category_settings.end() )\
		return category_settings[ key ].m_##valuetype;\
	else\
		return m_weaponSettings[ CATEGORY_GENERAL ][ key ].m_##valuetype

// =====================================================================================================================================================================

// Accessor to the singleton (use the Settings macro to get it neatly)
SettingsManager *SettingsManager::Instance( void )
{
	static SettingsManager settings;

	return &settings;
}

// =====================================================================================================================================================================

SettingsManager::SettingsManager()
{
	m_bSettingsLoaded = false;

	// Populate the weapon settings map with the categories, but not the actual settings
	// Don't add the weapons themselves - they will be added only if they're found in the settings file
	m_weaponSettings[ CATEGORY_GENERAL ];	// General settings that apply to everything by default
	m_weaponSettings[ CATEGORY_KNIFE ];
	m_weaponSettings[ CATEGORY_PISTOL ];
	m_weaponSettings[ CATEGORY_SHOTGUN ];
	m_weaponSettings[ CATEGORY_SMG ];
	m_weaponSettings[ CATEGORY_RIFLE ];
	m_weaponSettings[ CATEGORY_SNIPER ];
	m_weaponSettings[ CATEGORY_AUTOSNIPER ];
	m_weaponSettings[ CATEGORY_GRENADE ];

	WeaponSettingsField &general_settings = m_weaponSettings[ CATEGORY_GENERAL ];

	// Set the default values for general settings
	general_settings[ KEY_DUMP_TO_FILE ].m_bool = false;
	general_settings[ KEY_WRITE_FILE_TO_DEMO_DIR ].m_bool = false;
	general_settings[ KEY_ENABLE_BATCH_PROCESSING ].m_bool = false;
	general_settings[ KEY_TICK_5KS ].m_bool = true;
	general_settings[ KEY_TICK_4KS ].m_bool = true;
	general_settings[ KEY_TICK_3KS ].m_bool = true;
	general_settings[ KEY_TICK_DOUBLES ].m_bool = true;
	general_settings[ KEY_TICK_TRIPLES ].m_bool = true;
	general_settings[ KEY_TICK_QUADROS ].m_bool = true;
	general_settings[ KEY_TICK_PENTAS ].m_bool = true;
	general_settings[ KEY_TICK_FLASH_SMOKE_KILLS ].m_bool = true;
	general_settings[ KEY_TICK_JUMPSHOTS ].m_bool = true;
	general_settings[ KEY_TICK_NOSCOPES ].m_bool = true;
	general_settings[ KEY_TICK_FLICKSHOTS ].m_bool = true;
	general_settings[ KEY_TICK_WALLBANGS ].m_bool = true;
	general_settings[ KEY_WALLBANG_HEADSHOT_ONLY ].m_bool = true;
	general_settings[ KEY_WALLBANG_REQUIRE_TWO ].m_bool = true;
	general_settings[ KEY_WALLBANG_ANOTHER_WB_MAX_DT ].m_float = 4.0;
	general_settings[ KEY_TICK_FRAGS_VS_BOTS ].m_bool = false;
	general_settings[ KEY_TICK_FRAGS_BY_BOTS ].m_bool = true;
	general_settings[ KEY_5K_MAX_TIME ].m_float = -1.f;
	general_settings[ KEY_4K_MAX_TIME ].m_float = 10.f;
	general_settings[ KEY_3K_MAX_TIME ].m_float = 2.f;
	general_settings[ KEY_TICK_SLOW_STATIONARY_5KS ].m_bool = true;
	general_settings[ KEY_TICK_SLOW_STATIONARY_4KS ].m_bool = false;
	general_settings[ KEY_TICK_SLOW_STATIONARY_3KS ].m_bool = false;
	general_settings[ KEY_SLOW_5K_MAX_RANGE ].m_float = 256.f;
	general_settings[ KEY_SLOW_4K_MAX_RANGE ].m_float = 100.f;
	general_settings[ KEY_SLOW_3K_MAX_RANGE ].m_float = 0.f;
	general_settings[ KEY_5K_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_4K_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_3K_MIN_HEADSHOTS ].m_int = 1;
	general_settings[ KEY_5K_MUST_INCLUDE_SP_KILL ].m_bool = false;
	general_settings[ KEY_4K_MUST_INCLUDE_SP_KILL ].m_bool = false;
	general_settings[ KEY_3K_MUST_INCLUDE_SP_KILL ].m_bool = false;
	general_settings[ KEY_5K_SP_KILL_EXTRA_MAX_TIME ].m_float = 5.f;
	general_settings[ KEY_4K_SP_KILL_EXTRA_MAX_TIME ].m_float = 2.f;
	general_settings[ KEY_3K_SP_KILL_EXTRA_MAX_TIME ].m_float = 0.5;
	general_settings[ KEY_DOUBLE_MIN_HEADSHOTS ].m_int = 1;
	general_settings[ KEY_TRIPLE_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_QUADRO_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_PENTA_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_SP_DOUBLE_IGNORES_MIN_HS ].m_bool = true;
	general_settings[ KEY_SP_TRIPLE_IGNORES_MIN_HS ].m_bool = true;
	general_settings[ KEY_SP_QUADRO_IGNORES_MIN_HS ].m_bool = true;
	general_settings[ KEY_SP_PENTA_IGNORES_MIN_HS ].m_bool = true;
	general_settings[ KEY_NOSCOPE_MIN_DISTANCE ].m_float = 1000.f;
	general_settings[ KEY_NOSCOPE_MIN_DISTANCE_HS_MOD ].m_float = 0.5f;
	general_settings[ KEY_NOSCOPE_MIN_DISTANCE_WB_MOD ].m_float = 0.5f;
	general_settings[ KEY_JUMPSHOT_MIN_POSTKILL_AIR_TIME ].m_float = 0.1f;
	general_settings[ KEY_JUMPSHOT_MIN_DISTANCE ].m_float = 1000.f;
	general_settings[ KEY_JUMPSHOT_MIN_DISTANCE_HS_MOD ].m_float = 0.5f;
	general_settings[ KEY_JUMPSHOT_MIN_DISTANCE_WB_MOD ].m_float = 0.5f;
	general_settings[ KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE ].m_bool = true;
	general_settings[ KEY_JUMPSHOT_MULTIPLE_MAX_DT ].m_float = 2.f;
	general_settings[ KEY_FLICKSHOT_MAX_DURATION ].m_int = 150;
	general_settings[ KEY_FLICKSHOT_HEADSHOT_ONLY ].m_bool = false;
	general_settings[ KEY_FLICKSHOT_MIN_DISTANCE ].m_float = 100.f;
	general_settings[ KEY_FLICKSHOT_MIN_ANGLE_MOD ].m_float = 1.f;

	m_iMaxFlickDuration = general_settings[ KEY_FLICKSHOT_MAX_DURATION ].m_int;

	memset( &m_fcats, 0, sizeof(m_fcats) );
}

// =====================================================================================================================================================================
/**
 * Builds a filepath to a singular ini file found in the directory specified
 * @param sDirectory			directory path to search in
 * @param outPath				string that will hold the built filepath (only if file is found)
 * @return						false if failed to find a file or there were multiple ini files, true otherwise
 */
bool GetSettingsFileFromDirectory( const std::string &sDirectory, std::string &outPath )
{
	std::string strSearchPath = sDirectory + "*.ini";
	std::string strFilename = "";

	WIN32_FIND_DATAA data;

	HANDLE hFile = FindFirstFileA( strSearchPath.c_str(), &data );

	if( hFile == INVALID_HANDLE_VALUE )
	{
		FindClose( hFile );
		return false;
	}

	do
	{
		if( !( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			std::string sTemp = data.cFileName;

			// Make sure it's actually an ini extension and doesn't just start with that
			if( !FileHasExtension( sTemp, "ini" ) )
				continue;

			// If there are multiple settings files in the directory, the default file will be used
			if( !strFilename.empty() )
			{
				printf( "Warning: Multiple settings files found in batch directory - using default file!\n" );
				FindClose( hFile );
				return false;
			}

			strFilename = sTemp;
		}
	}
	while( FindNextFileA( hFile, &data ) != 0 );

	FindClose( hFile );

	if( strFilename.empty() )
		return false;

	outPath = sDirectory + strFilename;

	return true;
}

// =====================================================================================================================================================================

// This is where the settings for all categories are read from the settings file
void SettingsManager::LoadSettings( const char *szSettingsFile, bool bBatchDirSupplied )
{
	using namespace std;

	if( m_bSettingsLoaded )
		return;

	string sConfigPath;

	if( !szSettingsFile )
	{
		if( !bBatchDirSupplied || !GetSettingsFileFromDirectory( g_BatchDirectory, sConfigPath ) )
		{
			// Use default file if no file or batch directory with file was specified
			sConfigPath = g_ProgramDirectory + DEFAULT_SETTINGS_FILE;
		}
	}
	else
	{
		sConfigPath = szSettingsFile;
	}

	ifstream file( sConfigPath );

	if( !file.is_open() )
	{
		// If the user wants to parse the demos in the program's folder or just dragged some demos onto the program,
		// try opening any settings file we can find from the program folder
		if( !szSettingsFile && !bBatchDirSupplied && GetSettingsFileFromDirectory( g_ProgramDirectory, sConfigPath ) )
		{
			file.open( sConfigPath );
		}

		if( !file.is_open() )
		{
			RemoveFileNameFolders( sConfigPath );
			printf( "Warning: Could not open settings file \"%s\" - using built-in default values!\n", sConfigPath.c_str() );
			return;
		}
	}

	RemoveFileNameFolders( sConfigPath );

	if( sConfigPath != DEFAULT_SETTINGS_FILE )
	{
		printf( "Using settings from file \"%s\"\n", sConfigPath.c_str() );
	}

	// Allow chaining multiple categories together
	std::vector< SettingsCategory > current_categories;
	current_categories.push_back( GetCategoryByName( CAT_NAME_GENERAL ) );
	bool last_line_was_category = false;

	string line;
	while( getline( file, line ) )
	{
		if( line.empty() || line[0] == '#' ) // Empty line or comment
			continue;

		auto delimiterPos = line.find( '=' ); // Find delimiter

		if( delimiterPos == string::npos ) // Not a valid key-value field, check if it's a category
		{
			TrimString( line );

			if( line.front() == '[' && line.back() == ']' )
			{
				// Remove brackets
				line = line.substr( 1, line.length() - 2 );

				SettingsCategory category = GetCategoryByName( line.c_str() );

				// Remove previous categories if they're not all chained
				if( !last_line_was_category )
					current_categories.clear();

				if( category != CATEGORY_INVALID )
				{
					last_line_was_category = true;
					current_categories.push_back( category );
				}
				else // Let the user know the category name is messed up
				{
					printf( "Warning: Invalid category \"%s\" in settings file!\n", line.c_str() );
				}
			}

			continue;
		}

		last_line_was_category = false;

		// No point in parsing the settings if there is no category to set them to
		if( current_categories.empty() )
			continue;

		string key = line.substr( 0, delimiterPos );
		string value = line.substr( delimiterPos+1 );
		// Remove spaces
		TrimString( key );
		TrimString( value );

#ifdef _DEBUG_PRINT_DETAILS
		printf( "[Settings] key: %s | value: %s\n", key.c_str(), value.c_str() );
#endif

		if( key == KEY_DUMP_TO_FILE )
		{
			SetKeyValueBool( KEY_DUMP_TO_FILE, value )
		}
		else if( key == KEY_WRITE_FILE_TO_DEMO_DIR )
		{
			SetKeyValueBool( KEY_WRITE_FILE_TO_DEMO_DIR, value )
		}
		else if( key == KEY_ENABLE_BATCH_PROCESSING )
		{
			SetKeyValueBool( KEY_ENABLE_BATCH_PROCESSING, value )
		}
		else if( key == KEY_TICK_5KS )
		{
			SetKeyValueBool( KEY_TICK_5KS, value )
		}
		else if( key == KEY_TICK_4KS )
		{
			SetKeyValueBool( KEY_TICK_4KS, value )
		}
		else if( key == KEY_TICK_3KS )
		{
			SetKeyValueBool( KEY_TICK_3KS, value )
		}
		else if( key == KEY_TICK_DOUBLES )
		{
			SetKeyValueBool( KEY_TICK_DOUBLES, value )
		}
		else if( key == KEY_TICK_TRIPLES )
		{
			SetKeyValueBool( KEY_TICK_TRIPLES, value )
		}
		else if( key == KEY_TICK_QUADROS )
		{
			SetKeyValueBool( KEY_TICK_QUADROS, value )
		}
		else if( key == KEY_TICK_PENTAS )
		{
			SetKeyValueBool( KEY_TICK_PENTAS, value )
		}
		else if( key == KEY_TICK_FLASH_SMOKE_KILLS )
		{
			SetKeyValueBool( KEY_TICK_FLASH_SMOKE_KILLS, value )
		}
		else if( key == KEY_TICK_JUMPSHOTS )
		{
			SetKeyValueBool( KEY_TICK_JUMPSHOTS, value )
		}
		else if( key == KEY_TICK_NOSCOPES )
		{
			SetKeyValueBool( KEY_TICK_NOSCOPES, value )
		}
		else if( key == KEY_TICK_FLICKSHOTS )
		{
			SetKeyValueBool( KEY_TICK_FLICKSHOTS, value )
		}
		else if( key == KEY_TICK_WALLBANGS )
		{
			SetKeyValueBool( KEY_TICK_WALLBANGS, value )
		}
		else if( key == KEY_WALLBANG_HEADSHOT_ONLY )
		{
			SetKeyValueBool( KEY_WALLBANG_HEADSHOT_ONLY, value )
		}
		else if( key == KEY_WALLBANG_REQUIRE_TWO )
		{
			SetKeyValueBool( KEY_WALLBANG_REQUIRE_TWO, value )
		}
		else if( key == KEY_WALLBANG_ANOTHER_WB_MAX_DT )
		{
			SetKeyValueFloat( KEY_WALLBANG_ANOTHER_WB_MAX_DT, value )
		}
		else if( key == KEY_TICK_FRAGS_VS_BOTS )
		{
			SetKeyValueBool( KEY_TICK_FRAGS_VS_BOTS, value )
		}
		else if( key == KEY_TICK_FRAGS_BY_BOTS )
		{
			SetKeyValueBool( KEY_TICK_FRAGS_BY_BOTS, value )
		}
		else if( key == KEY_5K_MAX_TIME )
		{
			SetKeyValueFloat( KEY_5K_MAX_TIME, value )
		}
		else if( key == KEY_4K_MAX_TIME )
		{
			SetKeyValueFloat( KEY_4K_MAX_TIME, value )
		}
		else if( key == KEY_3K_MAX_TIME )
		{
			SetKeyValueFloat( KEY_3K_MAX_TIME, value )
		}
		else if( key == KEY_TICK_SLOW_STATIONARY_5KS )
		{
			SetKeyValueBool( KEY_TICK_SLOW_STATIONARY_5KS, value )
		}
		else if( key == KEY_TICK_SLOW_STATIONARY_4KS )
		{
			SetKeyValueBool( KEY_TICK_SLOW_STATIONARY_4KS, value )
		}
		else if( key == KEY_TICK_SLOW_STATIONARY_3KS )
		{
			SetKeyValueBool( KEY_TICK_SLOW_STATIONARY_3KS, value )
		}
		else if( key == KEY_SLOW_5K_MAX_RANGE )
		{
			SetKeyValueFloat( KEY_SLOW_5K_MAX_RANGE, value )
		}
		else if( key == KEY_SLOW_4K_MAX_RANGE )
		{
			SetKeyValueFloat( KEY_SLOW_4K_MAX_RANGE, value )
		}
		else if( key == KEY_SLOW_3K_MAX_RANGE )
		{
			SetKeyValueFloat( KEY_SLOW_3K_MAX_RANGE, value )
		}
		else if( key == KEY_5K_MIN_HEADSHOTS )
		{
			SetKeyValueInt( KEY_5K_MIN_HEADSHOTS, value )
		}
		else if( key == KEY_4K_MIN_HEADSHOTS )
		{
			SetKeyValueInt( KEY_4K_MIN_HEADSHOTS, value )
		}
		else if( key == KEY_3K_MIN_HEADSHOTS )
		{
			SetKeyValueInt( KEY_3K_MIN_HEADSHOTS, value )
		}
		else if( key == KEY_5K_MUST_INCLUDE_SP_KILL )
		{
			SetKeyValueBool( KEY_5K_MUST_INCLUDE_SP_KILL, value )
		}
		else if( key == KEY_4K_MUST_INCLUDE_SP_KILL )
		{
			SetKeyValueBool( KEY_4K_MUST_INCLUDE_SP_KILL, value )
		}
		else if( key == KEY_3K_MUST_INCLUDE_SP_KILL )
		{
			SetKeyValueBool( KEY_3K_MUST_INCLUDE_SP_KILL, value )
		}
		else if( key == KEY_5K_SP_KILL_EXTRA_MAX_TIME )
		{
			SetKeyValueFloat( KEY_5K_SP_KILL_EXTRA_MAX_TIME, value )
		}
		else if( key == KEY_4K_SP_KILL_EXTRA_MAX_TIME )
		{
			SetKeyValueFloat( KEY_4K_SP_KILL_EXTRA_MAX_TIME, value )
		}
		else if( key == KEY_3K_SP_KILL_EXTRA_MAX_TIME )
		{
			SetKeyValueFloat( KEY_3K_SP_KILL_EXTRA_MAX_TIME, value )
		}
		else if( key == KEY_DOUBLE_MIN_HEADSHOTS )
		{
			SetKeyValueInt( KEY_DOUBLE_MIN_HEADSHOTS, value )
		}
		else if( key == KEY_TRIPLE_MIN_HEADSHOTS )
		{
			SetKeyValueInt( KEY_TRIPLE_MIN_HEADSHOTS, value )
		}
		else if( key == KEY_QUADRO_MIN_HEADSHOTS )
		{
			SetKeyValueInt( KEY_QUADRO_MIN_HEADSHOTS, value )
		}
		else if( key == KEY_PENTA_MIN_HEADSHOTS )
		{
			SetKeyValueInt( KEY_PENTA_MIN_HEADSHOTS, value )
		}
		else if( key == KEY_SP_DOUBLE_IGNORES_MIN_HS )
		{
			SetKeyValueBool( KEY_SP_DOUBLE_IGNORES_MIN_HS, value )
		}
		else if( key == KEY_SP_TRIPLE_IGNORES_MIN_HS )
		{
			SetKeyValueBool( KEY_SP_TRIPLE_IGNORES_MIN_HS, value )
		}
		else if( key == KEY_SP_QUADRO_IGNORES_MIN_HS )
		{
			SetKeyValueBool( KEY_SP_QUADRO_IGNORES_MIN_HS, value )
		}
		else if( key == KEY_SP_PENTA_IGNORES_MIN_HS )
		{
			SetKeyValueBool( KEY_SP_PENTA_IGNORES_MIN_HS, value )
		}
		else if( key == KEY_NOSCOPE_MIN_DISTANCE )
		{
			SetKeyValueFloat( KEY_NOSCOPE_MIN_DISTANCE, value )
		}
		else if( key == KEY_NOSCOPE_MIN_DISTANCE_HS_MOD )
		{
			SetKeyValueFloat( KEY_NOSCOPE_MIN_DISTANCE_HS_MOD, value )
		}
		else if( key == KEY_NOSCOPE_MIN_DISTANCE_WB_MOD )
		{
			SetKeyValueFloat( KEY_NOSCOPE_MIN_DISTANCE_WB_MOD, value )
		}
		else if( key == KEY_JUMPSHOT_MIN_POSTKILL_AIR_TIME )
		{
			SetKeyValueFloat( KEY_JUMPSHOT_MIN_POSTKILL_AIR_TIME, value )
		}
		else if( key == KEY_JUMPSHOT_MIN_DISTANCE )
		{
			SetKeyValueFloat( KEY_JUMPSHOT_MIN_DISTANCE, value )
		}
		else if( key == KEY_JUMPSHOT_MIN_DISTANCE_HS_MOD )
		{
			SetKeyValueFloat( KEY_JUMPSHOT_MIN_DISTANCE_HS_MOD, value )
		}
		else if( key == KEY_JUMPSHOT_MIN_DISTANCE_WB_MOD )
		{
			SetKeyValueFloat( KEY_JUMPSHOT_MIN_DISTANCE_WB_MOD, value )
		}
		else if( key == KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE )
		{
			SetKeyValueBool( KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE, value )
		}
		else if( key == KEY_JUMPSHOT_MULTIPLE_MAX_DT )
		{
			SetKeyValueFloat( KEY_JUMPSHOT_MULTIPLE_MAX_DT, value )
		}
		else if( key == KEY_FLICKSHOT_MAX_DURATION )
		{
			SetKeyValueInt( KEY_FLICKSHOT_MAX_DURATION, value )
		}
		else if( key == KEY_FLICKSHOT_HEADSHOT_ONLY )
		{
			SetKeyValueBool( KEY_FLICKSHOT_HEADSHOT_ONLY, value )
		}
		else if( key == KEY_FLICKSHOT_MIN_DISTANCE )
		{
			SetKeyValueFloat( KEY_FLICKSHOT_MIN_DISTANCE, value )
		}
		else if( key == KEY_FLICKSHOT_MIN_ANGLE_MOD )
		{
			SetKeyValueFloat( KEY_FLICKSHOT_MIN_ANGLE_MOD, value )
		}
		else
		{
			printf( "Warning: Invalid key \"%s\" in settings file!\n", key.c_str() );
		}
	}

	file.close();

	// Find longest flick duration
	// and cap the maximum duration to avoid growing the view angle lists too much
	const int iFlickshotAbsoluteMaxDuration = 300;

	for( auto &[category, fields] : m_weaponSettings )
	{
		if( fields.find( KEY_FLICKSHOT_MAX_DURATION ) != fields.end() )
		{
			int duration = fields[ KEY_FLICKSHOT_MAX_DURATION ].m_int;

			if( duration > iFlickshotAbsoluteMaxDuration )
			{
				duration = iFlickshotAbsoluteMaxDuration;
				fields[ KEY_FLICKSHOT_MAX_DURATION ].m_int = duration;
			}

			if( duration > m_iMaxFlickDuration )
				m_iMaxFlickDuration = duration;
		}
	}

	m_bSettingsLoaded = true;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickFrag( MultiKillFragType type, CSWeaponID *pWeapons, short num_weapons, float frag_time, float farthest_distance, short headshots, short num_special_kills, bool &outIsStationary )
{
	CSWeaponID weaponID;
	CSWeaponCategory weaponCategory;

	GetMultiKillFragWeaponCategory( pWeapons, num_weapons, weaponID, weaponCategory );

	WeaponSettingsField *weapon_settings = nullptr;
	WeaponSettingsField &category_settings = m_weaponSettings[ weaponCategory ];
	WeaponSettingsField &general_settings = GetGeneralSettings();

	if( weaponID != WEAPON_NONE && m_weaponSettings.find( weaponID ) != m_weaponSettings.end() )
	{
		weapon_settings = &m_weaponSettings[ weaponID ];
	}

	outIsStationary = false;


	// ===== Determine which keys to check ==============================

	Key
		tick_key,
		tick_stationary_key,
		stationary_max_range_key,
		max_time_key,
		min_hs_key,
		require_sp_kill_key,
		sp_kill_extra_time_key;

	switch( type )
	{
		case FRAG_5K:
		{
			tick_key =					KEY_TICK_5KS;
			tick_stationary_key =		KEY_TICK_SLOW_STATIONARY_5KS;
			stationary_max_range_key =	KEY_SLOW_5K_MAX_RANGE;
			max_time_key =				KEY_5K_MAX_TIME;
			min_hs_key =				KEY_5K_MIN_HEADSHOTS;
			require_sp_kill_key =		KEY_5K_MUST_INCLUDE_SP_KILL;
			sp_kill_extra_time_key =	KEY_5K_SP_KILL_EXTRA_MAX_TIME;
		}
		break;
		case FRAG_4K:
		{
			tick_key =					KEY_TICK_4KS;
			tick_stationary_key =		KEY_TICK_SLOW_STATIONARY_4KS;
			stationary_max_range_key =	KEY_SLOW_4K_MAX_RANGE;
			max_time_key =				KEY_4K_MAX_TIME;
			min_hs_key =				KEY_4K_MIN_HEADSHOTS;
			require_sp_kill_key =		KEY_4K_MUST_INCLUDE_SP_KILL;
			sp_kill_extra_time_key =	KEY_4K_SP_KILL_EXTRA_MAX_TIME;
		}
		break;
		case FRAG_3K:
		{
			tick_key =					KEY_TICK_3KS;
			tick_stationary_key =		KEY_TICK_SLOW_STATIONARY_3KS;
			stationary_max_range_key =	KEY_SLOW_3K_MAX_RANGE;
			max_time_key =				KEY_3K_MAX_TIME;
			min_hs_key =				KEY_3K_MIN_HEADSHOTS;
			require_sp_kill_key =		KEY_3K_MUST_INCLUDE_SP_KILL;
			sp_kill_extra_time_key =	KEY_3K_SP_KILL_EXTRA_MAX_TIME;
		}
		break;
		default:
			return false;
	}


	// ===== Check the settings =========================================

	// Tick key
	if( weapon_settings && weapon_settings->find( tick_key ) != weapon_settings->end() )
	{
		if( !(*weapon_settings)[ tick_key ].m_bool )
			return false;
	}
	else if( category_settings.find( tick_key ) != category_settings.end() )
	{
		if( !category_settings[ tick_key ].m_bool )
			return false;
	}
	else
	{
		if( !general_settings[ tick_key ].m_bool )
			return false;
	}

	// Tick stationary key
	bool bTickStationary = false;
	bool bIsStationary = false;

	if( weapon_settings && weapon_settings->find( tick_stationary_key ) != weapon_settings->end() )
	{
		bTickStationary = (*weapon_settings)[ tick_stationary_key ].m_bool;
	}
	else if( category_settings.find( tick_stationary_key ) != category_settings.end() )
	{
		bTickStationary = category_settings[ tick_stationary_key ].m_bool;
	}
	else
	{
		bTickStationary = general_settings[ tick_stationary_key ].m_bool;
	}

	if( bTickStationary )
	{
		if( weapon_settings && weapon_settings->find( stationary_max_range_key ) != weapon_settings->end() )
		{
			if( (*weapon_settings)[ stationary_max_range_key ].m_float >= farthest_distance )
				bIsStationary = true;
		}
		else if( category_settings.find( stationary_max_range_key ) != category_settings.end() )
		{
			if( category_settings[ stationary_max_range_key ].m_float >= farthest_distance )
				bIsStationary = true;
		}
		else
		{
			if( general_settings[ stationary_max_range_key ].m_float >= farthest_distance )
				bIsStationary = true;
		}
	}

	// Max time key
	float fMaxTime = 0.f;

	if( weapon_settings && weapon_settings->find( max_time_key ) != weapon_settings->end() )
	{
		fMaxTime = (*weapon_settings)[ max_time_key ].m_float;
	}
	else if( category_settings.find( max_time_key ) != category_settings.end() )
	{
		fMaxTime = category_settings[ max_time_key ].m_float;
	}
	else
	{
		fMaxTime = general_settings[ max_time_key ].m_float;
	}

	// Extra max time key
	if( num_special_kills > 0 && fMaxTime >= 0.f )
	{
		if( weapon_settings && weapon_settings->find( sp_kill_extra_time_key ) != weapon_settings->end() )
		{
			fMaxTime += (*weapon_settings)[ sp_kill_extra_time_key ].m_float * num_special_kills;
		}
		else if( category_settings.find( sp_kill_extra_time_key ) != category_settings.end() )
		{
			fMaxTime += category_settings[ sp_kill_extra_time_key ].m_float * num_special_kills;
		}
		else
		{
			fMaxTime += general_settings[ sp_kill_extra_time_key ].m_float * num_special_kills;
		}
	}

	if( fMaxTime >= 0.f && frag_time > fMaxTime )
	{
		if( !bTickStationary || !bIsStationary )
			return false;
	}
	else
	{
		// Frags that are fast enough to be ticked without being stationary don't get the stationary tag
		bIsStationary = false;
	}

	// Min headshots and special kills check
	if( weaponCategory != CATEGORY_KNIFE && weaponCategory != CATEGORY_GRENADE )
	{
		if( weapon_settings && weapon_settings->find( min_hs_key ) != weapon_settings->end() )
		{
			if( headshots < (*weapon_settings)[ min_hs_key ].m_int )
				return false;
		}
		else if( category_settings.find( min_hs_key ) != category_settings.end() )
		{
			if( headshots < category_settings[ min_hs_key ].m_int )
				return false;
		}
		else
		{
			if( headshots < general_settings[ min_hs_key ].m_int )
				return false;
		}

		if( num_special_kills == 0 )
		{
			if( weapon_settings && weapon_settings->find( require_sp_kill_key ) != weapon_settings->end() )
			{
				if( (*weapon_settings)[ require_sp_kill_key ].m_bool )
					return false;
			}
			else if( category_settings.find( require_sp_kill_key ) != category_settings.end() )
			{
				if( category_settings[ require_sp_kill_key ].m_bool )
					return false;
			}
			else
			{
				if( general_settings[ require_sp_kill_key ].m_bool )
					return false;
			}
		}
	}

	outIsStationary = bIsStationary;

	return true;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickFrag( Key tick_key )
{
	if( m_fcats.weapon_settings && m_fcats.weapon_settings->find( tick_key ) != m_fcats.weapon_settings->end() )
	{
		if( (*m_fcats.weapon_settings)[ tick_key ].m_bool )
			return true;
	}
	else if( m_fcats.category_settings->find( tick_key ) != m_fcats.category_settings->end() )
	{
		if( (*m_fcats.category_settings)[ tick_key ].m_bool )
			return true;
	}
	else
	{
		if( GetGeneralSettings()[ tick_key ].m_bool )
			return true;
	}

	return false;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickFrag( Key tick_key, Key only_hs_key, bool bIsHeadshot )
{
	bool bShouldTick = ShouldTickFrag( tick_key );

	if( bShouldTick && !bIsHeadshot )
	{
		if( m_fcats.weapon_settings && m_fcats.weapon_settings->find( only_hs_key ) != m_fcats.weapon_settings->end() )
		{
			if( (*m_fcats.weapon_settings)[ only_hs_key ].m_bool )
				bShouldTick = false;
		}
		else if( m_fcats.category_settings->find( only_hs_key ) != m_fcats.category_settings->end() )
		{
			if( (*m_fcats.category_settings)[ only_hs_key ].m_bool )
				bShouldTick = false;
		}
		else
		{
			if( GetGeneralSettings()[ only_hs_key ].m_bool )
				bShouldTick = false;
		}
	}

	return bShouldTick;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickFrag( Key tick_key, Key min_dist_key, Key min_dist_hs_mod_key, Key min_dist_wb_mod_key, float fDistance, bool bIsHeadshot, bool bIsWallbang )
{
	bool bShouldTick = ShouldTickFrag( tick_key );

	if( bShouldTick )
	{
		float fMinDist;
		float fHsMod;
		float fWbMod;

		WeaponSettingsField &general_settings = GetGeneralSettings();
		bool findW = m_fcats.weapon_settings != nullptr;

		if( findW && m_fcats.weapon_settings->find( min_dist_key ) != m_fcats.weapon_settings->end() )
		{
			fMinDist = (*m_fcats.weapon_settings)[ min_dist_key ].m_float;
		}
		else if( m_fcats.category_settings->find( min_dist_key ) != m_fcats.category_settings->end() )
		{
			fMinDist = (*m_fcats.category_settings)[ min_dist_key ].m_float;
		}
		else
		{
			fMinDist = general_settings[ min_dist_key ].m_float;
		}

		if( bIsHeadshot )
		{
			if( findW && m_fcats.weapon_settings->find( min_dist_hs_mod_key ) != m_fcats.weapon_settings->end() )
			{
				fHsMod = (*m_fcats.weapon_settings)[ min_dist_hs_mod_key ].m_float;
			}
			else if( m_fcats.category_settings->find( min_dist_hs_mod_key ) != m_fcats.category_settings->end() )
			{
				fHsMod = (*m_fcats.category_settings)[ min_dist_hs_mod_key ].m_float;
			}
			else
			{
				fHsMod = general_settings[ min_dist_hs_mod_key ].m_float;
			}

			fMinDist *= fHsMod;
		}

		if( bIsWallbang )
		{
			if( findW && m_fcats.weapon_settings->find( min_dist_wb_mod_key ) != m_fcats.weapon_settings->end() )
			{
				fWbMod = (*m_fcats.weapon_settings)[ min_dist_wb_mod_key ].m_float;
			}
			else if( m_fcats.category_settings->find( min_dist_wb_mod_key ) != m_fcats.category_settings->end() )
			{
				fWbMod = (*m_fcats.category_settings)[ min_dist_wb_mod_key ].m_float;
			}
			else
			{
				fWbMod = general_settings[ min_dist_wb_mod_key ].m_float;
			}

			fMinDist *= fWbMod;
		}

		if( fDistance < fMinDist )
			bShouldTick = false;
	}

	return bShouldTick;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickCollat( CSWeaponCategory category, Key tick_key, Key min_hs_key, short headshots, Key ignore_min_hs_key, bool bMayIgnoreMinHs )
{
	bool bShouldTick = ShouldTickFrag( tick_key );

	if( bShouldTick && category != CATEGORY_GRENADE )
	{
		// Check if we can ignore minimum headshots
		if( bMayIgnoreMinHs )
		{
			if( m_fcats.weapon_settings && m_fcats.weapon_settings->find( ignore_min_hs_key ) != m_fcats.weapon_settings->end() )
			{
				if( (*m_fcats.weapon_settings)[ ignore_min_hs_key ].m_bool )
					return bShouldTick;
			}
			else if( m_fcats.category_settings->find( ignore_min_hs_key ) != m_fcats.category_settings->end() )
			{
				if( (*m_fcats.category_settings)[ ignore_min_hs_key ].m_bool )
					return bShouldTick;
			}
			else
			{
				if( GetGeneralSettings()[ ignore_min_hs_key ].m_bool )
					return bShouldTick;
			}
		}

		// Minimum headshots check
		if( m_fcats.weapon_settings && m_fcats.weapon_settings->find( min_hs_key ) != m_fcats.weapon_settings->end() )
		{
			if( (*m_fcats.weapon_settings)[ min_hs_key ].m_int > headshots )
				bShouldTick = false;
		}
		else if( m_fcats.category_settings->find( min_hs_key ) != m_fcats.category_settings->end() )
		{
			if( (*m_fcats.category_settings)[ min_hs_key ].m_int > headshots )
				bShouldTick = false;
		}
		else
		{
			if( GetGeneralSettings()[ min_hs_key ].m_int > headshots )
				bShouldTick = false;
		}
	}

	return bShouldTick;
}

// =====================================================================================================================================================================

bool SettingsManager::WallbangIsCloseToAnotherWallbang( CSWeaponID weapon, float time_to_closest_wb )
{
	bool bWeaponFound = m_weaponSettings.find( weapon ) != m_weaponSettings.end();

	CSWeaponCategory category = GetWeaponCategory( weapon );
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];
	WeaponSettingsField &general_settings = GetGeneralSettings();

	// Check if we can skip the second kill check and just tick this as a singular wallbang
	if( bWeaponFound && m_weaponSettings[ weapon ].find( KEY_WALLBANG_REQUIRE_TWO ) != m_weaponSettings[ weapon ].end() )
	{
		if( !m_weaponSettings[ weapon ][ KEY_WALLBANG_REQUIRE_TWO ].m_bool )
			return true;
	}
	else if( category_settings.find( KEY_WALLBANG_REQUIRE_TWO ) != category_settings.end() )
	{
		if( !category_settings[ KEY_WALLBANG_REQUIRE_TWO ].m_bool )
			return true;
	}
	else
	{
		if( !general_settings[ KEY_WALLBANG_REQUIRE_TWO ].m_bool )
			return true;
	}

	// Another wallbang required - check if there is a suitable one

	if( time_to_closest_wb < 0.f )
		return false;

	float max_deltatime;

	if( bWeaponFound && m_weaponSettings[ weapon ].find( KEY_WALLBANG_ANOTHER_WB_MAX_DT ) != m_weaponSettings[ weapon ].end() )
	{
		max_deltatime = m_weaponSettings[ weapon ][ KEY_WALLBANG_ANOTHER_WB_MAX_DT ].m_float;
	}
	else if( category_settings.find( KEY_WALLBANG_ANOTHER_WB_MAX_DT ) != category_settings.end() )
	{
		max_deltatime = category_settings[ KEY_WALLBANG_ANOTHER_WB_MAX_DT ].m_float;
	}
	else
	{
		max_deltatime = general_settings[ KEY_WALLBANG_ANOTHER_WB_MAX_DT ].m_float;
	}

	return time_to_closest_wb <= max_deltatime;
}

// =====================================================================================================================================================================

bool SettingsManager::JumpshotIsCloseToAnotherJumpshot( CSWeaponID weapon, float time_to_closest_js )
{
	// Is there another jumpshot at all?
	if( time_to_closest_js < 0.f )
		return false;

	bool bWeaponFound = m_weaponSettings.find( weapon ) != m_weaponSettings.end();

	CSWeaponCategory category = GetWeaponCategory( weapon );
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];
	WeaponSettingsField &general_settings = GetGeneralSettings();

	// Check if multiple jumpshots are supposed to be ticked
	if( bWeaponFound && m_weaponSettings[ weapon ].find( KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE ) != m_weaponSettings[ weapon ].end() )
	{
		if( !m_weaponSettings[ weapon ][ KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE ].m_bool )
			return false;
	}
	else if( category_settings.find( KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE ) != category_settings.end() )
	{
		if( !category_settings[ KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE ].m_bool )
			return false;
	}
	else
	{
		if( !general_settings[ KEY_JUMPSHOT_ALWAYS_TICK_MULTIPLE ].m_bool )
			return false;
	}

	// There is another jumpshot that might suffice - check the delta time

	float max_deltatime;

	if( bWeaponFound && m_weaponSettings[ weapon ].find( KEY_JUMPSHOT_MULTIPLE_MAX_DT ) != m_weaponSettings[ weapon ].end() )
	{
		max_deltatime = m_weaponSettings[ weapon ][ KEY_JUMPSHOT_MULTIPLE_MAX_DT ].m_float;
	}
	else if( category_settings.find( KEY_JUMPSHOT_MULTIPLE_MAX_DT ) != category_settings.end() )
	{
		max_deltatime = category_settings[ KEY_JUMPSHOT_MULTIPLE_MAX_DT ].m_float;
	}
	else
	{
		max_deltatime = general_settings[ KEY_JUMPSHOT_MULTIPLE_MAX_DT ].m_float;
	}

	return time_to_closest_js <= max_deltatime;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickFrag( unsigned short type_flags, CSWeaponID weapon, float distance, short headshots, const frag_delta_times_t &dtimes )
{
	CSWeaponCategory weaponCategory = GetWeaponCategory( weapon );

	m_fcats.weapon_settings = nullptr;
	m_fcats.category_settings = &m_weaponSettings[ weaponCategory ];

	if( m_weaponSettings.find( weapon ) != m_weaponSettings.end() )
	{
		m_fcats.weapon_settings = &m_weaponSettings[ weapon ];
	}

	bool bHeadshot = headshots > 0;
	bool bWallbang = (type_flags & FL_KILL_WALLBANG) != 0;
	bool bHasSpecialFlags = (type_flags & ~MASK_COLLATS) != 0;

	if( type_flags & FL_KILL_DOUBLE )
	{
		if( ShouldTickCollat( weaponCategory, KEY_TICK_DOUBLES, KEY_DOUBLE_MIN_HEADSHOTS, headshots, KEY_SP_DOUBLE_IGNORES_MIN_HS, bHasSpecialFlags ) )
			return true;
	}
	
	if( type_flags & FL_KILL_TRIPLE )
	{
		if( ShouldTickCollat( weaponCategory, KEY_TICK_TRIPLES, KEY_TRIPLE_MIN_HEADSHOTS, headshots, KEY_SP_TRIPLE_IGNORES_MIN_HS, bHasSpecialFlags ) )
			return true;
	}
	
	if( type_flags & FL_KILL_QUADRO )
	{
		if( ShouldTickCollat( weaponCategory, KEY_TICK_QUADROS, KEY_QUADRO_MIN_HEADSHOTS, headshots, KEY_SP_QUADRO_IGNORES_MIN_HS, bHasSpecialFlags ) )
			return true;
	}
	
	if( type_flags & FL_KILL_PENTA )
	{
		if( ShouldTickCollat( weaponCategory, KEY_TICK_PENTAS, KEY_PENTA_MIN_HEADSHOTS, headshots, KEY_SP_PENTA_IGNORES_MIN_HS, bHasSpecialFlags ) )
			return true;
	}

	if( type_flags & FL_KILL_FLASHKILL || type_flags & FL_KILL_SMOKEKILL )
	{
		if( ShouldTickFrag( KEY_TICK_FLASH_SMOKE_KILLS ) )
			return true;
	}

	if( type_flags & FL_KILL_FLICKSHOT )
	{
		if( distance >= GetFlickshotMinDistanceForWeapon( weapon ) )
		{
			if( ShouldTickFrag( KEY_TICK_FLICKSHOTS, KEY_FLICKSHOT_HEADSHOT_ONLY, bHeadshot ) )
				return true;
		}
	}

	if( type_flags & FL_KILL_WALLBANG )
	{
		if( ShouldTickFrag( KEY_TICK_WALLBANGS, KEY_WALLBANG_HEADSHOT_ONLY, bHeadshot )
		&& WallbangIsCloseToAnotherWallbang( weapon, dtimes.time_to_closest_wallbang ) )
		{
			return true;
		}
	}

	if( (type_flags & FL_KILL_JUMPSHOT) || (type_flags & FL_KILL_LADDERSHOT) )
	{
		if( ShouldTickFrag( KEY_TICK_JUMPSHOTS, KEY_JUMPSHOT_MIN_DISTANCE, KEY_JUMPSHOT_MIN_DISTANCE_HS_MOD, KEY_JUMPSHOT_MIN_DISTANCE_WB_MOD, distance, bHeadshot, bWallbang ) )
			return true;

		if( JumpshotIsCloseToAnotherJumpshot( weapon, dtimes.time_to_closest_jumpshot ) && ShouldTickFrag( KEY_TICK_JUMPSHOTS ) )
			return true;
	}

	if( type_flags & FL_KILL_NOSCOPE )
	{
		if( ShouldTickFrag( KEY_TICK_NOSCOPES, KEY_NOSCOPE_MIN_DISTANCE, KEY_NOSCOPE_MIN_DISTANCE_HS_MOD, KEY_NOSCOPE_MIN_DISTANCE_WB_MOD, distance, bHeadshot, bWallbang ) )
			return true;
	}

	return false;
}

// =====================================================================================================================================================================

bool SettingsManager::BatchProcessingEnabled( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_ENABLE_BATCH_PROCESSING ].m_bool;
}

// =====================================================================================================================================================================

void SettingsManager::DisableBatchProcessing( void )
{
	m_weaponSettings[ CATEGORY_GENERAL ][ KEY_ENABLE_BATCH_PROCESSING ].m_bool = false;
}

// =====================================================================================================================================================================

bool SettingsManager::DumpToFileEnabled( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_DUMP_TO_FILE ].m_bool;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldWriteOutputToDemoDirectory( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_WRITE_FILE_TO_DEMO_DIR ].m_bool;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickFragsVsBots( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_TICK_FRAGS_VS_BOTS ].m_bool;
}

// =====================================================================================================================================================================

bool SettingsManager::ShouldTickFragsByBots( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_TICK_FRAGS_BY_BOTS ].m_bool;
}

// =====================================================================================================================================================================

int SettingsManager::GetMaxFlickshotDuration( void )
{
	return m_iMaxFlickDuration;
}

// =====================================================================================================================================================================

int SettingsManager::GetFlickshotDurationForWeapon( CSWeaponID weapon )
{
	ReturnSettingsValueForWeapon( KEY_FLICKSHOT_MAX_DURATION, int );
}

// =====================================================================================================================================================================

float SettingsManager::GetFlickshotMinDistanceForWeapon( CSWeaponID weapon )
{
	ReturnSettingsValueForWeapon( KEY_FLICKSHOT_MIN_DISTANCE, float );
}

// =====================================================================================================================================================================

float SettingsManager::GetFlickshotMinAngleModForWeapon( CSWeaponID weapon )
{
	ReturnSettingsValueForWeapon( KEY_FLICKSHOT_MIN_ANGLE_MOD, float );
}

// =====================================================================================================================================================================

float SettingsManager::GetMinPostKillAirTimeForWeapon( CSWeaponID weapon )
{
	ReturnSettingsValueForWeapon( KEY_JUMPSHOT_MIN_POSTKILL_AIR_TIME, float );
}

// =====================================================================================================================================================================

SettingsCategory SettingsManager::GetCategoryByName( const char *szCategoryName )
{
	// Check if it's a weapon category first
	if( !_stricmp( szCategoryName, CAT_NAME_GENERAL ) )
		return CATEGORY_GENERAL;
	if( !_stricmp( szCategoryName, CAT_NAME_KNIFE ) )
		return CATEGORY_KNIFE;
	if( !_stricmp( szCategoryName, CAT_NAME_PISTOLS ) )
		return CATEGORY_PISTOL;
	if( !_stricmp( szCategoryName, CAT_NAME_SHOTGUNS ) )
		return CATEGORY_SHOTGUN;
	if( !_stricmp( szCategoryName, CAT_NAME_SMGS ) )
		return CATEGORY_SMG;
	if( !_stricmp( szCategoryName, CAT_NAME_RIFLES ) )
		return CATEGORY_RIFLE;
	if( !_stricmp( szCategoryName, CAT_NAME_SNIPERS ) )
		return CATEGORY_SNIPER;
	if( !_stricmp( szCategoryName, CAT_NAME_AUTOSNIPERS ) )
		return CATEGORY_AUTOSNIPER;
	if( !_stricmp( szCategoryName, CAT_NAME_GRENADES ) )
		return CATEGORY_GRENADE;

	// Check if it's a weapon next
	CSWeaponID weaponID = AliasToWeaponID( szCategoryName );

	if( weaponID == WEAPON_C4 || weaponID >= WEAPON_WORLD )
		return CATEGORY_INVALID;

	if( weaponID != WEAPON_NONE )
		return weaponID;

	// Some weapons have different names - check them last
	if( !_stricmp( szCategoryName, "mp5" ) )
		return WEAPON_MP5NAVY;
	if( !_stricmp( szCategoryName, "elites" )
	|| !_stricmp( szCategoryName, "dualies" )
	|| !_stricmp( szCategoryName, "dual elites" ) )
		return WEAPON_ELITE;

	return CATEGORY_INVALID;
}

// =====================================================================================================================================================================

SettingsManager::WeaponSettingsField &SettingsManager::GetGeneralSettings( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ];
}

// =====================================================================================================================================================================