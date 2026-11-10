# Dongle + mainline migration

## What changed
- ZMK: `urob/zmk` fork (Zephyr 3.5) → `zmkfirmware/zmk` main, pinned to a commit (v0.3 predates Zephyr 4.1 and the HWMv2 Zen boards)
- Helpers: `zmk-nodefree-config` submodule → `zmk-helpers` west module (v2 include paths)
- `tri_state`: vendored from `urob/zmk-tri-state` (MIT) as a local module: `CMakeLists.txt`, `Kconfig`, `behaviors/`, `dts/bindings/`
- Layout: unified template (`zephyr/module.yml`, `config/west.yml`, `build.yaml`), official `build-user-config` workflow
- Boards: `corneish_zen_v2_{left,right}` → `corneish_zen_{left,right}//zmk` (rev 2.0.0 is the default); dongle on `nice_nano//zmk`
- New shield `zen_dongle` (central, no keys) + `config/zen_dongle.{keymap,conf}`
- Left half is now a peripheral (`-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=n`); right already was. The left build also swaps the layer/output display widgets for the peripheral one (same as the right half), otherwise it fails to link

## Dropped options (fork-only, not in mainline)
- `ZMK_DISPLAY_FULL_REFRESH_PERIOD`: no periodic full e-paper refresh, expect some ghosting
- `ZMK_DISPLAY_HIDE_MOMENTARY_LAYERS`: layer widget shows momentary layers again
- `CUSTOM_WIDGET_LAYER_STATUS_HIDE_HEADING`: heading strip is back
- `IL0323_ALTERNATIVE_REFRESH`: stock IL0323 refresh

## Flash order
1. Check the clone: double-tap reset, open `INFO_UF2.TXT`, it must say `nice!nano v2`
2. `settings_reset_*.uf2` on all three boards (halves + dongle)
3. Flash `corneish_zen_left_peripheral`, `corneish_zen_right`, `zen_dongle`
4. Power all three on together; the halves pair to the dongle by themselves
5. Dongle must stay plugged in; without it the halves do nothing

## Modes
- Role is compile-time, so no runtime switch; the right half also bonds to one central only
- Dongle mode: `corneish_zen_left_peripheral` + `zen_dongle`; normal mode: `corneish_zen_left_central`
- Switching = reflash the left half, `settings_reset` the halves, re-pair

## Revert (no dongle)
- Flash `settings_reset_zen_left` and `settings_reset_zen_right`
- Flash `corneish_zen_left_central` + `corneish_zen_right`
- Re-pair the left half to the host

## Unverified
- Left-half display as a peripheral: same widgets as the right half (battery + peripheral status), not run on hardware
- Left-half USB port: probably charge only, no keys
- Battery gain: anecdotal, roughly the left half approaching the right half's runtime
