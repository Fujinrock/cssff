# cssff - Counter-Strike: Source Frag Finder
Windows command line application for quickly finding frags from Counter-Strike: Source v34 demo files.

## Features
cssff is capable of detecting many different types of frags:
- 5ks, 4ks and 3ks in specified timeframe
- Collats (doubles, triples, quadros, pentas)
- Projectile kills (flash and smoke)
- Jumpshots & laddershots
- Flickshots
- Noscopes
- Wallbangs (only in ClientMod demos)
- Frags that the POV recorder spectated (with the exception of mid-air frags)

Other features include:
- Support for Source TV, POV and client-recorded STV demos
- Support for ClientMod demos
- Frag timing
- Batch processing
- Error handling
- Plenty of settings to filter out bad frags
- Ability to have different settings for different weapons and weapon categories
- Dumping of demo information
- Clean, rounded ticks for frags
- And more...

## Support
cssff is only for Windows, and it only supports demos that are compatible with CS:S v34 (build 4044). Demos from the Steam version or v77 of CS:S are NOT supported. This is because the program was meant for editors/moviemakers to help them find new content for their projects, and CS:S versions past v34 have never been popular for that purpose.

## How to build
The project should not require any outside libraries, but C++ 20 features are used. A solution file for Visual Studio 2022 is included, which can be used to build the project. When building, the target platform should be set to Win32 (x86).

## How to use
cssff is simple to use. You can simply drag and drop demo files or folders onto the executable to process them. When multiple demos are processed, an output file is always written either to the program folder or demo directory depending on the settings used. When processing a single demo, more information about the demo is displayed inside the program window, including information about the found frags. The program can also be run from the command prompt, but do note that there are no special arguments that would make this necessary.

### Settings
The settings system of cssff is hierarchial. This means that the settings are divided into weapon-specific, weapon-category-specific and general settings. Weapon-specific settings rank the highest and will override any weapon-category-specific or general settings, whereas weapon-category-specific settings will only override general settings. This makes it fast to configure the program while still having maximum control over the output. It's recommended to specify values for every settings field only in the general category, which apply to all weapons, and only set weapon or weapon-category-specific settings as seen fit.

The syntax for specifying a category in the settings file is [categoryname], and any settings that follow will only apply to that category. If no category is specified at any point, the general category is used. Multiple categories can be used at the same time by stacking them in consecutive rows, which means that the settings that follow will apply to all of those categories. An example of the contents of a valid settings file with all fields and weapon categories is included in the repository.

The default settings file should be called "cssff_settings.ini" and it should be placed in the same directory as the executable. You can also drag and drop another .ini file onto the executable alongside any demos to read the settings from that file instead. If an entire folder is processed, the settings file can be placed inside that folder and it will be used if it's the only .ini file inside the folder. If no settings file is found or specified, the program will use default built-in values.

---
**Valid weapon category names:**
- General (applies to all weapons)
- Knife
- Pistols
- Shotguns
- Smgs
- Rifles (includes M249)
- Snipers (AWP & Scout)
- AutoSnipers (SG550 & G3SG1)
- Grenades (HE only)

---
**Valid weapon names:**
- AK47
- AUG
- AWP
- Deagle
- Elite
- Famas
- Fiveseven
- G3SG1
- Galil
- Glock
- M249
- M3
- M4A1
- MAC10
- MP5
- P228
- P90
- Scout
- SG550
- SG552
- TMP
- UMP45
- USP
- XM1014

