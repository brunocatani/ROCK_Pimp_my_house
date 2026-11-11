# ROCK Pimp my House

A Fallout 4 VR addon for ROCK that lets you decorate by hand: hold an object
against a surface, click the stick, and it stays exactly where you put it.

## Features

- Hold any loose object so it touches a wall, shelf, table or other fixed
  surface, then click the stick of the hand holding it. The object stays there,
  locked in place, instead of falling.
- Placed objects stay where you put them when you save, reload or come back to
  the cell.
- Grab or pull a placed object normally to take it again.
- Each hand's stick click places that hand's object; an object held in both
  hands answers either click. The other hand's click keeps its own actions, such
  as the Favorites menu or RobCo PALM.
- Weapons, held or equipped, are placed by holding the stick click for one
  second, because a short click on an equipped weapon belongs to ROCK's bipod.

Installing and enabling the plugin turns the feature on. There are no settings.

## Requirements and installation

- Fallout 4 VR 1.2.72.0 and F4SEVR.
- ROCK 1.0 with its requirements. Without ROCK the addon stays off. A loaded
  ROCK without the API this version was built for stops the game at the main
  menu with an error box naming the mismatch.

Install through your mod manager. The plugin belongs at
`Data/F4SE/Plugins/ROCK_Pimp_my_house.dll`. Its log is
`Documents/My Games/Fallout4VR/F4SE/ROCK_Pimp_my_house.log`.

<details>
<summary>Building from source</summary>

Requires Windows x64, Visual Studio 2022 with the v143 C++ toolset, CMake 4.2 or
newer, vcpkg with `VCPKG_ROOT` set, CommonLibF4VR, ROCK's public headers
(`include/ROCK/`, through `ROCK_INCLUDE_DIR`) and RPS Framework's include
directory (`RPS_FRAMEWORK_INCLUDE_DIR`). Copy `CMakeUserPresets.json.template`
to `CMakeUserPresets.json` and set the deployment folder with
`COPY_PLUGIN_BASE_PATH`.

```powershell
cmake --preset custom-fast
cmake --build build-fast --config Release

cmake --build build-fast --config Release --target build_tests
ctest --test-dir build-fast -C Release --output-on-failure
```

</details>

## License

[GNU GPL v3 only (GPL-3.0-only)](LICENSE).
