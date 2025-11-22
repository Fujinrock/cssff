#pragma once

#include <map>
#include <string>

enum MultiKillFragType;
enum CSWeaponID;
enum CSWeaponCategory;
typedef int SettingsCategory;
typedef const char *Key;

class SettingsManager
{
public:
	static SettingsManager *Instance( void );

	void LoadSettings( const char *szSettingsFile, bool bBatchDirSupplied );

	// Checks if the multi-kill frag should be ticked for the given weapons
	// Also checks if the frag is fast enough to be ticked or stationary
	bool ShouldTickFrag( MultiKillFragType type, CSWeaponID *pWeapons, short num_weapons, float frag_time, float farthest_distance, short headshots, bool contains_sp_kills, bool &outIsStationary );

	// Check if the flags indicate any frags that should be ticked for the given weapon
	// Also checks for minimum distance, minimum headshots etc.
	bool ShouldTickFrag( unsigned short type_flags, CSWeaponID weapon, float distance, short headshots, float time_to_closest_kill );

	bool BatchProcessingEnabled( void );
	void DisableBatchProcessing( void );

	bool DumpToFileEnabled( void );
	bool WriteOutputToDemoDirectory( void );

	bool ShouldTickFragsVsBots( void );

	// This returns the longest flick duration in milliseconds across all weapons and categories
	int GetMaxFlickshotDuration( void );
	int GetFlickshotDurationForWeapon( CSWeaponID weapon );

	float GetMinPostKillAirTimeForWeapon( CSWeaponID weapon );

private:
	SettingsManager( void );
	SettingsManager( const SettingsManager & );
	SettingsManager &operator=( const SettingsManager & );

	SettingsCategory GetCategoryByName( const char *szCategoryName );

	bool ShouldTickFrag( Key tick_key );
	bool ShouldTickFrag( Key tick_key, Key only_hs_key, bool bIsHeadshot );
	bool ShouldTickFrag( Key tick_key, Key min_dist_key, Key min_dist_hs_mod_key, Key min_dist_wb_mod_key, float fDistance, bool bIsHeadshot, bool bIsWallbang );
	bool ShouldTickCollat( CSWeaponCategory category, Key tick_key, Key min_hs_key, short headshots );
	bool WallbangIsCloseToAnotherKill( CSWeaponID weapon, float time_to_closest_kill );

	union setting_value
	{
		int m_int;
		float m_float;
		bool m_bool;
	};

	typedef std::map< std::string, setting_value > WeaponSettingsField;

	// A hash map of the settings per weapon or weapon category
	// General settings are in CATEGORY_GENERAL (CATEGORY_NONE)
	std::map< SettingsCategory, WeaponSettingsField > m_weaponSettings;

	WeaponSettingsField &GetGeneralSettings( void );

	// Settings categories that will be used inside helper functions in ShouldTickFrag
	struct FragCategories
	{
		WeaponSettingsField *weapon_settings;	///< Settings for the specific weapon, might be NULL
		WeaponSettingsField *category_settings; ///< Settings for the weapon category, never NULL
	}
	m_fcats;

	int m_iMaxFlickDuration;

	bool m_bSettingsLoaded;
};

#define Settings	SettingsManager::Instance