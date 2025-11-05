#include "Settings.h"
#include "Common.h"
#include "Weapons.h"
#include "Frag.h"
#include <fstream>
#include <vector>

extern std::string g_ProgramDirectory;

// Keys used in reading the settings file and in finding settings per weapon category
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
#define KEY_TICK_MIDAIR_KILLS					"tick_mid_air_kills"
#define KEY_TICK_NOSCOPES						"tick_noscopes"
#define KEY_TICK_FLICKSHOTS						"tick_flickshots"
#define KEY_TICK_WALLBANGS						"tick_wallbangs"
#define KEY_TICK_FRAGS_VS_BOTS					"tick_frags_vs_bots"
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
#define KEY_DOUBLE_MIN_HEADSHOTS				"double_min_headshots"
#define KEY_TRIPLE_MIN_HEADSHOTS				"triple_min_headshots"
#define KEY_QUADRO_MIN_HEADSHOTS				"quadro_min_headshots"
#define KEY_PENTA_MIN_HEADSHOTS					"penta_min_headshots"
#define KEY_NOSCOPE_MIN_DISTANCE				"noscope_min_distance"
#define KEY_NOSCOPE_MIN_DISTANCE_HS_MOD			"noscope_min_distance_hs_modifier"
#define KEY_NOSCOPE_MIN_DISTANCE_WB_MOD			"noscope_min_distance_wb_modifier"
#define KEY_MIDAIR_MIN_POSTKILL_AIR_TIME		"mid_air_min_post_kill_air_time"
#define KEY_MIDAIR_MIN_DISTANCE					"mid_air_min_distance"
#define KEY_MIDAIR_MIN_DISTANCE_HS_MOD			"mid_air_min_distance_hs_modifier"
#define KEY_MIDAIR_MIN_DISTANCE_WB_MOD			"mid_air_min_distance_wb_modifier"
#define KEY_WALLBANG_HEADSHOT_ONLY				"wallbang_headshot_only"
#define KEY_WALLBANG_REQUIRE_ANOTHER_KILL		"wallbang_require_another_kill"
#define KEY_WALLBANG_ANOTHER_KILL_MAX_DT		"wallbang_another_kill_max_delta_time"
#define KEY_FLICKSHOT_MAX_DURATION				"flickshot_max_duration"
#define KEY_FLICKSHOT_HEADSHOT_ONLY				"flickshot_headshot_only"

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
	else void(0);

#define SetKeyValueFloat( key, value )\
	try { float __fValue = stof( value );\
			for(size_t i=0;i<current_categories.size();++i)\
				m_weaponSettings[ current_categories[i] ][ key ].m_float = __fValue; }\
		catch( ... ) { continue; }

#define SetKeyValueInt( key, value )\
	try { int __nValue = stoi( value );\
			for(size_t i=0;i<current_categories.size();++i)\
				m_weaponSettings[ current_categories[i] ][ key ].m_int = __nValue; }\
		catch( ... ) { continue; }

// Accessor to the singleton (use the Settings macro to get it neatly)
SettingsManager *SettingsManager::Instance( void )
{
	static SettingsManager settings;

	return &settings;
}

SettingsManager::SettingsManager()
{
	m_bSettingsLoaded = false;

	// Populate the weapon settings map with the categories, but not the actual settings
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
	general_settings[ KEY_TICK_MIDAIR_KILLS ].m_bool = true;
	general_settings[ KEY_TICK_NOSCOPES ].m_bool = true;
	general_settings[ KEY_TICK_FLICKSHOTS ].m_bool = true;
	general_settings[ KEY_TICK_WALLBANGS ].m_bool = true;
	general_settings[ KEY_WALLBANG_HEADSHOT_ONLY ].m_bool = true;
	general_settings[ KEY_WALLBANG_REQUIRE_ANOTHER_KILL ].m_bool = true;
	general_settings[ KEY_WALLBANG_ANOTHER_KILL_MAX_DT ].m_float = 2.0;
	general_settings[ KEY_TICK_FRAGS_VS_BOTS ].m_bool = false;
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
	general_settings[ KEY_DOUBLE_MIN_HEADSHOTS ].m_int = 1;
	general_settings[ KEY_TRIPLE_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_QUADRO_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_PENTA_MIN_HEADSHOTS ].m_int = 0;
	general_settings[ KEY_NOSCOPE_MIN_DISTANCE ].m_float = 1000.f;
	general_settings[ KEY_NOSCOPE_MIN_DISTANCE_HS_MOD ].m_float = 0.5f;
	general_settings[ KEY_NOSCOPE_MIN_DISTANCE_WB_MOD ].m_float = 0.5f;
	general_settings[ KEY_MIDAIR_MIN_POSTKILL_AIR_TIME ].m_float = 0.1f;
	general_settings[ KEY_MIDAIR_MIN_DISTANCE ].m_float = 1000.f;
	general_settings[ KEY_MIDAIR_MIN_DISTANCE_HS_MOD ].m_float = 0.5f;
	general_settings[ KEY_MIDAIR_MIN_DISTANCE_WB_MOD ].m_float = 0.5f;
	general_settings[ KEY_FLICKSHOT_MAX_DURATION ].m_int = 150;
	general_settings[ KEY_FLICKSHOT_HEADSHOT_ONLY ].m_bool = false;

	m_iMaxFlickDuration = general_settings[ KEY_FLICKSHOT_MAX_DURATION ].m_int;
}

