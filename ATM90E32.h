/* ATM90E32 Energy Monitor Functions

The MIT License (MIT)

  Copyright (c) 2016 whatnick,Ryzee and Arun

  Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ATM90E32_h
#define ATM90E32_h

#include <Arduino.h>
#include <SPI.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <Preferences.h>
#endif

#define WRITE 0
#define READ 1

/* STATUS REGISTERS */
#define MeterEn 0x00       // Metering Enable
#define ChannelMapI 0x01   // Current Channel Mapping Configuration
#define ChannelMapU 0x02   // Voltage Channel Mapping Configuration
#define SagPeakDetCfg 0x05 // Sag and Peak Detector Period Configuration
#define OVth 0x06          // Over Voltage Threshold
#define ZXConfig 0x07      // Zero-Crossing Config
#define SagTh 0x08         // Voltage Sag Threshold
#define PhaseLossTh 0x09   // Voltage Phase Losing Threshold
#define INWarnTh 0x0A      // Neutral Current Warning Threshold
#define OIth 0x0B          // Over Current Threshold
#define FreqLoTh 0x0C      // Low Threshold for Frequency Detection
#define FreqHiTh 0x0D      // High Threshold for Frequency Detection
#define PMPwrCtrl 0x0E     // Partial Measurement Mode Power Control
#define IRQ0MergeCfg 0x0F  // IRQ0 Merge Configuration

/* EMM STATUS REGISTERS */
#define SoftReset 0x70    // Software Reset
#define EMMState0 0x71    // EMM State 0
#define EMMState1 0x72    // EMM State 1
#define EMMIntState0 0x73 // EMM Interrupt Status 0
#define EMMIntState1 0x74 // EMM Interrupt Status 1
#define EMMIntEn0 0x75    // EMM Interrupt Enable 0
#define EMMIntEn1 0x76    // EMM Interrupt Enable 1
#define LastSPIData 0x78  // Last Read/Write SPI Value
#define CRCErrStatus 0x79 // CRC Error Status
#define CRCDigest 0x7A    // CRC Digest
#define CfgRegAccEn 0x7F  // Configure Register Access Enable

/* EMM STATE 0 FLAGS */
#define EMMSTATE0_OIPHASEA 0x8000 // Over current on phase A
#define EMMSTATE0_OIPHASEB 0x4000 // Over current on phase B
#define EMMSTATE0_OIPHASEC 0x2000 // Over current on phase C
#define EMMSTATE0_OVPHASEA 0x1000 // Over voltage on phase A
#define EMMSTATE0_OVPHASEB 0x0800 // Over voltage on phase B
#define EMMSTATE0_OVPHASEC 0x0400 // Over voltage on phase C

/* EMM STATE 1 FLAGS */
#define EMMSTATE1_FREQHI 0x8000    // Frequency is above the high threshold
#define EMMSTATE1_SAGPHASEA 0x4000 // Voltage sag on phase A
#define EMMSTATE1_SAGPHASEB 0x2000 // Voltage sag on phase B
#define EMMSTATE1_SAGPHASEC 0x1000 // Voltage sag on phase C
#define EMMSTATE1_FREQLO 0x0800    // Frequency is below the low threshold
#define EMMSTATE1_PHASELOSSA 0x0400 // Phase loss on phase A
#define EMMSTATE1_PHASELOSSB 0x0200 // Phase loss on phase B
#define EMMSTATE1_PHASELOSSC 0x0100 // Phase loss on phase C