---
**Valid settings fields for weapons and weapon categories**
|Field name|Value type|Description|
|----------|----------|-----------|
|tick_5ks|boolean|Whether to tick 5 kills from a player|
|tick_4ks|boolean|Whether to tick 4 kills from a player|
|tick_3ks|boolean|Whether to tick 3 kills from a player|
|tick_doubles|boolean|Whether to tick collaterals with 2 kills|
|tick_triples|boolean|Whether to tick collaterals with 3 kills|
|tick_quadros|boolean|Whether to tick collaterals with 4 kills|
|tick_pentas|boolean|Whether to tick collaterals with 5 kills|
|tick_flash_smoke_kills|boolean|Whether to tick kills done with flashbangs or smokegrenades|
|tick_jumpshots|boolean|Whether to tick jumpshots|
|tick_noscopes|boolean|Whether to tick noscopes|
|tick_flickshots|boolean|Whether to tick flickshots|
|tick_wallbangs|boolean|Whether to tick kills through objects (only in ClientMod demos)|
|5k_max_time|decimal|Maximum time in seconds that may pass between first and last kill in a 5k to tick it (negative number means unlimited time)|
|4k_max_time|decimal|Maximum time in seconds that may pass between first and last kill in a 4k to tick it|
|3k_max_time|decimal|Maximum time in seconds that may pass between first and last kill in a 3k to tick it|
|tick_slow_stationary_5ks|boolean|Whether a long 5k in one spot should be ticked regardless of time|
|tick_slow_stationary_4ks|boolean|Whether a long 4k in one spot should be ticked regardless of time|
|tick_slow_stationary_3ks|boolean|Whether a long 3k in one spot should be ticked regardless of time|
|slow_5k_max_range|decimal|Fragger's maximum distance from the first kill for the 5k to be considered stationary|
|slow_4k_max_range|decimal|Fragger's maximum distance from the first kill for the 4k to be considered stationary|
|slow_3k_max_range|decimal|Fragger's maximum distance from the first kill for the 3k to be considered stationary|
|5k_must_include_special_kill|boolean|Whether a 5k has to include a flickshot, jumpshot, noscope, wallbang or a flashed kill in order to be ticked|
|4k_must_include_special_kill|boolean|Whether a 4k has to include a flickshot, jumpshot, noscope, wallbang or a flashed kill in order to be ticked|
|3k_must_include_special_kill|boolean|Whether a 3k has to include a flickshot, jumpshot, noscope, wallbang or a flashed kill in order to be ticked|
|5k_special_kill_extra_max_time|decimal|How many seconds each special kill should raise the maximum time for ticking 5 kills|
|4k_special_kill_extra_max_time|decimal|How many seconds each special kill should raise the maximum time for ticking 4 kills|
|3k_special_kill_extra_max_time|decimal|How many seconds each special kill should raise the maximum time for ticking 3 kills|
|5k_min_headshots|integer|Minimum amount of headshots to tick a 5k|
|4k_min_headshots|integer|Minimum amount of headshots to tick a 4k|
|3k_min_headshots|integer|Minimum amount of headshots to tick a 3k|
|double_min_headshots|integer|Minimum amount of headshots to tick a double|
|triple_min_headshots|integer|Minimum amount of headshots to tick a triple|
|quadro_min_headshots|integer|Minimum amount of headshots to tick a quadro|
|penta_min_headshots|integer|Minimum amount of headshots to tick a penta|
|special_double_ignores_min_hs|boolean|Whether minimum headshots should be ignored if the double was a flickshot, jumpshot, noscope, wallbang or done while flashed|
|special_triple_ignores_min_hs|boolean|Whether minimum headshots should be ignored if the triple was a flickshot, jumpshot, noscope, wallbang or done while flashed|
|special_quadro_ignores_min_hs|boolean|Whether minimum headshots should be ignored if the quadro was a flickshot, jumpshot, noscope, wallbang or done while flashed|
|special_penta_ignores_min_hs|boolean|Whether minimum headshots count should be ignored if the penta was a flickshot, jumpshot, noscope, wallbang or done while flashed|
|wallbang_headshot_only|boolean|Whether only headshot wallbangs should be ticked|
|wallbang_require_another_kill|boolean|Whether a second (non-wallbang) kill must accompany a wallbang for it to be ticked|
|wallbang_another_kill_max_delta_time|decimal|Maximum time in seconds that may pass between the wallbang and the other kill to tick it|
|flickshot_headshot_only|boolean|Whether only headshot flicks should be ticked|
|flickshot_min_distance|decimal|Victim's minimum distance from the fragger in order to tick a flickshot|
|flickshot_max_duration|integer|Maximum flick duration in milliseconds (max 300)|
|flickshot_min_angle_modifier|decimal|Flickshot minimum angle modifier (values above 1 increase the angle while values below 1 decrease it)|
|jumpshot_min_post_kill_air_time|decimal|Time in seconds that the fragger has to stay in air after a jumpshot in order to tick it|
|jumpshot_min_distance|decimal|Victim's minimum distance from the fragger in order to tick a jumpshot|
|jumpshot_min_distance_hs_modifier|decimal|Jumpshot minimum distance modifier used for headshots (values below 1 decrease the distance while values above 1 increase it)|
|jumpshot_min_distance_wb_modifier|decimal|Jumpshot minimum distance modifier used for wallbangs (only in ClientMod demos)|
|noscope_min_distance|decimal|Victim's minimum distance from the fragger in order to tick a noscope|
|noscope_min_distance_hs_modifier|decimal|Noscope minimum distance modifier used for headshots|
|noscope_min_distance_wb_modifier|decimal|Noscope minimum distance modifier used for wallbangs (only in ClientMod demos)|

---
The following settings fields should only be set in the general category, and are ignored in other categories, as they are not weapon-specific:
|Field name|Value type|Description|
|----------|----------|-----------|
|dump_to_file|boolean|Whether to dump frags, warnings and errors to a text file|
|enable_batch_processing|boolean|Enable/disable batch processing|
|write_output_to_demo_directory|boolean|Whether the output file should be written to the folder where the processed demo/batch was or to the executable folder|
|tick_frags_vs_bots|boolean|Whether frags against bots are ticked or not|
|tick_frags_by_bots|boolean|Whether frags by bots are ticked or not|

---
### Batch processing
If no demos are specified and batch processing is enabled when running the program, the program will search for demos from the executable directory and batch process them. You can also drag and drop multiple demos or an entire folder of demos onto the program to begin batch processing. Batch processing can be disabled in the settings file by changing "enable_batch_processing" to "false". Parsing more than one demo automatically enables "dump_to_file", which means results are always dumped to file when batch processing. When processing folders, do note that only one folder can be parsed at a time, and no subfolders are processed.

## Understanding the output
Each found frag will have the following information:
- Tick where the frag happens
- The name and team of the player who does the frag
- A description of the frag
- Demo name and type (either in the program window or in the output file)

Frags that were ticked as 3k, 4k or 5k will always include the time that passed between the first and last kill. However, the total amount of enemies killed during the round is always written at the beginning of the description regardless of if those kills matter for the frag that was ticked (unless the kill count is the same as the ticked frag implies). This can lead to descriptions such as "4k including 3k (3hs) deagle in 1.08 seconds", where the time only applies to the 3k, or "4k with AK47 mid-air headshot", where the time is not included, because only the mid-air kill was ticked as a frag (and the frag's tick will only point to that mid-air kill, not the beginning of the 4k). This behavior might be confusing at first, but it is intentional, because it can be useful to know about other kills done during the round.

When dumping to file, a standardized format is not used (no json etc.). Instead, a more human-readable format is used.