// This is where the settings for all categories are read from the settings file
void SettingsManager::LoadSettings( const char *szSettingsFile )
{
	using namespace std;

	if( m_bSettingsLoaded )
		return;

	std::string sDefaultConfigPath;

	// Use default file if no file was specified
	if( !szSettingsFile )
	{
		sDefaultConfigPath = g_ProgramDirectory + DEFAULT_SETTINGS_FILE;
		szSettingsFile = sDefaultConfigPath.c_str();
	}

	ifstream file( szSettingsFile );

	if( !file.is_open() )
	{
		string filename = szSettingsFile;
		RemoveFileNameFolders( filename );

		printf( "Warning: Could not open settings file \"%s\" - using built-in default values!\n", filename.c_str() );

		return;
	}

	// Allow chaining multiple categories together
	std::vector< CSWeaponCategory > current_categories;
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

				CSWeaponCategory category = GetCategoryByName( line.c_str() );

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
		else if( key == KEY_TICK_MIDAIR_KILLS )
		{
			SetKeyValueBool( KEY_TICK_MIDAIR_KILLS, value )
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
		else if( key == KEY_WALLBANG_REQUIRE_ANOTHER_KILL )
		{
			SetKeyValueBool( KEY_WALLBANG_REQUIRE_ANOTHER_KILL, value )
		}
		else if( key == KEY_WALLBANG_ANOTHER_KILL_MAX_DT )
		{
			SetKeyValueFloat( KEY_WALLBANG_ANOTHER_KILL_MAX_DT, value )
		}
		else if( key == KEY_TICK_FRAGS_VS_BOTS )
		{
			SetKeyValueBool( KEY_TICK_FRAGS_VS_BOTS, value )
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
		else if( key == KEY_MIDAIR_MIN_POSTKILL_AIR_TIME )
		{
			SetKeyValueFloat( KEY_MIDAIR_MIN_POSTKILL_AIR_TIME, value )
		}
		else if( key == KEY_MIDAIR_MIN_DISTANCE )
		{
			SetKeyValueFloat( KEY_MIDAIR_MIN_DISTANCE, value )
		}
		else if( key == KEY_MIDAIR_MIN_DISTANCE_HS_MOD )
		{
			SetKeyValueFloat( KEY_MIDAIR_MIN_DISTANCE_HS_MOD, value )
		}
		else if( key == KEY_MIDAIR_MIN_DISTANCE_WB_MOD )
		{
			SetKeyValueFloat( KEY_MIDAIR_MIN_DISTANCE_WB_MOD, value )
		}
		else if( key == KEY_FLICKSHOT_MAX_DURATION )
		{
			SetKeyValueInt( KEY_FLICKSHOT_MAX_DURATION, value )
		}
		else if( key == KEY_FLICKSHOT_HEADSHOT_ONLY )
		{
			SetKeyValueBool( KEY_FLICKSHOT_HEADSHOT_ONLY, value )
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

	for( auto it = m_weaponSettings.begin(); it != m_weaponSettings.end(); ++it )
	{
		if( it->second.find( KEY_FLICKSHOT_MAX_DURATION ) != it->second.end() )
		{
			int duration = it->second[ KEY_FLICKSHOT_MAX_DURATION ].m_int;

			if( duration > iFlickshotAbsoluteMaxDuration )
			{
				duration = iFlickshotAbsoluteMaxDuration;
				it->second[ KEY_FLICKSHOT_MAX_DURATION ].m_int = duration;
			}

			if( duration > m_iMaxFlickDuration )
				m_iMaxFlickDuration = duration;
		}
	}

	m_bSettingsLoaded = true;
}

// TODO: Get rid of the horrifying macro-hell below by replacing them with actual functions

// Macro used in ShouldTickFrag with multi-kill frags
#define DoMultikillFragCheck( tick_key, tick_stationary_key, stationary_max_dist_key, time_key, min_headshots_key, sp_kill_key )\
	if( category_settings.find( tick_key ) != category_settings.end() ){\
		if( !category_settings[ tick_key ].m_bool )\
			return false;\
	}\
	else{\
		if( !general_settings[ tick_key ].m_bool )\
			return false;\
	}\
	bool __bCheckTime = true;\
	bool __bCheckDistance = false;\
	if( category_settings.find( tick_stationary_key ) != category_settings.end() ){\
		__bCheckDistance = category_settings[ tick_stationary_key ].m_bool;\
	}\
	else{\
		__bCheckDistance = general_settings[ tick_stationary_key ].m_bool;\
	}\
	if( __bCheckDistance ){\
		if( category_settings.find( stationary_max_dist_key ) != category_settings.end() ){\
			if( category_settings[ stationary_max_dist_key ].m_float >= farthest_distance )\
				__bCheckTime = false;\
		}\
		else{\
			if( general_settings[ stationary_max_dist_key ].m_float >= farthest_distance )\
				__bCheckTime = false;\
		}\
	}\
	if( __bCheckTime ){\
		if( category_settings.find( time_key ) != category_settings.end() ){\
			if( category_settings[ time_key ].m_float < frag_time )\
				return false;\
		}\
		else{\
			if( general_settings[ time_key ].m_float < frag_time )\
				return false;\
		}\
	}\
	if( category != CATEGORY_KNIFE && category != CATEGORY_GRENADE ){\
		if( category_settings.find( min_headshots_key ) != category_settings.end() ){\
			if( headshots < category_settings[ min_headshots_key ].m_int )\
				return false;\
		}\
		else{\
			if( headshots < general_settings[ min_headshots_key ].m_int )\
				return false;\
		}\
		if( !contains_sp_kills ){\
			if( category_settings.find( sp_kill_key ) != category_settings.end() ){\
				if( category_settings[ sp_kill_key ].m_bool )\
					return false;\
			}\
			else{\
				if( general_settings[ sp_kill_key ].m_bool )\
					return false;\
			}\
		}\
	}\
	return true

bool SettingsManager::ShouldTickFrag( MultiKillFragType type, CSWeaponCategory category, float frag_time, float farthest_distance, short headshots, bool contains_sp_kills )
{
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];
	WeaponSettingsField &general_settings = GetGeneralSettings();

	switch( type )
	{
		case FRAG_5K:
		{
			DoMultikillFragCheck( KEY_TICK_5KS, KEY_TICK_SLOW_STATIONARY_5KS, KEY_SLOW_5K_MAX_RANGE, KEY_5K_MAX_TIME, KEY_5K_MIN_HEADSHOTS, KEY_5K_MUST_INCLUDE_SP_KILL );
		}
		break;
		case FRAG_4K:
		{
			DoMultikillFragCheck( KEY_TICK_4KS, KEY_TICK_SLOW_STATIONARY_4KS, KEY_SLOW_4K_MAX_RANGE, KEY_4K_MAX_TIME, KEY_4K_MIN_HEADSHOTS, KEY_4K_MUST_INCLUDE_SP_KILL );
		}
		break;
		case FRAG_3K:
		{
			DoMultikillFragCheck( KEY_TICK_3KS, KEY_TICK_SLOW_STATIONARY_3KS, KEY_SLOW_3K_MAX_RANGE, KEY_3K_MAX_TIME, KEY_3K_MIN_HEADSHOTS, KEY_3K_MUST_INCLUDE_SP_KILL );
		}
		break;
	}

	return false;
}

// Macro used in ShouldTickFrag to check if this kind of frag should be ticked at all
#define DoFragCheck( tick_key )\
	if( category_settings.find( tick_key ) != category_settings.end() ){\
		if( category_settings[ tick_key ].m_bool )\
			return true;\
	}\
	else{\
		if( general_settings[ tick_key ].m_bool )\
			return true;\
	}

// Same as DoFragCheck, but also checks for (non)required headshot
#define DoFragCheckWithHeadshot( tick_key, headshot_key, is_headshot )\
	bool __bShouldTick = false;\
	if( category_settings.find( tick_key ) != category_settings.end() ){\
		if( category_settings[ tick_key ].m_bool )\
			__bShouldTick = true;\
	}\
	else{\
		if( general_settings[ tick_key ].m_bool )\
			__bShouldTick = true;\
	}\
	if( __bShouldTick ){\
		if( category_settings.find( headshot_key ) != category_settings.end() ){\
			if( !category_settings[ headshot_key ].m_bool || is_headshot )\
				return true;\
		}\
		else{\
			if( !general_settings[ headshot_key ].m_bool || is_headshot )\
				return true;\
		}\
	}\
	else void(0);

// Same as DoFragCheck, but also checks for minimum required headshots
#define DoFragCheckWithMinHeadshots( tick_key, min_headshots_key, num_headshots )\
	bool __bShouldTick = false;\
	if( category_settings.find( tick_key ) != category_settings.end() ){\
		if( category_settings[ tick_key ].m_bool )\
			__bShouldTick = true;\
	}\
	else{\
		if( general_settings[ tick_key ].m_bool )\
			__bShouldTick = true;\
	}\
	if( __bShouldTick ){\
		if( category_settings.find( min_headshots_key ) != category_settings.end() ){\
			if( category_settings[ min_headshots_key ].m_int <= num_headshots )\
				return true;\
		}\
		else{\
			if( general_settings[ min_headshots_key ].m_int <= num_headshots )\
				return true;\
		}\
	}\
	else void(0);

// Same as DoFragCheck, but also checks minimum distance and scales it by headshot modifier if it's a headshot
#define DoFragCheckWithDistance( tick_key, min_dist_key, hs_mod_key, wb_mod_key, distance, is_headshot, is_wallbang )\
	bool __bShouldTick = false;\
	if( category_settings.find( tick_key ) != category_settings.end() ){\
		if( category_settings[ tick_key ].m_bool )\
			__bShouldTick = true;\
	}\
	else{\
		if( general_settings[ tick_key ].m_bool )\
			__bShouldTick = true;\
	}\
	if( __bShouldTick ){\
		float __fMinDist;\
		float __fHsMod;\
		float __fWbMod;\
		if( category_settings.find( min_dist_key ) != category_settings.end() ){\
			__fMinDist = category_settings[ min_dist_key ].m_float;\
		}\
		else{\
			__fMinDist = general_settings[ min_dist_key ].m_float;\
		}\
		if( is_headshot ){\
			if( category_settings.find( hs_mod_key ) != category_settings.end() ){\
				__fHsMod = category_settings[ hs_mod_key ].m_float;\
			}\
			else{\
				__fHsMod = general_settings[ hs_mod_key ].m_float;\
			}\
			__fMinDist *= __fHsMod;\
		}\
		if( is_wallbang ){\
			if( category_settings.find( wb_mod_key ) != category_settings.end() ){\
				__fWbMod = category_settings[ wb_mod_key ].m_float;\
			}\
			else{\
				__fWbMod = general_settings[ wb_mod_key ].m_float;\
			}\
			__fMinDist *= __fWbMod;\
		}\
		if( distance >= __fMinDist )\
			return true;\
	}\
	else void(0);

// Frag check for collats in particular
#define DoCollatFragCheck( category, tick_key, min_headshots_key, headshots )\
		if( category != CATEGORY_GRENADE ){\
			DoFragCheckWithMinHeadshots( tick_key, min_headshots_key, headshots )\
		}\
		else{\
			DoFragCheck( tick_key )\
		}

bool SettingsManager::WallbangIsCloseToAnotherKill( CSWeaponCategory category, float time_to_closest_kill )
{
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];
	WeaponSettingsField &general_settings = GetGeneralSettings();

	if( category_settings.find( KEY_WALLBANG_REQUIRE_ANOTHER_KILL ) != category_settings.end() )
	{
		if( !category_settings[ KEY_WALLBANG_REQUIRE_ANOTHER_KILL ].m_bool )
			return true;
	}
	else
	{
		if( !general_settings[ KEY_WALLBANG_REQUIRE_ANOTHER_KILL ].m_bool )
			return true;
	}

	float max_deltatime;

	if( category_settings.find( KEY_WALLBANG_ANOTHER_KILL_MAX_DT ) != category_settings.end() )
	{
		max_deltatime = category_settings[ KEY_WALLBANG_ANOTHER_KILL_MAX_DT ].m_float;
	}
	else
	{
		max_deltatime = general_settings[ KEY_WALLBANG_ANOTHER_KILL_MAX_DT ].m_float;
	}

	return time_to_closest_kill <= max_deltatime;
}

bool SettingsManager::ShouldTickFrag( unsigned short type_flags, CSWeaponCategory category, float distance, short headshots, float time_to_closest_kill )
{
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];
	WeaponSettingsField &general_settings = GetGeneralSettings();

	bool bHeadshot = headshots > 0;
	bool bWallbang = (type_flags & FL_KILL_WALLBANG) != 0;

	if( type_flags & FL_KILL_DOUBLE )
	{
		DoCollatFragCheck( category, KEY_TICK_DOUBLES, KEY_DOUBLE_MIN_HEADSHOTS, headshots )
	}

	if( type_flags & FL_KILL_TRIPLE )
	{
		DoCollatFragCheck( category, KEY_TICK_TRIPLES, KEY_TRIPLE_MIN_HEADSHOTS, headshots )
	}

	if( type_flags & FL_KILL_QUADRO )
	{
		DoCollatFragCheck( category, KEY_TICK_QUADROS, KEY_QUADRO_MIN_HEADSHOTS, headshots )
	}

	if( type_flags & FL_KILL_PENTA )
	{
		DoCollatFragCheck( category, KEY_TICK_PENTAS, KEY_PENTA_MIN_HEADSHOTS, headshots )
	}

	if( type_flags & FL_KILL_FLASHKILL || type_flags & FL_KILL_SMOKEKILL )
	{
		DoFragCheck( KEY_TICK_FLASH_SMOKE_KILLS )
	}

	if( type_flags & FL_KILL_FLICKSHOT )
	{
		DoFragCheckWithHeadshot( KEY_TICK_FLICKSHOTS, KEY_FLICKSHOT_HEADSHOT_ONLY, bHeadshot )
	}

	if( type_flags & FL_KILL_WALLBANG )
	{
		if( WallbangIsCloseToAnotherKill( category, time_to_closest_kill ) )
		{
			DoFragCheckWithHeadshot( KEY_TICK_WALLBANGS, KEY_WALLBANG_HEADSHOT_ONLY, bHeadshot )
		}
	}

	if( (type_flags & FL_KILL_MIDAIR) || (type_flags & FL_KILL_LADDERSHOT) )
	{
		DoFragCheckWithDistance( KEY_TICK_MIDAIR_KILLS, KEY_MIDAIR_MIN_DISTANCE, KEY_MIDAIR_MIN_DISTANCE_HS_MOD, KEY_MIDAIR_MIN_DISTANCE_WB_MOD, distance, bHeadshot, bWallbang )
	}

	if( type_flags & FL_KILL_NOSCOPE )
	{	// TODO: add running check
		DoFragCheckWithDistance( KEY_TICK_NOSCOPES, KEY_NOSCOPE_MIN_DISTANCE, KEY_NOSCOPE_MIN_DISTANCE_HS_MOD, KEY_NOSCOPE_MIN_DISTANCE_WB_MOD, distance, bHeadshot, bWallbang )
	}

	return false;
}