/* LOW POWER MODE REGISTERS - NOT USED */
#define DetectCtrl 0x10 // Detection Control
#define DetectTh1 0x11  // Detection Threshold 1
#define DetectTh2 0x12  // Detection Threshold 2
#define DetectTh3 0x13  // Detection Threshold 3
#define PMOffsetA 0x14  // Partial Measurement Offset A
#define PMOffsetB 0x15  // Partial Measurement Offset B
#define PMOffsetC 0x16  // Partial Measurement Offset C
#define PMPGA 0x17      // Partial Measurement PGA
#define PMIrmsA 0x18    // Partial Measurement RMS Current A
#define PMIrmsB 0x19    // Partial Measurement RMS Current B
#define PMIrmsC 0x1A    // Partial Measurement RMS Current C
#define PMConfig 0x10B  // Partial Measurement Configuration
#define PMAvgSamples 0x1C // Partial Measurement Average Samples
#define PMIrmsLSB 0x1D  // Partial Measurement RMS Current LSB

/* CONFIGURATION REGISTERS */
#define PLconstH 0x31 // High Word of PL Constant
#define PLconstL 0x32 // Low Word of PL Constant
#define MMode0 0x33   // Metering Mode Config
#define MMode1 0x34   // PGA Gain Configuration for Current Channels
#define PStartTh 0x35 // Startup Power Threshold (P)
#define QStartTh 0x36 // Startup Power Threshold (Q)
#define SStartTh 0x37 // Startup Power Threshold (S)
#define PPhaseTh 0x38 // Startup Power Accum Threshold (P)
#define QPhaseTh 0x39 // Startup Power Accum Threshold (Q)
#define SPhaseTh 0x3A // Startup Power Accum Threshold (S)

/* CALIBRATION REGISTERS */
#define PoffsetA 0x41 // A Line Power Offset (P)
#define QoffsetA 0x42 // A Line Power Offset (Q)
#define PoffsetB 0x43 // B Line Power Offset (P)
#define QoffsetB 0x44 // B Line Power Offset (Q)
#define PoffsetC 0x45 // C Line Power Offset (P)
#define QoffsetC 0x46 // C Line Power Offset (Q)
#define PQGainA 0x47  // A Line Calibration Gain
#define PhiA 0x48     // A Line Calibration Angle
#define PQGainB 0x49  // B Line Calibration Gain
#define PhiB 0x4A     // B Line Calibration Angle
#define PQGainC 0x4B  // C Line Calibration Gain
#define PhiC 0x4C     // C Line Calibration Angle

/* FUNDAMENTAL/HARMONIC ENERGY CALIBRATION REGISTERS */
#define POffsetAF 0x51 // A Fundamental Power Offset (P)
#define POffsetBF 0x52 // B Fundamental Power Offset (P)
#define POffsetCF 0x53 // C Fundamental Power Offset (P)
#define PGainAF 0x54   // A Fundamental Power Gain (P)
#define PGainBF 0x55   // B Fundamental Power Gain (P)
#define PGainCF 0x56   // C Fundamental Power Gain (P)

/* MEASUREMENT CALIBRATION REGISTERS */
#define UgainA 0x61   // A Voltage RMS Gain
#define IgainA 0x62   // A Current RMS Gain
#define UoffsetA 0x63 // A Voltage Offset
#define IoffsetA 0x64 // A Current Offset
#define UgainB 0x65   // B Voltage RMS Gain
#define IgainB 0x66   // B Current RMS Gain
#define UoffsetB 0x67 // B Voltage Offset
#define IoffsetB 0x68 // B Current Offset
#define UgainC 0x69   // C Voltage RMS Gain
#define IgainC 0x6A   // C Current RMS Gain
#define UoffsetC 0x6B // C Voltage Offset
#define IoffsetC 0x6C // C Current Offset
#define IoffsetN 0x6E // Neutral Current Offset

