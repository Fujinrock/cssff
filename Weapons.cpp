#include "Weapons.h"
#include <string>
#include <map>
#include <assert.h>

// =====================================================================================================================================================================

CSWeaponCategory GetWeaponCategory( CSWeaponID weapon )
{
	switch( weapon )
	{
		default: return CATEGORY_NONE;

		case WEAPON_KNIFE:
			return CATEGORY_KNIFE;

		case WEAPON_GLOCK:
		case WEAPON_USP:
		case WEAPON_P228:
		case WEAPON_DEAGLE:
		case WEAPON_FIVESEVEN:
		case WEAPON_ELITE:
			return CATEGORY_PISTOL;

		case WEAPON_M3:
		case WEAPON_XM1014:
			return CATEGORY_SHOTGUN;

		case WEAPON_TMP:
		case WEAPON_MAC10:
		case WEAPON_MP5NAVY:
		case WEAPON_UMP45:
		case WEAPON_P90:
			return CATEGORY_SMG;

		case WEAPON_FAMAS:
		case WEAPON_GALIL:
		case WEAPON_M4A1:
		case WEAPON_AK47:
		case WEAPON_AUG:
		case WEAPON_SG552:
		case WEAPON_M249:
			return CATEGORY_RIFLE;

		case WEAPON_SCOUT:
		case WEAPON_AWP:
			return CATEGORY_SNIPER;

		case WEAPON_SG550:
		case WEAPON_G3SG1:
			return CATEGORY_AUTOSNIPER;

		case WEAPON_HEGRENADE:
		case WEAPON_FLASHBANG:
		case WEAPON_SMOKEGRENADE:
			return CATEGORY_GRENADE;
	}
}

// =====================================================================================================================================================================

CSWeaponCategory GetWeaponCategory( CSWeaponID *pWeapons, int num_weapons )
{
	// This function is a bit hacky in that it relies on the fact that num_weapons == number of kills,
	// because duplicate weapons are not removed before calling this
	std::map< CSWeaponCategory, int > categories;

	for( int i = 0; i < num_weapons; ++i )
	{
		CSWeaponCategory category = GetWeaponCategory( pWeapons[ i ] );

		++categories[ category ];
	}

	assert( categories.size() > 0 );

	if( categories.size() == 1 )
		return categories.begin()->first;

	// If a 4k or 3k has more than 1 category, use the general category
	// also use the general category if a (+)5k has more than 2 categories
	if( num_weapons < 5 || categories.size() > 2 )
		return CATEGORY_GENERAL;

	// If a (+)5k has 2 categories, and the other category has only 1 kill, use the category with more kills
	for( auto it = categories.begin(); it != categories.end(); ++it )
	{
		if( it->second == num_weapons - 1 )
			return it->first;
	}

	// (+)5k with mixed weapons, use the general category
	return CATEGORY_GENERAL;
}

// =====================================================================================================================================================================

CSWeaponID AliasToWeaponID( const char *alias )
{
	if( alias )
	{
		for( int i = 0; s_WeaponAliasInfo[ i ] != nullptr; ++i )
			if( !_stricmp( s_WeaponAliasInfo[i], alias ) )
				return (CSWeaponID)i;
	}

	assert( false );
	return WEAPON_NONE;
}

// =====================================================================================================================================================================

const char *WeaponIDToAlias( CSWeaponID weaponID )
{
	// NOTE: this returns prettier names that can't be found in s_WeaponAliasInfo for certain weapons

	if( weaponID == WEAPON_MP5NAVY )
	{
		return "MP5";
	}
	if( weaponID == WEAPON_ELITE )
	{
		return "dualies"; // "Dual elites" seems a bit too long
	}
	if( weaponID == WEAPON_WORLD )
	{
		return "unknown weapon";
	}
	if( weaponID >= WEAPON_NONE && weaponID < WEAPON_MAX )
	{
		return s_WeaponAliasInfo[ weaponID ];
	}

	assert( false );
	return nullptr;
}

// =====================================================================================================================================================================

bool WeaponIsSniper( const char *szWeaponAlias )
{
	CSWeaponID weaponID = AliasToWeaponID( szWeaponAlias );

	switch( weaponID )
	{
		case WEAPON_AWP:
		case WEAPON_SCOUT:
		case WEAPON_SG550:
		case WEAPON_G3SG1:
			return true;
	}

	return false;
}

// =====================================================================================================================================================================

bool WeaponUsesBullets( const char *szWeaponAlias )
{
	CSWeaponID weaponID = AliasToWeaponID( szWeaponAlias );

	switch( weaponID )
	{
		case WEAPON_HEGRENADE:
		case WEAPON_FLASHBANG:
		case WEAPON_SMOKEGRENADE:
		case WEAPON_KNIFE:
		case WEAPON_C4:
			return false;
	}

	return true;
}

// =====================================================================================================================================================================