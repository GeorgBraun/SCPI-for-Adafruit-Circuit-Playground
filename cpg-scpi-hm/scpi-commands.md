# List of currently supported scpi commands

**Attention: Circuit Playground Express is currently not supported!**

```
*IDN?
*RST


OUT:LED:RED <1/0>
OUT:LED <VALUE>
OUT:DEMO:LED


SYST:CON?

SYST:CON:TIMESTAMP <OFF/MS>
SYST:CON:MEAS:TINT <VALUE>
SYST:CON:MEAS:COUNT <-1..VALUE>
SYST:CON:MEAS:TYPE <SI/RAW>
SYST:CON:MEAS:CAPLIM <VALUE>
SYST:CON:LED:COL <VALUE>


MEAS:STOP

MEAS:BUTTON?
MEAS:BUTTON:RIGHT?
MEAS:BUTTON:LEFT?
MEAS:SWITCH?
MEAS:TEMP?
MEAS:ACC?
MEAS:LIGHT? // only RAW values
MEAS:SOUND? // only RAW values
MEAS:CAP:SENSE? // Individual values from 8 cap sensors
MEAS:CAP:TAP?   // Single int value with one bit per cap sensor
                // 0-1-threshold is defined via SYST:CON:LED:CAPLIM

MEAS:TIME?
```