/* ENERGY REGISTERS */
#define APenergyT 0x80 // Total Forward Active Energy
#define APenergyA 0x81 // A Forward Active Energy
#define APenergyB 0x82 // B Forward Active Energy
#define APenergyC 0x83 // C Forward Active Energy
#define ANenergyT 0x84 // Total Reverse Active Energy
#define ANenergyA 0x85 // A Reverse Active Energy
#define ANenergyB 0x86 // B Reverse Active Energy
#define ANenergyC 0x87 // C Reverse Active Energy
#define RPenergyT 0x88 // Total Forward Reactive Energy
#define RPenergyA 0x89 // A Forward Reactive Energy
#define RPenergyB 0x8A // B Forward Reactive Energy
#define RPenergyC 0x8B // C Forward Reactive Energy
#define RNenergyT 0x8C // Total Reverse Reactive Energy
#define RNenergyA 0x8D // A Reverse Reactive Energy
#define RNenergyB 0x8E // B Reverse Reactive Energy
#define RNenergyC 0x8F // C Reverse Reactive Energy

#define SAenergyT 0x90 // Total Apparent Energy
#define SenergyA 0x91  // A Apparent Energy
#define SenergyB 0x92  // B Apparent Energy
#define SenergyC 0x93  // C Apparent Energy

/* FUNDAMENTAL / HARMONIC ENERGY REGISTERS */
#define APenergyTF 0xA0 // Total Forward Fundamental Energy
#define APenergyAF 0xA1 // A Forward Fundamental Energy
#define APenergyBF 0xA2 // B Forward Fundamental Energy
#define APenergyCF 0xA3 // C Forward Fundamental Energy
#define ANenergyTF 0xA4 // Total Reverse Fundamental Energy
#define ANenergyAF 0xA5 // A Reverse Fundamental Energy
#define ANenergyBF 0xA6 // B Reverse Fundamental Energy
#define ANenergyCF 0xA7 // C Reverse Fundamental Energy
#define APenergyTH 0xA8 // Total Forward Harmonic Energy
#define APenergyAH 0xA9 // A Forward Harmonic Energy
#define APenergyBH 0xAA // B Forward Harmonic Energy
#define APenergyCH 0xAB // C Forward Harmonic Energy
#define ANenergyTH 0xAC // Total Reverse Harmonic Energy
#define ANenergyAH 0xAD // A Reverse Harmonic Energy
#define ANenergyBH 0xAE // B Reverse Harmonic Energy
#define ANenergyCH 0xAF // C Reverse Harmonic Energy

/* POWER & P.F. REGISTERS */
#define PmeanT 0xB0  // Total Mean Power (P)
#define PmeanA 0xB1  // A Mean Power (P)
#define PmeanB 0xB2  // B Mean Power (P)
#define PmeanC 0xB3  // C Mean Power (P)
#define QmeanT 0xB4  // Total Mean Power (Q)
#define QmeanA 0xB5  // A Mean Power (Q)
#define QmeanB 0xB6  // B Mean Power (Q)
#define QmeanC 0xB7  // C Mean Power (Q)
#define SmeanT 0xB8  // Total Mean Power (S)
#define SmeanA 0xB9  // A Mean Power (S)
#define SmeanB 0xBA  // B Mean Power (S)
#define SmeanC 0xBB  // C Mean Power (S)
#define PFmeanT 0xBC // Total Mean Power Factor
#define PFmeanA 0xBD // A Power Factor
#define PFmeanB 0xBE // B Power Factor
#define PFmeanC 0xBF // C Power Factor

#define PmeanTLSB 0xC0 // Lower Word of Total Active Power
#define PmeanALSB 0xC1 // Lower Word of A Active Power
#define PmeanBLSB 0xC2 // Lower Word of B Active Power
#define PmeanCLSB 0xC3 // Lower Word of C Active Power
#define QmeanTLSB 0xC4 // Lower Word of Total Reactive Power
#define QmeanALSB 0xC5 // Lower Word of A Reactive Power
#define QmeanBLSB 0xC6 // Lower Word of B Reactive Power
#define QmeanCLSB 0xC7 // Lower Word of C Reactive Power
#define SAmeanTLSB 0xC8 // Lower Word of Total Apparent Power
#define SmeanALSB 0xC9 // Lower Word of A Apparent Power
#define SmeanBLSB 0xCA // Lower Word of B Apparent Power
#define SmeanCLSB 0xCB // Lower Word of C Apparent Power

