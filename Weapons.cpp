#include "Weapons.h"
#include <string>
#include <map>
#include <assert.h>

// =====================================================================================================================================================================

CSWeaponCategory GetWeaponCategory( CSWeaponID weapon )
{
	switch( weapon )
	{
		default:
			return CATEGORY_NONE;

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
/**
 * Determines what settings category should be used to check if this multikill frag should be ticked
 * @param pWeapons			pointer to the CSWeaponID array holding the used weapons
 * @param num_weapons		number of weapon IDs in the array
 * @param outWeapon			used to store the weapon ID (or WEAPON_NONE) of the singular weapon that was used to do this frag
 * @param outCategory		used to store the weapon category (or CATEGORY_GENERAL) of the weapon(s) used to do this frag
 */
void GetMultiKillFragWeaponCategory( CSWeaponID *pWeapons, int num_weapons, CSWeaponID &outWeapon, CSWeaponCategory &outCategory )
{
	// Rules for determining the category:
	// - If a single weapon was used, its ID and category are returned
	// - If multiple weapons of the same category were used and it was less than 5k, or a 5k but neither weapon killed 4,
	//	 WEAPON_NONE is returned for weapon ID but their common weapon category is returned
	// - If a 5k had two weapons, but the other weapon was used to do 4 of the kills, its weapon ID and category are returned
	// - If a 5k had more weapons but only two categories and the other category had 4 kills, return WEAPON_NONE and the category with 4 kills
	// - If more than two weapons were used or weapons of different categories were used and the rules above don't apply,
	//	 WEAPON_NONE and CATEGORY_GENERAL are returned
	// Got all that?

	// This function is a bit hacky in that it relies on the fact that num_weapons == number of kills,
	// because duplicate weapons are not removed before calling this
	std::map< CSWeaponID, int > weapons;
	std::map< CSWeaponCategory, int > categories;

	for( int i = 0; i < num_weapons; ++i )
	{
		CSWeaponCategory category = GetWeaponCategory( pWeapons[ i ] );

		++weapons[ pWeapons[ i ] ];
		++categories[ category ];
	}

	assert( weapons.size() > 0 && categories.size() > 0 );

	// Only one weapon, return its ID and category
	if( weapons.size() == 1 )
	{
		outWeapon = weapons.begin()->first;
		outCategory = categories.begin()->first;
		return;
	}

	// More than one weapon but same category in a 4k or 3k
	// Return WEAPON_NONE and common category
	// (+)5k will be checked later
	if( categories.size() == 1 && num_weapons < 5 )
	{
		outWeapon = WEAPON_NONE;
		outCategory = categories.begin()->first;
		return;
	}

	// If a 4k or 3k has more than 1 category or a (+)5k has more than 2 categories,
	// use WEAPON_NONE and the general category
	if( num_weapons < 5 || categories.size() > 2 )
	{
		outWeapon = WEAPON_NONE;
		outCategory = CATEGORY_GENERAL;
		return;
	}

	// At this point we know it's a (+)5k with more than one weapon
	outWeapon = WEAPON_NONE;
	outCategory = CATEGORY_GENERAL;

	// If the other of two weapons did 4 of the kills, return it and its category
	if( weapons.size() == 2 )
	{
		for( const auto &[weapon, count] : weapons )
		{
			if( count == num_weapons - 1 )
			{
				outWeapon = weapon;
				outCategory = GetWeaponCategory( weapon );
				return;
			}
		}
	}

	// The (+)5k has more than two weapons, so WEAPON_NONE will be returned, but check if we can still return a common category
	for( const auto &[category, count] : categories )
	{
		if( count >= num_weapons - 1 )
		{
			outCategory = category;
			return;
		}
	}
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
	if( weaponID == WEAPON_WORLD || weaponID == WEAPON_NONE )
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
		case WEAPON_NONE:
		case WEAPON_WORLD:
			return false;
	}

	return true;
}

// =====================================================================================================================================================================