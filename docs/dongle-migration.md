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
2. `settings_reset_*.uf2` on ALL boards first (both halves + dongle). Resetting one side after the other already bonded leaves a stale key: the half connects but sends nothing (`Security failed ... err 2` in the central log)
3. Then flash `corneish_zen_right`, `corneish_zen_left_peripheral`, `zen_dongle` (halves before the central)
4. Power all three on together; the halves pair to the dongle by themselves
5. Dongle must stay plugged in; without it the halves do nothing
- Copy to the drive with `/bin/cat x.uf2 > /Volumes/CORNEISHZEN/x.uf2` or `cp` (`cat` is `bat` in this shell); a trailing "Device not configured" is normal

## Modes
- Role is compile-time, so no runtime switch; the right half also bonds to one central only
- Dongle mode: `corneish_zen_left_peripheral` + `zen_dongle`; normal mode: `corneish_zen_left_central`
- Switching = reflash the left half, `settings_reset` the halves, re-pair

## Revert (no dongle)
- Flash `settings_reset_zen_left` and `settings_reset_zen_right`
- Flash `corneish_zen_left_central` + `corneish_zen_right`
- Re-pair the left half to the host

## Added after the first migration
- `CONFIG_ZMK_USB_BOOT` removed (it is USB boot-protocol support, not a bootloader trigger)
- `&bootloader` on the NUM layer: bottom-row far left (position 24, left half) and far right (35, right half). Locality is event-source: it reboots the half whose key you pressed
  - Standalone: press either key; the right half's key is forwarded by the central
  - Dongle: same keys reach the halves through the dongle; the dongle itself has no keys, double-tap its reset
  - A mistyped press drops that half into the bootloader until reset or reflash
- Auto-unpair (`CONFIG_ZEN_SPLIT_AUTO_UNPAIR_ON_KEY_MISMATCH`, `split/auto_unpair.c`): port of zmk#3377 as an out-of-tree `bt_conn_cb`; on a central, `PIN_OR_KEY_MISSING` from a split peripheral drops that bond and disconnects so it re-pairs. Differs from the PR by acting only on central-role links, never on host links. On for `zen_dongle` and `corneish_zen_left_central`. Relies on `ZMK_BLE_EXPERIMENTAL_SEC` (peripheral accepts re-pair). Not tested on hardware
- 2M PHY (`CONFIG_BT_CTLR_PHY_2M=y`): `ZMK_BLE_EXPERIMENTAL_CONN` turns it off by default (stability). On for `right`, `left_peripheral`, `zen_dongle`; off for `left_central` (same radio serves the Mac). A link only uses 2M if both ends have it, so the right half gets 2M only in dongle mode. 2M costs ~3 dB receiver sensitivity
- Split link latency 99 (central builds, default was 30): interval stays 6 (7.5 ms), so a key press still leaves at the next connection event; latency only lets an idle half skip events (~750 ms wake instead of ~230 ms) and delays central-to-peripheral traffic. Supervision timeout 4 s > (1+99)*7.5 ms*2 = 1.5 s. Untested; no battery measurement
- TX power left at +8 dBm; measure first: `docs/range-test.md`, `scripts/split-rssi.py`, `*_log` artifacts
- Deep-sleep timeout unchanged (30 min)
- keymap-drawer: `keymap_drawer.config.yaml` adds `zmk-helpers/include`
- Opt-in artifacts: `*_nodisplay` (display off), `*_idle_exp` (below)

## Experimental: idle split parameters (zmk#3182 port)
- `CONFIG_ZEN_SPLIT_BLE_IDLE_PARAMS` (`split/central_idle.c`): central moves split links to interval 22.5 ms / latency 10 when ZMK goes idle (30 s), back to normal on activity. Draft upstream: ~0.6-1 s window of possible misordered keys when waking
- The Zephyr controller fix it needs (zmkfirmware/zephyr#51, peripheral assert on connection-parameter update) is already in the pinned Zephyr; the cherry-pick applied empty. Builds clean; untested on hardware
- Build only via the `*_idle_exp` artifacts; default builds are unchanged

## Unverified
- Left-half display as a peripheral: same widgets as the right half (battery + peripheral status), not run on hardware
- Left-half USB port: probably charge only, no keys
- Battery gain: anecdotal, roughly the left half approaching the right half's runtime