/* FUND/HARM POWER & V/I RMS REGISTERS */
#define PmeanTF 0xD0 // Total Active Fundamental Power
#define PmeanAF 0xD1 // A Active Fundamental Power
#define PmeanBF 0xD2 // B Active Fundamental Power
#define PmeanCF 0xD3 // C Active Fundamental Power
#define PmeanTH 0xD4 // Total Active Harmonic Power
#define PmeanAH 0xD5 // A Active Harmonic Power
#define PmeanBH 0xD6 // B Active Harmonic Power
#define PmeanCH 0xD7 // C Active Harmonic Power
#define UrmsA 0xD9   // A RMS Voltage
#define UrmsB 0xDA   // B RMS Voltage
#define UrmsC 0xDB   // C RMS Voltage
#define IrmsN 0xDC   // Neutral RMS Current
#define IrmsA 0xDD   // A RMS Current
#define IrmsB 0xDE   // B RMS Current
#define IrmsC 0xDF   // C RMS Current

#define PmeanTFLSB 0xE0 // Lower Word of Total Active Fundamental Power
#define PmeanAFLSB 0xE1 // Lower Word of A Active Fundamental Power
#define PmeanBFLSB 0xE2 // Lower Word of B Active Fundamental Power
#define PmeanCFLSB 0xE3 // Lower Word of C Active Fundamental Power
#define PmeanTHLSB 0xE4 // Lower Word of Total Active Harmonic Power
#define PmeanAHLSB 0xE5 // Lower Word of A Active Harmonic Power
#define PmeanBHLSB 0xE6 // Lower Word of B Active Harmonic Power
#define PmeanCHLSB 0xE7 // Lower Word of C Active Harmonic Power
#define UrmsALSB 0xE9   // Lower Word of A RMS Voltage
#define UrmsBLSB 0xEA   // Lower Word of B RMS Voltage
#define UrmsCLSB 0xEB   // Lower Word of C RMS Voltage
#define IrmsALSB 0xED   // Lower Word of A RMS Current
#define IrmsBLSB 0xEE   // Lower Word of B RMS Current
#define IrmsCLSB 0xEF   // Lower Word of C RMS Current

/* PEAK, FREQUENCY, ANGLE & TEMP REGISTERS */
#define UPeakA 0xF1  // A Voltage Peak
#define UPeakB 0xF2  // B Voltage Peak
#define UPeakC 0xF3  // C Voltage Peak
#define IPeakA 0xF5  // A Current Peak
#define IPeakB 0xF6  // B Current Peak
#define IPeakC 0xF7  // C Current Peak
#define Freq 0xF8    // Frequency
#define PAngleA 0xF9 // A Mean Phase Angle
#define PAngleB 0xFA // B Mean Phase Angle
#define PAngleC 0xFB // C Mean Phase Angle
#define Temp 0xFC    // Measured Temperature
#define UangleA 0xFD // A Voltage Phase Angle
#define UangleB 0xFE // B Voltage Phase Angle
#define UangleC 0xFF // C Voltage Phase Angle

class ATM90E32 {
 public:
  enum Phase : uint8_t {
    PHASE_A = 0,
    PHASE_B = 1,
    PHASE_C = 2
  };

  enum LineFrequency : uint8_t {
    LINE_FREQUENCY_50HZ = 50,
    LINE_FREQUENCY_60HZ = 60
  };

  enum CurrentPhases : uint8_t {
    CURRENT_PHASES_2 = 2,
    CURRENT_PHASES_3 = 3
  };

  enum PgaGain : uint16_t {
    PGA_GAIN_1X = 0x0000,
    PGA_GAIN_2X = 0x0015,
    PGA_GAIN_4X = 0x002A
  };

