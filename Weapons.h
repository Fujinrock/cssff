#pragma once

// This matches CSWeaponID
static const char * s_WeaponAliasInfo[] = 
{
	"NONE",		// 0 WEAPON_NONE
	"P228",		// 1 WEAPON_P228
	"glock",	// 2 WEAPON_GLOCK
	"scout",	// 3 WEAPON_SCOUT
	"XM1014",	// 4 WEAPON_XM1014
	"C4",		// 5 WEAPON_C4
	"MAC10",	// 6 WEAPON_MAC10
	"AUG",		// 7 WEAPON_AUG
	"elite",	// 8 WEAPON_ELITE
	"fiveseven",// 9 WEAPON_FIVESEVEN
	"UMP45",	// 10 WEAPON_UMP45
	"SG550",	// 11 WEAPON_SG550

	"galil",	// 12 WEAPON_GALIL
	"famas",	// 13 WEAPON_FAMAS
	"USP",		// 14 WEAPON_USP
	"AWP",		// 15 WEAPON_AWP
	"MP5navy",	// 16 WEAPON_MP5N 
	"M249",		// 17 WEAPON_M249
	"M3",		// 18 WEAPON_M3
	"M4A1",		// 19 WEAPON_M4A1
	"TMP",		// 20 WEAPON_TMP
	"G3SG1",	// 21 WEAPON_G3SG1
	"deagle",	// 22 WEAPON_DEAGLE
	"SG552",	// 23 WEAPON_SG552
	"AK47",		// 24 WEAPON_AK47
	"knife",	// 25 WEAPON_KNIFE
	"P90",		// 26 WEAPON_P90

	"WORLD",	// 27 WORLD (suicide, usually)

	"hegrenade",				// 28 WEAPON_HEGRENADE
	"flashbang",				// 29 WEAPON_FLASHBANG
	"smokegrenade_projectile"	// 30 WEAPON_SMOKEGRENADE (Why does only this use _projectile in the end?)
};

// This matches s_WeaponAliasInfo
enum CSWeaponID
{
	WEAPON_NONE = 0,

	WEAPON_P228,
	WEAPON_GLOCK,
	WEAPON_SCOUT,
	WEAPON_XM1014,
	WEAPON_C4,
	WEAPON_MAC10,
	WEAPON_AUG,
	WEAPON_ELITE,
	WEAPON_FIVESEVEN,
	WEAPON_UMP45,
	WEAPON_SG550,

	WEAPON_GALIL,
	WEAPON_FAMAS,
	WEAPON_USP,
	WEAPON_AWP,
	WEAPON_MP5NAVY,
	WEAPON_M249,
	WEAPON_M3,
	WEAPON_M4A1,
	WEAPON_TMP,
	WEAPON_G3SG1,
	WEAPON_DEAGLE,
	WEAPON_SG552,
	WEAPON_AK47,
	WEAPON_KNIFE,
	WEAPON_P90,

	WEAPON_WORLD,	// Suicide

	WEAPON_HEGRENADE,
	WEAPON_FLASHBANG,
	WEAPON_SMOKEGRENADE,

	WEAPON_MAX
};

enum CSWeaponCategory
{
	CATEGORY_NONE = 0,

	CATEGORY_KNIFE,
	CATEGORY_PISTOL,
	CATEGORY_SHOTGUN,
	CATEGORY_SMG,
	CATEGORY_RIFLE,			// This also includes the M249
	CATEGORY_SNIPER,		// Only AWP and Scout
	CATEGORY_AUTOSNIPER,	// SG550 and G3SG1
	CATEGORY_GRENADE,		// Only relevant for HE

	CATEGORY_INVALID		// For settings reading only
};

#define CATEGORY_GENERAL	CATEGORY_NONE

CSWeaponCategory GetWeaponCategory( CSWeaponID weapon );

CSWeaponCategory GetWeaponCategory( CSWeaponID *pWeapons, int num_weapons );

CSWeaponID AliasToWeaponID( const char *alias );

const char *WeaponIDToAlias( CSWeaponID weaponID );

bool WeaponIsSniper( const char *szWeaponAlias );

bool WeaponUsesBullets( const char *szWeaponAlias );