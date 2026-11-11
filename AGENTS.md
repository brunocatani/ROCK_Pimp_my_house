# ROCK Pimp my House

Optional decoration add-on for ROCK. Installing and enabling the DLL enables the feature; there is no INI toggle.

- ROCK owns the safe held-placement operation; this plugin owns eligibility, click, hold and rearm behaviour and registration. Use ROCK's Grab interface and the Animation BeforeRock callback; never duplicate engine offsets or Havok writes.
- Each hand's stick click places an ordinary object that hand holds while it touches a surface; an object held in both hands answers either click. Only that hand's click is reserved, so the other hand's click keeps its own actions (the Favorites menu or PALM on the right). A weapon, held or equipped, is placed by holding the click for one second instead, because the click belongs to ROCK's bipod on an equipped weapon; the equipped weapon answers only its firing hand's click (`equippedWeaponHand`). That press is never reserved, so bipod tapping keeps working; a held loose weapon's press is reserved as an object's is.
- A loaded ROCK that does not serve the ROCK API this plugin was built for stops the game at the main menu with an error box (RPS Framework's `RPS/Runtime/RequiredUpdate.h`, found through `RPS_FRAMEWORK_INCLUDE_DIR`); without ROCK the plugin stays off.
- Build: `cmake --preset custom-fast`, then `cmake --build --preset custom-fast --config Release`. Tests: `ctest --preset custom-fast`. Deploys only the DLL and PDB to `F4SE/Plugins` under the mod folder set by `COPY_PLUGIN_BASE_PATH` in `CMakeUserPresets.json`.
- Log: `Documents\My Games\Fallout4VR\F4SE\ROCK_Pimp_my_house.log`.