  enum FrequencyStatus : uint8_t {
    FREQUENCY_STATUS_NORMAL = 0,
    FREQUENCY_STATUS_LOW = 1,
    FREQUENCY_STATUS_HIGH = 2
  };

  struct PhaseStatus {
    bool overVoltage;
    bool voltageSag;
    bool phaseLoss;
    bool overCurrent;

    PhaseStatus()
        : overVoltage(false), voltageSag(false), phaseLoss(false), overCurrent(false) {}

    bool IsOk() const {
      return !overVoltage && !voltageSag && !phaseLoss && !overCurrent;
    }
  };

  struct PhaseConfig {
    uint16_t voltageGain;
    uint16_t currentGain;
    int16_t voltageOffset;
    int16_t currentOffset;
    int16_t activePowerOffset;
    int16_t reactivePowerOffset;
    float referenceVoltage;
    float referenceCurrent;

    PhaseConfig()
        : voltageGain(7305),
          currentGain(27961),
          voltageOffset(0),
          currentOffset(0),
          activePowerOffset(0),
          reactivePowerOffset(0),
          referenceVoltage(120.0f),
          referenceCurrent(5.0f) {}
  };

  struct Config {
    int csPin;
    LineFrequency lineFrequency;
    CurrentPhases currentPhases;
    PgaGain pgaGain;
    bool peakCurrentSigned;
    bool enableOffsetCalibration;
    bool enableGainCalibration;
    PhaseConfig phase[3];

    Config()
        : csPin(-1),
          lineFrequency(LINE_FREQUENCY_60HZ),
          currentPhases(CURRENT_PHASES_3),
          pgaGain(PGA_GAIN_1X),
          peakCurrentSigned(false),
          enableOffsetCalibration(false),
          enableGainCalibration(false) {}
  };

  ATM90E32(void);
  ~ATM90E32(void);

  void begin(int pin, unsigned short lineFreq, unsigned short pgagain, unsigned short ugain,
             unsigned short igainA, unsigned short igainB, unsigned short igainC);
  bool begin(const Config &config);

  double CalculateVIOffset(unsigned short regh_addr, unsigned short regl_addr);
  double CalculatePowerOffset(unsigned short regh_addr, unsigned short regl_addr);
  double CalibrateVI(unsigned short reg, unsigned short actualVal);
  uint16_t CalculateVoltageThreshold(LineFrequency lineFrequency, uint16_t voltageGain, float multiplier) const;

  void SetReferenceVoltage(Phase phase, float referenceVoltage);
  void SetReferenceCurrent(Phase phase, float referenceCurrent);
  void SetPeakCurrentSigned(bool peakCurrentSigned);
  void SetEnableOffsetCalibration(bool enableOffsetCalibration);
  void SetEnableGainCalibration(bool enableGainCalibration);
  void ResetEnergyAccumulators();

  bool RunGainCalibration();
  bool RunOffsetCalibration();
  bool RunPowerOffsetCalibration();
  bool ClearGainCalibration();
  bool ClearOffsetCalibration();
  bool ClearPowerOffsetCalibration();

  double GetLineVoltageA();
  double GetLineVoltageB();
  double GetLineVoltageC();

  double GetLineCurrentA();
  double GetLineCurrentB();
  double GetLineCurrentC();
  double GetLineCurrentN();

  double GetActivePowerA();
  double GetActivePowerB();
  double GetActivePowerC();
  double GetTotalActivePower();

  double GetTotalActiveFundPower();
  double GetTotalActiveHarPower();
  double GetActiveHarPowerA();
  double GetActiveHarPowerB();
  double GetActiveHarPowerC();
  double GetHarmonicActivePowerA();
  double GetHarmonicActivePowerB();
  double GetHarmonicActivePowerC();

  double GetReactivePowerA();
  double GetReactivePowerB();
  double GetReactivePowerC();
  double GetTotalReactivePower();

