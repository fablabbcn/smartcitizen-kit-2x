# SCK 2.3 PM adapter (SEN5x / SEN6x)

Small board that replaces the Urban board and connects a Sensirion SEN5x or SEN6x to the SCK 2.3 Data board. KiCad 10 project.

- **J1**: 2x8 SMD male header, same footprint, position and pin numbering as the Urban board 2.3 `CONN_DATA` (bottom side).
- **J2**: JST GH 6 pin `SM06B-GHS-TB`, same footprint and position as the Urban board 2.3 `PM_SENSOR` (bottom side). The footprint is copied from the Eagle design because the KiCad library one is its mirror image, so the existing cables keep the same pinout.
- Same outline and holes as the Urban board 2.3, so it fits in the same place.

## Power

The PM connector on the Urban board gets 5 V from the Data board step-up (ETA1036-50, enabled by PA06 `pinPM_ENABLE`). The SEN6x needs 3.15–3.6 V (3.6 V absolute maximum), so it can't go on that line. Disabling the step-up doesn't help: its output is disconnected in shutdown, so the line goes to 0 V.

**JP1** selects the sensor supply. Set it before plugging in the sensor: 5 V destroys a SEN6x.

| JP1 | Supply | Sensor |
|-----|--------|--------|
| 1-2 | +5 V (step-up) | SEN5x (4.5–5.5 V) |
| 2-3 | 3V3 | SEN6x |

The 3V3 comes from one of two sources. Fit only one of them:

- **R1 (default): AP7361C-33E LDO from the switched 5 V.** The firmware can still power the sensor off with PA06, and the sensor is isolated from the Wi-Fi current bursts on the 3V3 rail. Dissipation is about 0.2 W at 110 mA.
- **R2 (not fitted): the Data board 3V3 rail (LD39200, 2 A, behind a 1.5 A fuse).** This has enough current for the SEN66's 350 mA peaks. However, the sensor is then always powered, and the rail is shared with the ESP8266. The SEN66 allows only 30 mV of supply ripple, so this option is there to measure whether the shared rail is clean enough.

I2C pull-ups (10k to 3V3) are on the Data board. The SEN6x and SEN5x I2C pins are 5 V tolerant.

## Firmware

There is no SEN6x support in the firmware yet. Without the Urban board, the Urban sensors are not detected and are skipped.

## Files

- `SCK_PM_ADAPTER_2.3.kicad_sch`, `.kicad_pcb`, `.kicad_pro`, `SCK_PM_ADAPTER.pretty/` (J2 footprint)
- `SCK_PM_ADAPTER_2.3.pdf`: schematic
- `SCK_PM_ADAPTER_2.3_PRODUCTION.zip`: gerbers, drill, BOM, positions

Status: v0.1, not manufactured or tested yet.