bool SettingsManager::BatchProcessingEnabled( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_ENABLE_BATCH_PROCESSING ].m_bool;
}

void SettingsManager::DisableBatchProcessing( void )
{
	m_weaponSettings[ CATEGORY_GENERAL ][ KEY_ENABLE_BATCH_PROCESSING ].m_bool = false;
}

bool SettingsManager::DumpToFileEnabled( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_DUMP_TO_FILE ].m_bool;
}

bool SettingsManager::WriteOutputToDemoDirectory( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_WRITE_FILE_TO_DEMO_DIR ].m_bool;
}

bool SettingsManager::ShouldTickFragsVsBots( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_TICK_FRAGS_VS_BOTS ].m_bool;
}

int SettingsManager::GetMaxFlickshotDuration( void )
{
	return m_iMaxFlickDuration;
}

int SettingsManager::GetFlickshotDurationForCategory( CSWeaponCategory category )
{
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];

	if( category_settings.find( KEY_FLICKSHOT_MAX_DURATION ) != category_settings.end() )
	{
		return category_settings[ KEY_FLICKSHOT_MAX_DURATION ].m_int;
	}
	else
	{
		return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_FLICKSHOT_MAX_DURATION ].m_int;
	}
}

float SettingsManager::GetMinPostKillAirTimeForCategory( CSWeaponCategory category )
{
	WeaponSettingsField &category_settings = m_weaponSettings[ category ];

	if( category_settings.find( KEY_MIDAIR_MIN_POSTKILL_AIR_TIME ) != category_settings.end() )
	{
		return category_settings[ KEY_MIDAIR_MIN_POSTKILL_AIR_TIME ].m_float;
	}
	else
	{
		return m_weaponSettings[ CATEGORY_GENERAL ][ KEY_MIDAIR_MIN_POSTKILL_AIR_TIME ].m_float;
	}
}

CSWeaponCategory SettingsManager::GetCategoryByName( const char *szCategoryName )
{
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

	return CATEGORY_INVALID;
}

SettingsManager::WeaponSettingsField &SettingsManager::GetGeneralSettings( void )
{
	return m_weaponSettings[ CATEGORY_GENERAL ];
}