  double GetApparentPowerA();
  double GetApparentPowerB();
  double GetApparentPowerC();
  double GetTotalApparentPower();

  double GetFrequency();

  double GetPowerFactorA();
  double GetPowerFactorB();
  double GetPowerFactorC();
  double GetTotalPowerFactor();

  double GetPhaseA();
  double GetPhaseB();
  double GetPhaseC();
  double GetSignedPhaseA();
  double GetSignedPhaseB();
  double GetSignedPhaseC();
  double GetSignedPhaseAngleA();
  double GetSignedPhaseAngleB();
  double GetSignedPhaseAngleC();

  double GetPeakCurrentA();
  double GetPeakCurrentB();
  double GetPeakCurrentC();

  double GetTemperature();

  double GetValueRegister(unsigned short registerRead);

  double GetImportEnergy();
  double GetImportReactiveEnergy();
  double GetImportApparentEnergy();
  double GetExportEnergy();
  double GetExportReactiveEnergy();
  double GetImportEnergyA();
  double GetImportEnergyB();
  double GetImportEnergyC();
  double GetExportEnergyA();
  double GetExportEnergyB();
  double GetExportEnergyC();
  double GetForwardActiveEnergyA();
  double GetForwardActiveEnergyB();
  double GetForwardActiveEnergyC();
  double GetReverseActiveEnergyA();
  double GetReverseActiveEnergyB();
  double GetReverseActiveEnergyC();

  PhaseStatus GetPhaseStatus(Phase phase);
  String GetPhaseStatusText(Phase phase);
  FrequencyStatus GetFrequencyStatus();
  String GetFrequencyStatusText();

  unsigned short GetSysStatus0();
  unsigned short GetSysStatus1();
  unsigned short GetMeterStatus0();
  unsigned short GetMeterStatus1();

 private:
  struct RuntimePhaseState {
    uint16_t voltageGain;
    uint16_t currentGain;
    int16_t voltageOffset;
    int16_t currentOffset;
    int16_t activePowerOffset;
    int16_t reactivePowerOffset;
    float referenceVoltage;
    float referenceCurrent;
    uint32_t cumulativeForwardActiveEnergy;
    uint32_t cumulativeReverseActiveEnergy;

    RuntimePhaseState()
        : voltageGain(7305),
          currentGain(27961),
          voltageOffset(0),
          currentOffset(0),
          activePowerOffset(0),
          reactivePowerOffset(0),
          referenceVoltage(120.0f),
          referenceCurrent(5.0f),
          cumulativeForwardActiveEnergy(0),
          cumulativeReverseActiveEnergy(0) {}
  };

  struct GainCalibrationStore {
    uint16_t voltageGain;
    uint16_t currentGain;
  };

  struct OffsetCalibrationStore {
    int16_t voltageOffset;
    int16_t currentOffset;
  };

  struct PowerOffsetCalibrationStore {
    int16_t activePowerOffset;
    int16_t reactivePowerOffset;
  };

#if defined(ARDUINO_ARCH_ESP32)
  struct GainBlob {
    uint32_t magic;
    GainCalibrationStore phase[3];
  };

  struct OffsetBlob {
    uint32_t magic;
    OffsetCalibrationStore phase[3];
  };

  struct PowerOffsetBlob {
    uint32_t magic;
    PowerOffsetCalibrationStore phase[3];
  };
#endif

