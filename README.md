# ATM90E32 Arduino Library

Arduino library for energy meters built around the Microchip ATM90E32AS. It powers CircuitSetup split-phase and expandable energy meter boards and now includes an ESPHome-parity API for config-driven setup, per-phase energy counters, calibration helpers, and decoded status reporting.

## Highlights

- Keeps the legacy `begin(...)` API for existing sketches.
- Adds `ATM90E32::Config` and per-phase configuration for ESPHome-style setup.
- Supports 50 Hz / 60 Hz setup, 2-phase / 3-phase mode, harmonic power, peak current, and signed phase angles.
- Adds cumulative per-phase forward/reverse active energy in Wh.
- Adds phase and frequency status helpers.
- Adds gain, voltage/current offset, and power offset calibration flows.
- Saves calibration data on ESP32 with `Preferences`; other Arduino targets keep calibration runtime-only.

## Legacy Setup

Existing sketches can keep using the original `begin(...)` signature:

```cpp
#include <ATM90E32.h>

ATM90E32 energyMeter;

void setup() {
  energyMeter.begin(CS_pin, lineFreq, PGAGain, VoltageGain, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
}
```

## Config-Driven Setup

The new API mirrors ESPHome's configuration model more closely:

```cpp
#include <ATM90E32.h>

ATM90E32 energyMeter;

void setup() {
  ATM90E32::Config config;
  config.csPin = 5;
  config.lineFrequency = ATM90E32::LINE_FREQUENCY_60HZ;
  config.currentPhases = ATM90E32::CURRENT_PHASES_2;
  config.pgaGain = ATM90E32::PGA_GAIN_1X;
  config.enableGainCalibration = true;
  config.enableOffsetCalibration = true;

  config.phase[ATM90E32::PHASE_A].voltageGain = 7305;
  config.phase[ATM90E32::PHASE_A].currentGain = 27961;
  config.phase[ATM90E32::PHASE_A].referenceVoltage = 120.0f;
  config.phase[ATM90E32::PHASE_A].referenceCurrent = 5.0f;

  config.phase[ATM90E32::PHASE_B].voltageGain = 7305;
  config.phase[ATM90E32::PHASE_B].currentGain = 27961;

  config.phase[ATM90E32::PHASE_C].voltageGain = 7305;
  config.phase[ATM90E32::PHASE_C].currentGain = 27961;

  if (!energyMeter.begin(config)) {
    Serial.println("ATM90E32 init failed");
  }
}
```

## Common Measurements

```cpp
void loop() {
  double voltageA = energyMeter.GetLineVoltageA();
  double currentA = energyMeter.GetLineCurrentA();
  double powerA = energyMeter.GetActivePowerA();
  double totalPower = energyMeter.GetTotalActivePower();
  double harmonicPowerA = energyMeter.GetHarmonicActivePowerA();
  double peakCurrentA = energyMeter.GetPeakCurrentA();
  double signedAngleA = energyMeter.GetSignedPhaseAngleA();
}
```

Other available getters include:

- `GetReactivePowerA/B/C()` and `GetTotalReactivePower()`
- `GetApparentPowerA/B/C()` and `GetTotalApparentPower()`
- `GetPowerFactorA/B/C()` and `GetTotalPowerFactor()`
- `GetPhaseA/B/C()` for raw phase angle and `GetSignedPhaseAngleA/B/C()` for `[-180, 180]`
- `GetTotalActiveFundPower()` and `GetTotalActiveHarPower()`
- `GetFrequency()`
- `GetTemperature()`

## Energy Counters

Total energy getters keep the legacy behavior and return kWh from the ATM90E32 total registers:

- `GetImportEnergy()`
- `GetImportReactiveEnergy()`
- `GetImportApparentEnergy()`
- `GetExportEnergy()`
- `GetExportReactiveEnergy()`

Per-phase forward/reverse active energy getters are cumulative Wh counters built on the chip's read-and-clear phase registers:

- `GetForwardActiveEnergyA/B/C()`
- `GetReverseActiveEnergyA/B/C()`
- `GetImportEnergyA/B/C()`
- `GetExportEnergyA/B/C()`

You can clear the cumulative per-phase counters in software with:

- `ResetEnergyAccumulators()`

## Status Helpers

Raw status registers are still available:

- `GetSysStatus0()`
- `GetSysStatus1()`
- `GetMeterStatus0()`
- `GetMeterStatus1()`

The new decoded helpers are:

- `GetPhaseStatus(ATM90E32::PHASE_A/B/C)`
- `GetPhaseStatusText(ATM90E32::PHASE_A/B/C)`
- `GetFrequencyStatus()`
- `GetFrequencyStatusText()`

## Calibration

Reference values can be updated in code:

```cpp
energyMeter.SetReferenceVoltage(ATM90E32::PHASE_A, 120.0f);
energyMeter.SetReferenceCurrent(ATM90E32::PHASE_A, 5.0f);
```

Calibration helpers:

- `RunGainCalibration()`
- `RunOffsetCalibration()`
- `RunPowerOffsetCalibration()`
- `ClearGainCalibration()`
- `ClearOffsetCalibration()`
- `ClearPowerOffsetCalibration()`

Legacy helpers are still present:

- `CalculateVIOffset(...)`
- `CalculatePowerOffset(...)`
- `CalibrateVI(...)`

On ESP32, the run/clear calibration methods save and restore calibration values with `Preferences`, keyed by the chip-select pin. On non-ESP32 Arduino targets, the same methods apply calibration values to the ATM90E32 for the current boot only.

## Example

See [examples/ESPHomeParityDemo/ESPHomeParityDemo.ino](examples/ESPHomeParityDemo/ESPHomeParityDemo.ino) for a full config-based sketch with serial-triggered calibration commands and status output.

## Raw Register Access

Any ATM90E32 register defined in `ATM90E32.h` can still be read directly:

```cpp
double raw = energyMeter.GetValueRegister(UrmsA);
```
