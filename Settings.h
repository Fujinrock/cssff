#pragma once

#include <map>
#include <string>

enum MultiKillFragType;
enum CSWeaponCategory;

class SettingsManager
{
public:
	static SettingsManager *Instance( void );

	void LoadSettings( const char *szSettingsFile = nullptr );

	// Checks if the multi-kill frag should be ticked in the given category
	// Also checks if the frag is fast enough to be ticked
	bool ShouldTickFrag( MultiKillFragType type, CSWeaponCategory category, float frag_time, float farthest_distance, short headshots, bool contains_sp_kills );

	// Check if the flags indicate any frags that should be ticked in the given category
	// Also checks for minimum distance, minimum headshots etc.
	bool ShouldTickFrag( unsigned short type_flags, CSWeaponCategory category, float distance, short headshots, float time_to_closest_kill );

	bool BatchProcessingEnabled( void );
	void DisableBatchProcessing( void );

	bool DumpToFileEnabled( void );
	bool WriteOutputToDemoDirectory( void );

	bool ShouldTickFragsVsBots( void );

	// This returns the longest flick duration across all categories in milliseconds
	int GetMaxFlickshotDuration( void );
	int GetFlickshotDurationForCategory( CSWeaponCategory category );

	float GetMinPostKillAirTimeForCategory( CSWeaponCategory category );

private:
	SettingsManager( void );
	SettingsManager( const SettingsManager & );
	SettingsManager &operator=( const SettingsManager & );

	CSWeaponCategory GetCategoryByName( const char *szCategoryName );

	bool WallbangIsCloseToAnotherKill( CSWeaponCategory category, float time_to_closest_kill );

	union setting_value
	{
		int m_int;
		float m_float;
		bool m_bool;
	};

	typedef std::map< std::string, setting_value > WeaponSettingsField;

	// A hash map of the settings per weapon category
	// General settings are in CATEGORY_GENERAL (CATEGORY_NONE)
	std::map< CSWeaponCategory, WeaponSettingsField > m_weaponSettings;

	WeaponSettingsField &GetGeneralSettings( void );

	int m_iMaxFlickDuration;

	bool m_bSettingsLoaded;
};

#define Settings	SettingsManager::Instance