  unsigned short CommEnergyIC(unsigned char rw, unsigned short address, unsigned short value);
  uint16_t Read16Register(uint16_t reg);
  int32_t Read32Register(uint16_t regHigh, uint16_t regLow);
  void Write16Register(uint16_t reg, uint16_t value, bool validate = false);
  bool ValidateLastSPIData(uint16_t expected);
  void BeginConfiguration();
  void EndConfiguration();
  bool BeginInternal(uint16_t mmode0, uint16_t sagPeakConfig, uint16_t freqHighThreshold,
                     uint16_t freqLowThreshold, uint16_t sagThreshold, uint16_t overVoltageThreshold);
  void PopulateRuntimeFromConfig();
  void ApplyRuntimeCalibrationToRegisters();
  void ApplyGainRegisters();
  void ApplyOffsetRegisters(Phase phase);
  void ApplyPowerOffsetRegisters(Phase phase);
  bool VerifyGainWrites();
  void ClearRuntimeToConfigGains();
  void ClearRuntimeToConfigOffsets();
  void ClearRuntimeToConfigPowerOffsets();
  void SetPhaseRuntimeFromConfig(Phase phase);
  uint16_t GetVoltageGainRegister(Phase phase) const;
  uint16_t GetCurrentGainRegister(Phase phase) const;
  uint16_t GetVoltageOffsetRegister(Phase phase) const;
  uint16_t GetCurrentOffsetRegister(Phase phase) const;
  uint16_t GetPowerOffsetRegister(Phase phase) const;
  uint16_t GetReactivePowerOffsetRegister(Phase phase) const;
  uint16_t GetVoltageRegister(Phase phase) const;
  uint16_t GetVoltageLowRegister(Phase phase) const;
  uint16_t GetCurrentRegister(Phase phase) const;
  uint16_t GetCurrentLowRegister(Phase phase) const;
  uint16_t GetActivePowerRegister(Phase phase) const;
  uint16_t GetActivePowerLowRegister(Phase phase) const;
  uint16_t GetReactivePowerRegister(Phase phase) const;
  uint16_t GetReactivePowerLowRegister(Phase phase) const;
  uint16_t GetApparentPowerRegister(Phase phase) const;
  uint16_t GetApparentPowerLowRegister(Phase phase) const;
  uint16_t GetPowerFactorRegister(Phase phase) const;
  uint16_t GetPhaseAngleRegister(Phase phase) const;
  uint16_t GetHarmonicActivePowerRegister(Phase phase) const;
  uint16_t GetHarmonicActivePowerLowRegister(Phase phase) const;
  uint16_t GetPeakCurrentRegister(Phase phase) const;
  uint16_t GetForwardActiveEnergyRegister(Phase phase) const;
  uint16_t GetReverseActiveEnergyRegister(Phase phase) const;
  const char *GetPhaseLabel(Phase phase) const;
  double GetLineVoltage(Phase phase);
  double GetLineCurrent(Phase phase);
  double GetActivePower(Phase phase);
  double GetReactivePower(Phase phase);
  double GetApparentPower(Phase phase);
  double GetPowerFactor(Phase phase);
  double GetPhaseRaw(Phase phase);
  double GetPhaseSigned(Phase phase);
  double GetHarmonicActivePower(Phase phase);
  double GetPeakCurrent(Phase phase);
  double GetAccumulatedEnergy(Phase phase, uint16_t reg, uint32_t &counter);
  double GetAverageVoltage(Phase phase, uint8_t sampleCount);
  double GetAverageCurrent(Phase phase, uint8_t sampleCount);
  int16_t CalibrateOffset(Phase phase, bool voltage);
  int16_t CalibratePowerOffset(Phase phase, bool reactive);
  bool ShouldUseLegacy120VThresholds(uint16_t legacyMMode0) const;

#if defined(ARDUINO_ARCH_ESP32)
  bool LoadGainPreferences();
  bool LoadOffsetPreferences();
  bool LoadPowerOffsetPreferences();
  bool SaveGainPreferences();
  bool SaveOffsetPreferences();
  bool SavePowerOffsetPreferences();
  bool RemovePreferenceKey(const char *key);
  void BuildPreferenceKey(char *buffer, size_t bufferSize, const char *prefix) const;
#endif

  int _cs;
  uint16_t _legacyLineFrequencyRaw;
  bool _usingLegacyBegin;
  bool _begun;
  Config _config;
  RuntimePhaseState _phaseState[3];
};

#endif
