/* ATM90E32 Energy Monitor Functions

  The MIT License (MIT)

  Copyright (c) 2016 whatnick,Ryzee and Arun

  Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ATM90E32.h"

namespace {

const float ATM90E32_POWER_LSB = 0.00032f;
const float ATM90E32_ENERGY_COUNT_TO_WH = 10.0f / 3200.0f;
const float ATM90E32_TOTAL_ENERGY_TO_KWH = 1.0f / 100.0f / 3200.0f;
const float ATM90E32_SQRT2 = 1.41421356237f;
const float ATM90E32_OVER_CURRENT_THRESHOLD = 65.53f;

#if defined(ARDUINO_ARCH_ESP32)
const uint32_t ATM90E32_GAIN_MAGIC = 0x47414E31UL;
const uint32_t ATM90E32_OFFSET_MAGIC = 0x4F465331UL;
const uint32_t ATM90E32_POWER_OFFSET_MAGIC = 0x504F4631UL;
#endif

}  // namespace

ATM90E32::ATM90E32(void)
    : _cs(-1), _legacyLineFrequencyRaw(0), _usingLegacyBegin(false), _begun(false), _config() {
}

ATM90E32::~ATM90E32() {
}

unsigned short ATM90E32::CommEnergyIC(unsigned char rw, unsigned short address, unsigned short value) {
  unsigned char *data = reinterpret_cast<unsigned char *>(&value);
  unsigned char *addressData = reinterpret_cast<unsigned char *>(&address);
  unsigned short output;
  unsigned short swappedAddress;

#if defined(ESP8266) || defined(ARDUINO_ARCH_ESP8266)
  SPISettings settings(200000, MSBFIRST, SPI_MODE1);
#elif defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_SAMD)
  SPISettings settings(200000, MSBFIRST, SPI_MODE3);
#else
  SPISettings settings(200000, MSBFIRST, SPI_MODE0);
#endif

  output = (value >> 8) | (value << 8);
  value = output;

  address |= static_cast<unsigned short>(rw) << 15;
  swappedAddress = (address >> 8) | (address << 8);
  address = swappedAddress;

#if !defined(ENERGIA)
  SPI.beginTransaction(settings);
#endif

  digitalWrite(_cs, LOW);
  delayMicroseconds(10);

  for (byte i = 0; i < 2; i++) {
    SPI.transfer(*addressData);
    addressData++;
  }

  delayMicroseconds(4);

  if (rw) {
    for (byte i = 0; i < 2; i++) {
      *data = SPI.transfer(0x00);
      data++;
    }
  } else {
    for (byte i = 0; i < 2; i++) {
      SPI.transfer(*data);
      data++;
    }
  }

  digitalWrite(_cs, HIGH);
  delayMicroseconds(10);

#if !defined(ENERGIA)
  SPI.endTransaction();
#endif

  output = (value >> 8) | (value << 8);
  return output;
}

uint16_t ATM90E32::Read16Register(uint16_t reg) {
  return CommEnergyIC(READ, reg, 0xFFFF);
}

int32_t ATM90E32::Read32Register(uint16_t regHigh, uint16_t regLow) {
  const uint16_t valueHigh = Read16Register(regHigh);
  const uint16_t valueLow = Read16Register(regLow);
  return (static_cast<int32_t>(static_cast<int16_t>(valueHigh)) << 16) | static_cast<uint16_t>(valueLow);
}

void ATM90E32::Write16Register(uint16_t reg, uint16_t value, bool validate) {
  CommEnergyIC(WRITE, reg, value);
  if (validate) {
    ValidateLastSPIData(value);
  }
}

bool ATM90E32::ValidateLastSPIData(uint16_t expected) {
  return Read16Register(LastSPIData) == expected;
}

void ATM90E32::BeginConfiguration() {
  Write16Register(CfgRegAccEn, 0x55AA);
}

void ATM90E32::EndConfiguration() {
  Write16Register(CfgRegAccEn, 0x0000);
}

void ATM90E32::ResetEnergyAccumulators() {
  for (uint8_t phase = 0; phase < 3; phase++) {
    _phaseState[phase].cumulativeForwardActiveEnergy = 0;
    _phaseState[phase].cumulativeReverseActiveEnergy = 0;
  }
}

void ATM90E32::SetPhaseRuntimeFromConfig(Phase phase) {
  const uint8_t index = static_cast<uint8_t>(phase);
  _phaseState[index].voltageGain = _config.phase[index].voltageGain;
  _phaseState[index].currentGain = _config.phase[index].currentGain;
  _phaseState[index].voltageOffset = _config.phase[index].voltageOffset;
  _phaseState[index].currentOffset = _config.phase[index].currentOffset;
  _phaseState[index].activePowerOffset = _config.phase[index].activePowerOffset;
  _phaseState[index].reactivePowerOffset = _config.phase[index].reactivePowerOffset;
  _phaseState[index].referenceVoltage = _config.phase[index].referenceVoltage;
  _phaseState[index].referenceCurrent = _config.phase[index].referenceCurrent;
}

void ATM90E32::PopulateRuntimeFromConfig() {
  for (uint8_t phase = 0; phase < 3; phase++) {
    SetPhaseRuntimeFromConfig(static_cast<Phase>(phase));
  }
  ResetEnergyAccumulators();
}

void ATM90E32::ClearRuntimeToConfigGains() {
  for (uint8_t phase = 0; phase < 3; phase++) {
    _phaseState[phase].voltageGain = _config.phase[phase].voltageGain;
    _phaseState[phase].currentGain = _config.phase[phase].currentGain;
  }
}

void ATM90E32::ClearRuntimeToConfigOffsets() {
  for (uint8_t phase = 0; phase < 3; phase++) {
    _phaseState[phase].voltageOffset = _config.phase[phase].voltageOffset;
    _phaseState[phase].currentOffset = _config.phase[phase].currentOffset;
  }
}

void ATM90E32::ClearRuntimeToConfigPowerOffsets() {
  for (uint8_t phase = 0; phase < 3; phase++) {
    _phaseState[phase].activePowerOffset = _config.phase[phase].activePowerOffset;
    _phaseState[phase].reactivePowerOffset = _config.phase[phase].reactivePowerOffset;
  }
}

uint16_t ATM90E32::GetVoltageGainRegister(Phase phase) const {
  switch (phase) {
    case PHASE_A:
      return UgainA;
    case PHASE_B:
      return UgainB;
    default:
      return UgainC;
  }
}

uint16_t ATM90E32::GetCurrentGainRegister(Phase phase) const {
  switch (phase) {
    case PHASE_A:
      return IgainA;
    case PHASE_B:
      return IgainB;
    default:
      return IgainC;
  }
}

uint16_t ATM90E32::GetVoltageOffsetRegister(Phase phase) const {
  switch (phase) {
    case PHASE_A:
      return UoffsetA;
    case PHASE_B:
      return UoffsetB;
    default:
      return UoffsetC;
  }
}

uint16_t ATM90E32::GetCurrentOffsetRegister(Phase phase) const {
  switch (phase) {
    case PHASE_A:
      return IoffsetA;
    case PHASE_B:
      return IoffsetB;
    default:
      return IoffsetC;
  }
}

uint16_t ATM90E32::GetPowerOffsetRegister(Phase phase) const {
  switch (phase) {
    case PHASE_A:
      return PoffsetA;
    case PHASE_B:
      return PoffsetB;
    default:
      return PoffsetC;
  }
}

uint16_t ATM90E32::GetReactivePowerOffsetRegister(Phase phase) const {
  switch (phase) {
    case PHASE_A:
      return QoffsetA;
    case PHASE_B:
      return QoffsetB;
    default:
      return QoffsetC;
  }
}

uint16_t ATM90E32::GetVoltageRegister(Phase phase) const {
  return static_cast<uint16_t>(UrmsA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetVoltageLowRegister(Phase phase) const {
  return static_cast<uint16_t>(UrmsALSB + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetCurrentRegister(Phase phase) const {
  return static_cast<uint16_t>(IrmsA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetCurrentLowRegister(Phase phase) const {
  return static_cast<uint16_t>(IrmsALSB + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetActivePowerRegister(Phase phase) const {
  return static_cast<uint16_t>(PmeanA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetActivePowerLowRegister(Phase phase) const {
  return static_cast<uint16_t>(PmeanALSB + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetReactivePowerRegister(Phase phase) const {
  return static_cast<uint16_t>(QmeanA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetReactivePowerLowRegister(Phase phase) const {
  return static_cast<uint16_t>(QmeanALSB + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetApparentPowerRegister(Phase phase) const {
  return static_cast<uint16_t>(SmeanA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetApparentPowerLowRegister(Phase phase) const {
  return static_cast<uint16_t>(SmeanALSB + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetPowerFactorRegister(Phase phase) const {
  return static_cast<uint16_t>(PFmeanA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetPhaseAngleRegister(Phase phase) const {
  return static_cast<uint16_t>(PAngleA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetHarmonicActivePowerRegister(Phase phase) const {
  return static_cast<uint16_t>(PmeanAH + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetHarmonicActivePowerLowRegister(Phase phase) const {
  return static_cast<uint16_t>(PmeanAHLSB + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetPeakCurrentRegister(Phase phase) const {
  return static_cast<uint16_t>(IPeakA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetForwardActiveEnergyRegister(Phase phase) const {
  return static_cast<uint16_t>(APenergyA + static_cast<uint8_t>(phase));
}

uint16_t ATM90E32::GetReverseActiveEnergyRegister(Phase phase) const {
  return static_cast<uint16_t>(ANenergyA + static_cast<uint8_t>(phase));
}

const char *ATM90E32::GetPhaseLabel(Phase phase) const {
  switch (phase) {
    case PHASE_A:
      return "A";
    case PHASE_B:
      return "B";
    default:
      return "C";
  }
}

bool ATM90E32::VerifyGainWrites() {
  for (uint8_t phase = 0; phase < 3; phase++) {
    const uint16_t storedVoltageGain = Read16Register(GetVoltageGainRegister(static_cast<Phase>(phase)));
    const uint16_t storedCurrentGain = Read16Register(GetCurrentGainRegister(static_cast<Phase>(phase)));
    if (storedVoltageGain != _phaseState[phase].voltageGain ||
        storedCurrentGain != _phaseState[phase].currentGain) {
      return false;
    }
  }
  return true;
}

void ATM90E32::ApplyGainRegisters() {
  BeginConfiguration();
  for (uint8_t phase = 0; phase < 3; phase++) {
    Write16Register(GetVoltageGainRegister(static_cast<Phase>(phase)), _phaseState[phase].voltageGain);
    Write16Register(GetCurrentGainRegister(static_cast<Phase>(phase)), _phaseState[phase].currentGain);
  }
  EndConfiguration();
}

void ATM90E32::ApplyOffsetRegisters(Phase phase) {
  const uint8_t index = static_cast<uint8_t>(phase);
  BeginConfiguration();
  Write16Register(GetVoltageOffsetRegister(phase), static_cast<uint16_t>(_phaseState[index].voltageOffset));
  Write16Register(GetCurrentOffsetRegister(phase), static_cast<uint16_t>(_phaseState[index].currentOffset));
  EndConfiguration();
}

void ATM90E32::ApplyPowerOffsetRegisters(Phase phase) {
  const uint8_t index = static_cast<uint8_t>(phase);
  BeginConfiguration();
  Write16Register(GetPowerOffsetRegister(phase), static_cast<uint16_t>(_phaseState[index].activePowerOffset));
  Write16Register(GetReactivePowerOffsetRegister(phase), static_cast<uint16_t>(_phaseState[index].reactivePowerOffset));
  EndConfiguration();
}

void ATM90E32::ApplyRuntimeCalibrationToRegisters() {
  ApplyGainRegisters();
  for (uint8_t phase = 0; phase < 3; phase++) {
    ApplyOffsetRegisters(static_cast<Phase>(phase));
    ApplyPowerOffsetRegisters(static_cast<Phase>(phase));
  }
}

bool ATM90E32::ShouldUseLegacy120VThresholds(uint16_t legacyMMode0) const {
  return (legacyMMode0 & (1U << 12)) != 0;
}

bool ATM90E32::BeginInternal(uint16_t mmode0, uint16_t sagPeakConfig, uint16_t freqHighThreshold,
                             uint16_t freqLowThreshold, uint16_t sagThreshold, uint16_t overVoltageThreshold) {
  if (_cs < 0) {
    _begun = false;
    return false;
  }

  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  SPI.begin();

  Write16Register(SoftReset, 0x789A);
  delay(6);
  Write16Register(CfgRegAccEn, 0x55AA);
  if (!ValidateLastSPIData(0x55AA)) {
    _begun = false;
    EndConfiguration();
    return false;
  }

  Write16Register(MeterEn, 0x0001);
  Write16Register(SagPeakDetCfg, sagPeakConfig);
  Write16Register(SagTh, sagThreshold);
  Write16Register(OVth, overVoltageThreshold);
  Write16Register(FreqHiTh, freqHighThreshold);
  Write16Register(FreqLoTh, freqLowThreshold);
  Write16Register(EMMIntEn0, 0xB76F);
  Write16Register(EMMIntEn1, 0xDDFD);
  Write16Register(EMMIntState0, 0x0001);
  Write16Register(EMMIntState1, 0x0001);
  Write16Register(ZXConfig, 0xD654);

  Write16Register(PLconstH, 0x0861);
  Write16Register(PLconstL, 0xC468);
  Write16Register(MMode0, mmode0);
  Write16Register(MMode1, static_cast<uint16_t>(_config.pgaGain));
  Write16Register(PStartTh, 0x1D4C);
  Write16Register(QStartTh, 0x1D4C);
  Write16Register(SStartTh, 0x1D4C);
  Write16Register(PPhaseTh, 0x02EE);
  Write16Register(QPhaseTh, 0x02EE);
  Write16Register(SPhaseTh, 0x02EE);

  Write16Register(PQGainA, 0x0000);
  Write16Register(PhiA, 0x0000);
  Write16Register(PQGainB, 0x0000);
  Write16Register(PhiB, 0x0000);
  Write16Register(PQGainC, 0x0000);
  Write16Register(PhiC, 0x0000);

  Write16Register(POffsetAF, 0x0000);
  Write16Register(POffsetBF, 0x0000);
  Write16Register(POffsetCF, 0x0000);
  Write16Register(PGainAF, 0x0000);
  Write16Register(PGainBF, 0x0000);
  Write16Register(PGainCF, 0x0000);

  EndConfiguration();

  ApplyRuntimeCalibrationToRegisters();

  _begun = true;
  return true;
}

void ATM90E32::begin(int pin, unsigned short lineFreq, unsigned short pgagain, unsigned short ugain,
                     unsigned short igainA, unsigned short igainB, unsigned short igainC) {
  _usingLegacyBegin = true;
  _legacyLineFrequencyRaw = lineFreq;
  _config = Config();
  _config.csPin = pin;
  _config.lineFrequency = ShouldUseLegacy120VThresholds(lineFreq) ? LINE_FREQUENCY_60HZ : LINE_FREQUENCY_50HZ;
  _config.currentPhases = (lineFreq & (1U << 8)) ? CURRENT_PHASES_2 : CURRENT_PHASES_3;
  _config.pgaGain = static_cast<PgaGain>(pgagain);
  _config.phase[PHASE_A].voltageGain = ugain;
  _config.phase[PHASE_B].voltageGain = ugain;
  _config.phase[PHASE_C].voltageGain = ugain;
  _config.phase[PHASE_A].currentGain = igainA;
  _config.phase[PHASE_B].currentGain = igainB;
  _config.phase[PHASE_C].currentGain = igainC;

  _cs = pin;
  PopulateRuntimeFromConfig();

  const bool nominal120V = ShouldUseLegacy120VThresholds(lineFreq);
  const uint16_t freqHighThreshold = nominal120V ? 6100 : 5100;
  const uint16_t freqLowThreshold = nominal120V ? 5900 : 4900;
  const float sagVoltage = nominal120V ? 90.0f : 190.0f;
  const float divider = (2.0f * _phaseState[PHASE_A].voltageGain) / 32768.0f;
  const uint16_t sagThreshold = divider > 0.0f
                                    ? static_cast<uint16_t>((sagVoltage * 100.0f * ATM90E32_SQRT2) / divider)
                                    : 0;
  const uint16_t overVoltageThreshold =
      CalculateVoltageThreshold(_config.lineFrequency, _phaseState[PHASE_A].voltageGain, 1.22f);

  BeginInternal(lineFreq, 0x143F, freqHighThreshold, freqLowThreshold, sagThreshold, overVoltageThreshold);
}

bool ATM90E32::begin(const Config &config) {
  _usingLegacyBegin = false;
  _legacyLineFrequencyRaw = 0;
  _config = config;
  _cs = config.csPin;

  PopulateRuntimeFromConfig();

#if defined(ARDUINO_ARCH_ESP32)
  if (_config.enableOffsetCalibration) {
    LoadOffsetPreferences();
    LoadPowerOffsetPreferences();
  }
  if (_config.enableGainCalibration) {
    LoadGainPreferences();
  }
#endif

  uint16_t mmode0 = 0x0087;
  if (_config.lineFrequency == LINE_FREQUENCY_60HZ) {
    mmode0 |= (1U << 12);
  }
  if (_config.currentPhases == CURRENT_PHASES_2) {
    mmode0 |= (1U << 8);
    mmode0 &= static_cast<uint16_t>(~(1U << 1));
  }

  const uint16_t freqHighThreshold = (_config.lineFrequency == LINE_FREQUENCY_60HZ) ? 6300 : 5300;
  const uint16_t freqLowThreshold = (_config.lineFrequency == LINE_FREQUENCY_60HZ) ? 5700 : 4700;
  const uint16_t sagThreshold =
      CalculateVoltageThreshold(_config.lineFrequency, _phaseState[PHASE_A].voltageGain, 0.78f);
  const uint16_t overVoltageThreshold =
      CalculateVoltageThreshold(_config.lineFrequency, _phaseState[PHASE_A].voltageGain, 1.22f);

  return BeginInternal(mmode0, 0xFF3F, freqHighThreshold, freqLowThreshold, sagThreshold, overVoltageThreshold);
}

uint16_t ATM90E32::CalculateVoltageThreshold(LineFrequency lineFrequency, uint16_t voltageGain,
                                             float multiplier) const {
  const float nominalVoltage = (lineFrequency == LINE_FREQUENCY_60HZ) ? 120.0f : 220.0f;
  const float targetVoltage = nominalVoltage * multiplier;
  const float peak01V = targetVoltage * 100.0f * ATM90E32_SQRT2;
  const float divider = (2.0f * voltageGain) / 32768.0f;
  if (divider <= 0.0f) {
    return 0;
  }
  return static_cast<uint16_t>(peak01V / divider);
}

void ATM90E32::SetReferenceVoltage(Phase phase, float referenceVoltage) {
  const uint8_t index = static_cast<uint8_t>(phase);
  _config.phase[index].referenceVoltage = referenceVoltage;
  _phaseState[index].referenceVoltage = referenceVoltage;
}

void ATM90E32::SetReferenceCurrent(Phase phase, float referenceCurrent) {
  const uint8_t index = static_cast<uint8_t>(phase);
  _config.phase[index].referenceCurrent = referenceCurrent;
  _phaseState[index].referenceCurrent = referenceCurrent;
}

void ATM90E32::SetPeakCurrentSigned(bool peakCurrentSigned) {
  _config.peakCurrentSigned = peakCurrentSigned;
}

void ATM90E32::SetEnableOffsetCalibration(bool enableOffsetCalibration) {
  _config.enableOffsetCalibration = enableOffsetCalibration;
}

void ATM90E32::SetEnableGainCalibration(bool enableGainCalibration) {
  _config.enableGainCalibration = enableGainCalibration;
}

double ATM90E32::GetAverageVoltage(Phase phase, uint8_t sampleCount) {
  if (sampleCount == 0) {
    return 0.0;
  }

  uint32_t accumulation = 0;
  for (uint8_t i = 0; i < sampleCount; i++) {
    const uint16_t voltage = Read16Register(GetVoltageRegister(phase));
    ValidateLastSPIData(voltage);
    accumulation += voltage;
  }

  return static_cast<double>(accumulation) / static_cast<double>(sampleCount) / 100.0;
}

double ATM90E32::GetAverageCurrent(Phase phase, uint8_t sampleCount) {
  if (sampleCount == 0) {
    return 0.0;
  }

  uint32_t accumulation = 0;
  for (uint8_t i = 0; i < sampleCount; i++) {
    const uint16_t current = Read16Register(GetCurrentRegister(phase));
    ValidateLastSPIData(current);
    accumulation += current;
  }

  return static_cast<double>(accumulation) / static_cast<double>(sampleCount) / 1000.0;
}

int16_t ATM90E32::CalibrateOffset(Phase phase, bool voltage) {
  const uint8_t sampleCount = 5;
  uint64_t totalValue = 0;

  for (uint8_t i = 0; i < sampleCount; i++) {
    const uint16_t high = Read16Register(voltage ? GetVoltageRegister(phase) : GetCurrentRegister(phase));
    const uint16_t low = Read16Register(voltage ? GetVoltageLowRegister(phase) : GetCurrentLowRegister(phase));
    const uint32_t reading = (static_cast<uint32_t>(high) << 16) | low;
    totalValue += reading;
  }

  const uint32_t averageValue = static_cast<uint32_t>(totalValue / sampleCount);
  const uint32_t shifted = averageValue >> 7;
  const uint32_t offset = (~shifted) + 1;
  return static_cast<int16_t>(offset);
}

int16_t ATM90E32::CalibratePowerOffset(Phase phase, bool reactive) {
  const uint8_t sampleCount = 5;
  int64_t totalValue = 0;

  for (uint8_t i = 0; i < sampleCount; i++) {
    totalValue += reactive
                      ? Read32Register(GetReactivePowerRegister(phase), GetReactivePowerLowRegister(phase))
                      : Read32Register(GetActivePowerRegister(phase), GetActivePowerLowRegister(phase));
  }

  const int32_t averageValue = static_cast<int32_t>(totalValue / sampleCount);
  const int32_t powerOffset = -averageValue;
  return static_cast<int16_t>(powerOffset);
}

double ATM90E32::CalculateVIOffset(unsigned short regh_addr, unsigned short regl_addr) {
  const uint8_t sampleCount = 5;
  uint64_t totalValue = 0;

  for (uint8_t i = 0; i < sampleCount; i++) {
    const uint16_t valueHigh = Read16Register(regh_addr);
    const uint16_t valueLow = Read16Register(regl_addr);
    totalValue += (static_cast<uint32_t>(valueHigh) << 16) | valueLow;
  }

  const uint32_t averageValue = static_cast<uint32_t>(totalValue / sampleCount);
  const uint32_t shifted = averageValue >> 7;
  const uint32_t offset = (~shifted) + 1;
  return static_cast<uint16_t>(offset);
}

double ATM90E32::CalculatePowerOffset(unsigned short regh_addr, unsigned short regl_addr) {
  const uint8_t sampleCount = 5;
  int64_t totalValue = 0;

  for (uint8_t i = 0; i < sampleCount; i++) {
    totalValue += Read32Register(regh_addr, regl_addr);
  }

  const int32_t averageValue = static_cast<int32_t>(totalValue / sampleCount);
  return static_cast<int16_t>(-averageValue);
}

double ATM90E32::CalibrateVI(unsigned short reg, unsigned short actualVal) {
  if (!_begun) {
    return 0.0;
  }

  uint16_t gainRegister = 0;
  Phase phase = PHASE_A;
  bool isVoltage = true;

  switch (reg) {
    case UrmsA:
      gainRegister = UgainA;
      phase = PHASE_A;
      isVoltage = true;
      break;
    case UrmsB:
      gainRegister = UgainB;
      phase = PHASE_B;
      isVoltage = true;
      break;
    case UrmsC:
      gainRegister = UgainC;
      phase = PHASE_C;
      isVoltage = true;
      break;
    case IrmsA:
      gainRegister = IgainA;
      phase = PHASE_A;
      isVoltage = false;
      break;
    case IrmsB:
      gainRegister = IgainB;
      phase = PHASE_B;
      isVoltage = false;
      break;
    case IrmsC:
      gainRegister = IgainC;
      phase = PHASE_C;
      isVoltage = false;
      break;
    default:
      return 0.0;
  }

  uint32_t sampleTotal = 0;
  for (uint8_t i = 0; i < 4; i++) {
    sampleTotal += Read16Register(reg);
  }
  const uint16_t measuredAverage = static_cast<uint16_t>(sampleTotal / 4U);
  if (measuredAverage == 0) {
    return 0.0;
  }

  const uint16_t currentGain = Read16Register(gainRegister);
  uint32_t newGain = (static_cast<uint32_t>(actualVal) * currentGain) / measuredAverage;
  if (newGain > 0xFFFFUL) {
    newGain = 0xFFFFUL;
  }

  BeginConfiguration();
  Write16Register(gainRegister, static_cast<uint16_t>(newGain));
  EndConfiguration();

  if (isVoltage) {
    _phaseState[static_cast<uint8_t>(phase)].voltageGain = static_cast<uint16_t>(newGain);
  } else {
    _phaseState[static_cast<uint8_t>(phase)].currentGain = static_cast<uint16_t>(newGain);
  }

  return static_cast<double>(newGain);
}

bool ATM90E32::RunGainCalibration() {
  if (!_begun || !_config.enableGainCalibration) {
    return false;
  }

  for (uint8_t phase = 0; phase < 3; phase++) {
    const Phase currentPhase = static_cast<Phase>(phase);
    const double measuredVoltage = GetAverageVoltage(currentPhase, 10);
    const double measuredCurrent = GetAverageCurrent(currentPhase, 10);
    const uint16_t currentVoltageGain = Read16Register(GetVoltageGainRegister(currentPhase));
    const uint16_t currentCurrentGain = Read16Register(GetCurrentGainRegister(currentPhase));

    if (_phaseState[phase].referenceVoltage > 0.0f && measuredVoltage > 0.0) {
      uint32_t newVoltageGain = static_cast<uint32_t>(
          (_phaseState[phase].referenceVoltage / static_cast<float>(measuredVoltage)) * currentVoltageGain);
      if (newVoltageGain == 0) {
        newVoltageGain = currentVoltageGain;
      }
      if (newVoltageGain > 0xFFFFUL) {
        newVoltageGain = 0xFFFFUL;
      }
      _phaseState[phase].voltageGain = static_cast<uint16_t>(newVoltageGain);
    }

    if (_phaseState[phase].referenceCurrent > 0.0f && measuredCurrent > 0.0) {
      uint32_t newCurrentGain = static_cast<uint32_t>(
          (_phaseState[phase].referenceCurrent / static_cast<float>(measuredCurrent)) * currentCurrentGain);
      if (newCurrentGain == 0) {
        newCurrentGain = currentCurrentGain;
      }
      if (newCurrentGain > 0xFFFFUL) {
        newCurrentGain = 0xFFFFUL;
      }
      _phaseState[phase].currentGain = static_cast<uint16_t>(newCurrentGain);
    }
  }

  ApplyGainRegisters();
  const bool verifySuccess = VerifyGainWrites();
  const bool saveSuccess =
#if defined(ARDUINO_ARCH_ESP32)
      SaveGainPreferences();
#else
      true;
#endif

  return verifySuccess && saveSuccess;
}

bool ATM90E32::RunOffsetCalibration() {
  if (!_begun || !_config.enableOffsetCalibration) {
    return false;
  }

  for (uint8_t phase = 0; phase < 3; phase++) {
    const Phase currentPhase = static_cast<Phase>(phase);
    _phaseState[phase].voltageOffset = CalibrateOffset(currentPhase, true);
    _phaseState[phase].currentOffset = CalibrateOffset(currentPhase, false);
    ApplyOffsetRegisters(currentPhase);
  }

  const bool saveSuccess =
#if defined(ARDUINO_ARCH_ESP32)
      SaveOffsetPreferences();
#else
      true;
#endif

  return saveSuccess;
}

bool ATM90E32::RunPowerOffsetCalibration() {
  if (!_begun || !_config.enableOffsetCalibration) {
    return false;
  }

  for (uint8_t phase = 0; phase < 3; phase++) {
    const Phase currentPhase = static_cast<Phase>(phase);
    _phaseState[phase].activePowerOffset = CalibratePowerOffset(currentPhase, false);
    _phaseState[phase].reactivePowerOffset = CalibratePowerOffset(currentPhase, true);
    ApplyPowerOffsetRegisters(currentPhase);
  }

  const bool saveSuccess =
#if defined(ARDUINO_ARCH_ESP32)
      SavePowerOffsetPreferences();
#else
      true;
#endif

  return saveSuccess;
}

bool ATM90E32::ClearGainCalibration() {
  if (!_begun) {
    return false;
  }

  ClearRuntimeToConfigGains();
  ApplyGainRegisters();
  const bool verifySuccess = VerifyGainWrites();
  const bool clearSuccess =
#if defined(ARDUINO_ARCH_ESP32)
      RemovePreferenceKey("gain");
#else
      true;
#endif

  return verifySuccess && clearSuccess;
}

bool ATM90E32::ClearOffsetCalibration() {
  if (!_begun) {
    return false;
  }

  ClearRuntimeToConfigOffsets();
  for (uint8_t phase = 0; phase < 3; phase++) {
    ApplyOffsetRegisters(static_cast<Phase>(phase));
  }

  const bool clearSuccess =
#if defined(ARDUINO_ARCH_ESP32)
      RemovePreferenceKey("offs");
#else
      true;
#endif

  return clearSuccess;
}

bool ATM90E32::ClearPowerOffsetCalibration() {
  if (!_begun) {
    return false;
  }

  ClearRuntimeToConfigPowerOffsets();
  for (uint8_t phase = 0; phase < 3; phase++) {
    ApplyPowerOffsetRegisters(static_cast<Phase>(phase));
  }

  const bool clearSuccess =
#if defined(ARDUINO_ARCH_ESP32)
      RemovePreferenceKey("pofs");
#else
      true;
#endif

  return clearSuccess;
}

double ATM90E32::GetLineVoltage(Phase phase) {
  const uint16_t voltage = Read16Register(GetVoltageRegister(phase));
  ValidateLastSPIData(voltage);
  return static_cast<double>(voltage) / 100.0;
}

double ATM90E32::GetLineCurrent(Phase phase) {
  const uint16_t current = Read16Register(GetCurrentRegister(phase));
  ValidateLastSPIData(current);
  return static_cast<double>(current) / 1000.0;
}

double ATM90E32::GetActivePower(Phase phase) {
  return static_cast<double>(Read32Register(GetActivePowerRegister(phase), GetActivePowerLowRegister(phase))) *
         ATM90E32_POWER_LSB;
}

double ATM90E32::GetReactivePower(Phase phase) {
  return static_cast<double>(Read32Register(GetReactivePowerRegister(phase), GetReactivePowerLowRegister(phase))) *
         ATM90E32_POWER_LSB;
}

double ATM90E32::GetApparentPower(Phase phase) {
  return static_cast<double>(Read32Register(GetApparentPowerRegister(phase), GetApparentPowerLowRegister(phase))) *
         ATM90E32_POWER_LSB;
}

double ATM90E32::GetPowerFactor(Phase phase) {
  const uint16_t powerFactorRaw = Read16Register(GetPowerFactorRegister(phase));
  ValidateLastSPIData(powerFactorRaw);
  const int16_t powerFactor = static_cast<int16_t>(powerFactorRaw);
  return static_cast<double>(powerFactor) / 1000.0;
}

double ATM90E32::GetPhaseRaw(Phase phase) {
  const uint16_t angle = Read16Register(GetPhaseAngleRegister(phase));
  return static_cast<double>(angle) / 10.0;
}

double ATM90E32::GetPhaseSigned(Phase phase) {
  const double angle = GetPhaseRaw(phase);
  return (angle > 180.0) ? (angle - 360.0) : angle;
}

double ATM90E32::GetHarmonicActivePower(Phase phase) {
  return static_cast<double>(Read32Register(GetHarmonicActivePowerRegister(phase),
                                            GetHarmonicActivePowerLowRegister(phase))) *
         ATM90E32_POWER_LSB;
}

double ATM90E32::GetPeakCurrent(Phase phase) {
  int16_t peak = static_cast<int16_t>(Read16Register(GetPeakCurrentRegister(phase)));
  if (!_config.peakCurrentSigned && peak < 0) {
    peak = static_cast<int16_t>(-peak);
  }
  return (static_cast<double>(peak) * _phaseState[static_cast<uint8_t>(phase)].currentGain) / 8192000.0;
}

double ATM90E32::GetAccumulatedEnergy(Phase phase, uint16_t reg, uint32_t &counter) {
  const uint16_t increment = Read16Register(reg);
  if (UINT32_MAX - counter > increment) {
    counter += increment;
  } else {
    counter = increment;
  }
  (void)phase;
  return static_cast<double>(counter) * ATM90E32_ENERGY_COUNT_TO_WH;
}

double ATM90E32::GetLineVoltageA() {
  return GetLineVoltage(PHASE_A);
}

double ATM90E32::GetLineVoltageB() {
  return GetLineVoltage(PHASE_B);
}

double ATM90E32::GetLineVoltageC() {
  return GetLineVoltage(PHASE_C);
}

double ATM90E32::GetLineCurrentA() {
  return GetLineCurrent(PHASE_A);
}

double ATM90E32::GetLineCurrentB() {
  return GetLineCurrent(PHASE_B);
}

double ATM90E32::GetLineCurrentC() {
  return GetLineCurrent(PHASE_C);
}

double ATM90E32::GetLineCurrentN() {
  return static_cast<double>(Read16Register(IrmsN)) / 1000.0;
}

double ATM90E32::GetActivePowerA() {
  return GetActivePower(PHASE_A);
}

double ATM90E32::GetActivePowerB() {
  return GetActivePower(PHASE_B);
}

double ATM90E32::GetActivePowerC() {
  return GetActivePower(PHASE_C);
}

double ATM90E32::GetTotalActivePower() {
  return static_cast<double>(Read32Register(PmeanT, PmeanTLSB)) * ATM90E32_POWER_LSB;
}

double ATM90E32::GetTotalActiveFundPower() {
  return static_cast<double>(Read32Register(PmeanTF, PmeanTFLSB)) * ATM90E32_POWER_LSB;
}

double ATM90E32::GetTotalActiveHarPower() {
  return static_cast<double>(Read32Register(PmeanTH, PmeanTHLSB)) * ATM90E32_POWER_LSB;
}

double ATM90E32::GetActiveHarPowerA() {
  return GetHarmonicActivePower(PHASE_A);
}

double ATM90E32::GetActiveHarPowerB() {
  return GetHarmonicActivePower(PHASE_B);
}

double ATM90E32::GetActiveHarPowerC() {
  return GetHarmonicActivePower(PHASE_C);
}

double ATM90E32::GetHarmonicActivePowerA() {
  return GetActiveHarPowerA();
}

double ATM90E32::GetHarmonicActivePowerB() {
  return GetActiveHarPowerB();
}

double ATM90E32::GetHarmonicActivePowerC() {
  return GetActiveHarPowerC();
}

double ATM90E32::GetReactivePowerA() {
  return GetReactivePower(PHASE_A);
}

double ATM90E32::GetReactivePowerB() {
  return GetReactivePower(PHASE_B);
}

double ATM90E32::GetReactivePowerC() {
  return GetReactivePower(PHASE_C);
}

double ATM90E32::GetTotalReactivePower() {
  return static_cast<double>(Read32Register(QmeanT, QmeanTLSB)) * ATM90E32_POWER_LSB;
}

double ATM90E32::GetApparentPowerA() {
  return GetApparentPower(PHASE_A);
}

double ATM90E32::GetApparentPowerB() {
  return GetApparentPower(PHASE_B);
}

double ATM90E32::GetApparentPowerC() {
  return GetApparentPower(PHASE_C);
}

double ATM90E32::GetTotalApparentPower() {
  return static_cast<double>(Read32Register(SmeanT, SAmeanTLSB)) * ATM90E32_POWER_LSB;
}

double ATM90E32::GetFrequency() {
  return static_cast<double>(Read16Register(Freq)) / 100.0;
}

double ATM90E32::GetPowerFactorA() {
  return GetPowerFactor(PHASE_A);
}

double ATM90E32::GetPowerFactorB() {
  return GetPowerFactor(PHASE_B);
}

double ATM90E32::GetPowerFactorC() {
  return GetPowerFactor(PHASE_C);
}

double ATM90E32::GetTotalPowerFactor() {
  return static_cast<double>(static_cast<int16_t>(Read16Register(PFmeanT))) / 1000.0;
}

double ATM90E32::GetPhaseA() {
  return GetPhaseRaw(PHASE_A);
}

double ATM90E32::GetPhaseB() {
  return GetPhaseRaw(PHASE_B);
}

double ATM90E32::GetPhaseC() {
  return GetPhaseRaw(PHASE_C);
}

double ATM90E32::GetSignedPhaseA() {
  return GetPhaseSigned(PHASE_A);
}

double ATM90E32::GetSignedPhaseB() {
  return GetPhaseSigned(PHASE_B);
}

double ATM90E32::GetSignedPhaseC() {
  return GetPhaseSigned(PHASE_C);
}

double ATM90E32::GetSignedPhaseAngleA() {
  return GetSignedPhaseA();
}

double ATM90E32::GetSignedPhaseAngleB() {
  return GetSignedPhaseB();
}

double ATM90E32::GetSignedPhaseAngleC() {
  return GetSignedPhaseC();
}

double ATM90E32::GetPeakCurrentA() {
  return GetPeakCurrent(PHASE_A);
}

double ATM90E32::GetPeakCurrentB() {
  return GetPeakCurrent(PHASE_B);
}

double ATM90E32::GetPeakCurrentC() {
  return GetPeakCurrent(PHASE_C);
}

double ATM90E32::GetTemperature() {
  return static_cast<double>(static_cast<int16_t>(Read16Register(Temp)));
}

double ATM90E32::GetValueRegister(unsigned short registerRead) {
  return static_cast<double>(Read16Register(registerRead));
}

double ATM90E32::GetImportEnergy() {
  return static_cast<double>(Read16Register(APenergyT)) * ATM90E32_TOTAL_ENERGY_TO_KWH;
}

double ATM90E32::GetImportReactiveEnergy() {
  return static_cast<double>(Read16Register(RPenergyT)) * ATM90E32_TOTAL_ENERGY_TO_KWH;
}

double ATM90E32::GetImportApparentEnergy() {
  return static_cast<double>(Read16Register(SAenergyT)) * ATM90E32_TOTAL_ENERGY_TO_KWH;
}

double ATM90E32::GetExportEnergy() {
  return static_cast<double>(Read16Register(ANenergyT)) * ATM90E32_TOTAL_ENERGY_TO_KWH;
}

double ATM90E32::GetExportReactiveEnergy() {
  return static_cast<double>(Read16Register(RNenergyT)) * ATM90E32_TOTAL_ENERGY_TO_KWH;
}

double ATM90E32::GetImportEnergyA() {
  return GetAccumulatedEnergy(PHASE_A, GetForwardActiveEnergyRegister(PHASE_A),
                              _phaseState[PHASE_A].cumulativeForwardActiveEnergy);
}

double ATM90E32::GetImportEnergyB() {
  return GetAccumulatedEnergy(PHASE_B, GetForwardActiveEnergyRegister(PHASE_B),
                              _phaseState[PHASE_B].cumulativeForwardActiveEnergy);
}

double ATM90E32::GetImportEnergyC() {
  return GetAccumulatedEnergy(PHASE_C, GetForwardActiveEnergyRegister(PHASE_C),
                              _phaseState[PHASE_C].cumulativeForwardActiveEnergy);
}

double ATM90E32::GetExportEnergyA() {
  return GetAccumulatedEnergy(PHASE_A, GetReverseActiveEnergyRegister(PHASE_A),
                              _phaseState[PHASE_A].cumulativeReverseActiveEnergy);
}

double ATM90E32::GetExportEnergyB() {
  return GetAccumulatedEnergy(PHASE_B, GetReverseActiveEnergyRegister(PHASE_B),
                              _phaseState[PHASE_B].cumulativeReverseActiveEnergy);
}

double ATM90E32::GetExportEnergyC() {
  return GetAccumulatedEnergy(PHASE_C, GetReverseActiveEnergyRegister(PHASE_C),
                              _phaseState[PHASE_C].cumulativeReverseActiveEnergy);
}

double ATM90E32::GetForwardActiveEnergyA() {
  return GetImportEnergyA();
}

double ATM90E32::GetForwardActiveEnergyB() {
  return GetImportEnergyB();
}

double ATM90E32::GetForwardActiveEnergyC() {
  return GetImportEnergyC();
}

double ATM90E32::GetReverseActiveEnergyA() {
  return GetExportEnergyA();
}

double ATM90E32::GetReverseActiveEnergyB() {
  return GetExportEnergyB();
}

double ATM90E32::GetReverseActiveEnergyC() {
  return GetExportEnergyC();
}

ATM90E32::PhaseStatus ATM90E32::GetPhaseStatus(Phase phase) {
  const uint16_t state0 = Read16Register(EMMState0);
  const uint16_t state1 = Read16Register(EMMState1);

  PhaseStatus status;
  switch (phase) {
    case PHASE_A:
      status.overVoltage = (state0 & EMMSTATE0_OVPHASEA) != 0;
      status.voltageSag = (state1 & EMMSTATE1_SAGPHASEA) != 0;
      status.phaseLoss = (state1 & EMMSTATE1_PHASELOSSA) != 0;
      break;
    case PHASE_B:
      status.overVoltage = (state0 & EMMSTATE0_OVPHASEB) != 0;
      status.voltageSag = (state1 & EMMSTATE1_SAGPHASEB) != 0;
      status.phaseLoss = (state1 & EMMSTATE1_PHASELOSSB) != 0;
      break;
    case PHASE_C:
    default:
      status.overVoltage = (state0 & EMMSTATE0_OVPHASEC) != 0;
      status.voltageSag = (state1 & EMMSTATE1_SAGPHASEC) != 0;
      status.phaseLoss = (state1 & EMMSTATE1_PHASELOSSC) != 0;
      break;
  }

  status.overCurrent = GetLineCurrent(phase) > ATM90E32_OVER_CURRENT_THRESHOLD;
  return status;
}

String ATM90E32::GetPhaseStatusText(Phase phase) {
  const PhaseStatus status = GetPhaseStatus(phase);
  if (status.IsOk()) {
    return String("Okay");
  }

  String text;
  if (status.overVoltage) {
    text += "Over Voltage";
  }
  if (status.voltageSag) {
    if (text.length() > 0) {
      text += "; ";
    }
    text += "Voltage Sag";
  }
  if (status.phaseLoss) {
    if (text.length() > 0) {
      text += "; ";
    }
    text += "Phase Loss";
  }
  if (status.overCurrent) {
    if (text.length() > 0) {
      text += "; ";
    }
    text += "Over Current";
  }
  return text;
}

ATM90E32::FrequencyStatus ATM90E32::GetFrequencyStatus() {
  const uint16_t state1 = Read16Register(EMMState1);
  if ((state1 & EMMSTATE1_FREQHI) != 0) {
    return FREQUENCY_STATUS_HIGH;
  }
  if ((state1 & EMMSTATE1_FREQLO) != 0) {
    return FREQUENCY_STATUS_LOW;
  }
  return FREQUENCY_STATUS_NORMAL;
}

String ATM90E32::GetFrequencyStatusText() {
  const FrequencyStatus status = GetFrequencyStatus();
  switch (status) {
    case FREQUENCY_STATUS_HIGH:
      return String("HIGH");
    case FREQUENCY_STATUS_LOW:
      return String("LOW");
    default:
      return String("Normal");
  }
}

unsigned short ATM90E32::GetSysStatus0() {
  return Read16Register(EMMIntState0);
}

unsigned short ATM90E32::GetSysStatus1() {
  return Read16Register(EMMIntState1);
}

unsigned short ATM90E32::GetMeterStatus0() {
  return Read16Register(EMMState0);
}

unsigned short ATM90E32::GetMeterStatus1() {
  return Read16Register(EMMState1);
}

#if defined(ARDUINO_ARCH_ESP32)
void ATM90E32::BuildPreferenceKey(char *buffer, size_t bufferSize, const char *prefix) const {
  snprintf(buffer, bufferSize, "%s_%d", prefix, _cs);
}

bool ATM90E32::RemovePreferenceKey(const char *keyPrefix) {
  Preferences preferences;
  if (!preferences.begin("atm90e32", false)) {
    return false;
  }

  char key[16];
  BuildPreferenceKey(key, sizeof(key), keyPrefix);
  const bool removed = preferences.remove(key);
  preferences.end();
  return removed;
}

bool ATM90E32::LoadGainPreferences() {
  Preferences preferences;
  if (!preferences.begin("atm90e32", true)) {
    return false;
  }

  char key[16];
  BuildPreferenceKey(key, sizeof(key), "gain");
  GainBlob blob;
  const size_t length = preferences.getBytesLength(key);
  const size_t bytesRead = (length == sizeof(blob)) ? preferences.getBytes(key, &blob, sizeof(blob)) : 0;
  preferences.end();

  if (bytesRead != sizeof(blob) || blob.magic != ATM90E32_GAIN_MAGIC) {
    return false;
  }

  for (uint8_t phase = 0; phase < 3; phase++) {
    _phaseState[phase].voltageGain = blob.phase[phase].voltageGain;
    _phaseState[phase].currentGain = blob.phase[phase].currentGain;
  }
  return true;
}

bool ATM90E32::LoadOffsetPreferences() {
  Preferences preferences;
  if (!preferences.begin("atm90e32", true)) {
    return false;
  }

  char key[16];
  BuildPreferenceKey(key, sizeof(key), "offs");
  OffsetBlob blob;
  const size_t length = preferences.getBytesLength(key);
  const size_t bytesRead = (length == sizeof(blob)) ? preferences.getBytes(key, &blob, sizeof(blob)) : 0;
  preferences.end();

  if (bytesRead != sizeof(blob) || blob.magic != ATM90E32_OFFSET_MAGIC) {
    return false;
  }

  for (uint8_t phase = 0; phase < 3; phase++) {
    _phaseState[phase].voltageOffset = blob.phase[phase].voltageOffset;
    _phaseState[phase].currentOffset = blob.phase[phase].currentOffset;
  }
  return true;
}

bool ATM90E32::LoadPowerOffsetPreferences() {
  Preferences preferences;
  if (!preferences.begin("atm90e32", true)) {
    return false;
  }

  char key[16];
  BuildPreferenceKey(key, sizeof(key), "pofs");
  PowerOffsetBlob blob;
  const size_t length = preferences.getBytesLength(key);
  const size_t bytesRead = (length == sizeof(blob)) ? preferences.getBytes(key, &blob, sizeof(blob)) : 0;
  preferences.end();

  if (bytesRead != sizeof(blob) || blob.magic != ATM90E32_POWER_OFFSET_MAGIC) {
    return false;
  }

  for (uint8_t phase = 0; phase < 3; phase++) {
    _phaseState[phase].activePowerOffset = blob.phase[phase].activePowerOffset;
    _phaseState[phase].reactivePowerOffset = blob.phase[phase].reactivePowerOffset;
  }
  return true;
}

bool ATM90E32::SaveGainPreferences() {
  Preferences preferences;
  if (!preferences.begin("atm90e32", false)) {
    return false;
  }

  GainBlob blob;
  blob.magic = ATM90E32_GAIN_MAGIC;
  for (uint8_t phase = 0; phase < 3; phase++) {
    blob.phase[phase].voltageGain = _phaseState[phase].voltageGain;
    blob.phase[phase].currentGain = _phaseState[phase].currentGain;
  }

  char key[16];
  BuildPreferenceKey(key, sizeof(key), "gain");
  const bool success = preferences.putBytes(key, &blob, sizeof(blob)) == sizeof(blob);
  preferences.end();
  return success;
}

bool ATM90E32::SaveOffsetPreferences() {
  Preferences preferences;
  if (!preferences.begin("atm90e32", false)) {
    return false;
  }

  OffsetBlob blob;
  blob.magic = ATM90E32_OFFSET_MAGIC;
  for (uint8_t phase = 0; phase < 3; phase++) {
    blob.phase[phase].voltageOffset = _phaseState[phase].voltageOffset;
    blob.phase[phase].currentOffset = _phaseState[phase].currentOffset;
  }

  char key[16];
  BuildPreferenceKey(key, sizeof(key), "offs");
  const bool success = preferences.putBytes(key, &blob, sizeof(blob)) == sizeof(blob);
  preferences.end();
  return success;
}

bool ATM90E32::SavePowerOffsetPreferences() {
  Preferences preferences;
  if (!preferences.begin("atm90e32", false)) {
    return false;
  }

  PowerOffsetBlob blob;
  blob.magic = ATM90E32_POWER_OFFSET_MAGIC;
  for (uint8_t phase = 0; phase < 3; phase++) {
    blob.phase[phase].activePowerOffset = _phaseState[phase].activePowerOffset;
    blob.phase[phase].reactivePowerOffset = _phaseState[phase].reactivePowerOffset;
  }

  char key[16];
  BuildPreferenceKey(key, sizeof(key), "pofs");
  const bool success = preferences.putBytes(key, &blob, sizeof(blob)) == sizeof(blob);
  preferences.end();
  return success;
}
#endif
