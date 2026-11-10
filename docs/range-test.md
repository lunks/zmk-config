# Range test: how far is the dongle really

Goal: measure the RSSI the central sees from each half at the real dongle position, then decide on TX power / PHY.

## Needs
- `zen_dongle_log.uf2` (dongle) or `corneish_zen_left_central_log.uf2` (standalone), from the CI artifacts
- the dongle in its final USB port, halves at the typing position
- Mac terminal; `python3` (no packages)

## Procedure
1. Flash the log build onto the central (double-tap reset, drag the `.uf2`). Leave it plugged in where it will live.
2. Find the port: `ls /dev/cu.usbmodem*`
3. Capture, 3 min: `stty -f /dev/cu.usbmodem1101 115200 raw -echo; timeout 180 /bin/cat /dev/cu.usbmodem1101 > range.log`  (use `/bin/cat`, plain `cat` is `bat` here)
4. While capturing, power-cycle each half 10+ times (switch off 3 s, on, wait ~10 s for it to connect). Each boot gives one RSSI sample.
5. Analyse: `python3 scripts/split-rssi.py range.log`
6. Repeat with a half held at its worst realistic spot (far edge of desk, behind the hand, body in between).

## Reading it
- RSSI is the half's advertisement as heard by the central, at the half's TX power at that time (+8 dBm today, `BT_CTLR_TX_PWR_PLUS_8` in `config/corneish_zen.conf`)
- Margin = RSSI at candidate TX - receiver sensitivity (-95 dBm on 1M, -92 on 2M). Table flags margins under 20 dB
- Rule of thumb: keep >= 20 dB at the worst sample. If 0 dBm still shows >= 20 dB, lowering TX on the halves is safe and saves battery
- Change TX per artifact with `-DCONFIG_BT_CTLR_TX_PWR_PLUS_8=n -DCONFIG_BT_CTLR_TX_PWR_0=y` (or `_MINUS_4`, `_MINUS_8`) in `build.yaml` cmake-args

## Limits
- Advertising RSSI, not live link RSSI; ZMK does not log RSSI for an established connection
- A half that re-pairs from a bond uses directed advertising: its RSSI may not be logged. Sample from a fresh/unpaired boot if a half shows 0 samples
- Assumes advertising uses the same TX power as the connection
- Walls, the Mac's USB-C dock and your body move RSSI by 10+ dB: measure in place
