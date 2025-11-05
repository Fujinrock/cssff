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
- Support for STV, POV and STV-POV demos
- Support for ClientMod demos
- Frag timing
- Error handling
- Batch processing
- Plenty of settings to filter out bad frags
- Ability to specify different settings for different weapon categories
- Dumping of demo information
- Clean, rounded ticks for frags
- And more...

## Support
cssff is only for Windows, and it only supports demos that are compatible with CS:S v34 (build 4044). Demos from the Steam version or v77 of CS:S are NOT supported. This is because the program was meant for editors/moviemakers to help them find new content for their projects, and CS:S versions past v34 have never been popular for that purpose.

## How to build
The project should not require any outside libraries, but C++ 20 features are used. A solution file for Visual Studio 2022 is included, which can be used to build the project.

## How to use
cssff is simple to use. You can simply drag and drop demo files or folders onto the executable to process them. When multiple demos are processed, an output file is always written either to the program folder or demo directory depending on the settings used. When processing a single demo, more information about the demo is displayed inside the program window, including information about the found frags. The program can also be run from the command prompt, but do note that there are no special arguments that would make this necessary.

### Settings
The default settings file should be called "cssff_settings.ini" and it should be placed in the same directory as the executable. You can also drag and drop another .ini file onto the executable alongside any demos to read the settings from that file instead. If no settings file is found or specified, the program will use default built-in values.

The settings file can be divided into different weapon categories. The syntax for specifying a category is [categoryname], and any settings that follow will only apply to that category. If no category is specified at any point, the general category is used, which applies to all weapons. Multiple categories can be used at once by stacking them, which means that the settings that follow will apply to all of those categories. The settings of a specific weapon category will override general settings, and general settings will be used if the weapon category does not specify a value for a settings field. This means that you do not need to add every settings field to every weapon category. An example of the contents of a valid settings file with all fields and categories is included in the repository.

Valid category names:
- General (applies to all weapons)
- Knife
- Pistols
- Shotguns
- Smgs
- Rifles (includes M249)
- Snipers (AWP & Scout)
- AutoSnipers (SG550 & G3SG1)
- Grenades (HE only)

The following settings fields should only be set in the general category, and are ignored in other categories, as they are not weapon-specific:
- dump_to_file (Whether to dump frags/data to a text file)
- enable_batch_processing (Enable/disable batch processing)
- write_output_to_demo_directory (Whether the output file should be written to the folder where the processed demo/batch was or to the executable folder)
- tick_frags_vs_bots (Whether frags against bots are ticked or not)

### Batch processing
If no demos are specified and batch processing is enabled when running the program, the program will search for demos from the executable directory and batch process them. You can also drag and drop multiple demos or an entire folder of demos onto the program to begin batch processing. Batch processing can be disabled in the settings file by changing "enable_batch_processing" to "false". Parsing more than one demo automatically enables "dump_to_file", which means results are always dumped to file when batch processing. When processing folders, do note that only one folder can be parsed at a time, and no subfolders are processed.

## Understanding the output
Each found frag will have the following information:
- Tick where the frag happens
- The name and team of the player who does the frag
- A description of the frag
- Demo name and type (either in the program window or in the output file)

Frags that were ticked as 3k, 4k or 5k will always include the time that passed between the first and last kill. However, the total amount of enemies killed during the round is always written at the beginning of the description regardless of if those kills matter for the frag that was ticked (unless the amount is the same as the ticked frag implies). This can lead to descriptions such as "4k including 3k (3hs) deagle in 1.08 seconds", where the time only applies to the 3k, or "4k with AK47 mid-air headshot", where the time is not included, because only the mid-air kill was ticked as a frag (and the frag's tick will only point to that mid-air kill, not the beginning of the 4k). This behavior might be confusing at first, but it is intentional, because it can be useful to know about other kills done during the round.

When dumping to file, a standardized format is not used (no json etc.). Instead, a more human-readable format is used.
