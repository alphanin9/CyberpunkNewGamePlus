# WIP Cyberpunk save file reading implementation

A native code implementation of parts of [WolvenKit](https://github.com/WolvenKit/WolvenKit)'s Cyberpunk 2077 save parsing. Intended for use in a RED4ext plugin later on, so there's no fancy UI needed. Will select the last Point Of No Return save for parsing due to implementation reasons.

Dependencies include [lz4](https://github.com/lz4/lz4) for save data decompression and [RED4ext.SDK](https://github.com/WopsS/RED4ext.SDK/tree/master) for various types utilized by Cyberpunk 2077.

## But why?

I needed something that could read save files from a RED4ext plugin without needing to make the game actually load the save in question.

## Supports

- Player inventory parsing
- Not much else

## To do

- Player development system parsing (may be tough thanks to WolvenKit's reflection-reliant logic being difficult to do from within native code)
- Code cleanup

## Credits

- The [WolvenKit](https://github.com/WolvenKit/WolvenKit) team, [Seberoth](https://github.com/seberoth) in particular