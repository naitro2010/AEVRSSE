# ***Skyrim AE Frame Interleaved VR Stereo Rendering Mod***

## ***Warning***
Do not use this mod if you have photosensitive epilepsy.
It renders a different perspective each frame which can cause flickering if used with a normal 2D display or without ReShade and 3DToElse.

## ***Runtime requirements***
- Skyrim AE 1.6.1170
- [Skyrim Script Extender (SKSE)](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## ***Optional requirements***
- For changing stereo rendering settings like convergence and eye separation with console commands [ConsoleUtil Extended](https://www.nexusmods.com/skyrimspecialedition/mods/133569)
- For Side by Side and Over/Under stereo formats you will probably want ReShade and [3DToElse](https://github.com/BlueSkyDefender/Depth3D/blob/master/Other%20%20Shaders/3DToElse.fx)

## ***Compatibility***

- Temporal Anti Aliasing and Frame Generation need to be off
- Shadows and some shader features might need to be disabled until I figure out how to fix compatibility with some lighting features.
- 3DToElse is required to swap left and right frames in ReShade at the moment.
