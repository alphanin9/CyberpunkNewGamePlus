# Work-in-progress Cyberpunk 2077 New Game+ implementation
A RED4ext plugin native code implementation of parts of [WolvenKit](https://github.com/WolvenKit/WolvenKit)'s Cyberpunk 2077 save parsing along with providing an ingame API to achieve New Game+. 
Dependencies include:
- [lz4](https://github.com/lz4/lz4) for save data decompression
- [RED4ext.SDK](https://github.com/WopsS/RED4ext.SDK) for various types utilized by Cyberpunk 2077, taking advantage of Cyberpunk 2077's runtime type information system and plugin initialization
- [RedLib](https://github.com/psiberx/cp2077-red-lib) for a simplified way of registering new native classes within Cyberpunk 2077's scripting system
- [simdjson](https://github.com/simdjson/simdjson) for JSON parsing
- [ArchiveXL](https://github.com/psiberx/cp2077-archive-xl) for loading archive files from arbitrary locations

## Supports
- Player inventory parsing
- Scriptable system parsing
- Minor implementation of PersistencySystem parsing, being able to parse the player's vehicle garage
- Redscript integration, being able to provide internal game systems the loaded save data

## Supported game versions
- 2.12a

## Ingame dependencies
- RED4ext 1.24.3+
- Codeware 1.9.0+
- ArchiveXL, version-independent
- TweakXL, version-independent

## Save versions tested on
- 2.00+
- 1.63 and older compatibility is not planned due to large differences in save handling
- Savegame parsing may fail when attempting to parse modded saves with now uninstalled mods

## To do
- Code cleanup
    - Making everything share a naming convention 

## Credits
- ### RED4ext plugin
    - The [WolvenKit](https://github.com/WolvenKit/WolvenKit) team, [Seberoth](https://github.com/seberoth) in particular
    - [psiberx](https://github.com/psiberx) for help with RED4ext and understanding Cyberpunk 2077's inner workings
- ### Redscript layer
    - [Rayshader](https://github.com/rayshader) for [NativeDB](https://nativedb.red4ext.com/) and assistance during development
    - [psiberx](https://github.com/psiberx) again for [Codeware](https://github.com/psiberx/cp2077-codeware)
- ### Ingame assets
    - [MorningSpice](https://github.com/MorningSpice) and other extremely helpful people for general assistance in finding various unused VOs to reactivate