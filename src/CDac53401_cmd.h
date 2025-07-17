#ifndef ARDUINOPROJECTMODULE_DAC53401_CMD_H
#define ARDUINOPROJECTMODULE_DAC53401_CMD_H

//
// 
// Transcribed from TI Datasheet for DACx3401, SLASES7A – JULY 2019 – REVISED DECEMBER 2019 
// DACx340110-Bit and8-Bit, Voltage-Output Digital-to-AnalogConvertersWithNonvolatile Memory and PMBusTMCompatible I2C Interface in Tiny 2 × 2 WSON

#define REG_STATUS                (0xD0)
#define REG_GENERAL_CONFIG        (0xD1)
#define REG_MED_ALARM_CONFIG      (0xD2)
#define REG_TRIGGER               (0xD3)
#define REG_DATA                  (0x21)
#define REG_MARGIN_HIGH           (0x25)
#define REG_MARGIN_LOW            (0x26)
#define REG_PMBUS_OP              (0x01)
#define REG_PMBUS_STATUS_BYTE     (0x78)
#define REG_PMBUS_VERSION         (0x98)

// REG_STATUS
#define RD_STATUS_NVM_CRC_ALARM_USER(v)      ((v) & 0x8000)     // 0 : No CRC error in user NVM bits 1: CRC error in user NVM bits
#define RD_STATUS_NVM_CRC_ALARM_INTERNAL(v)  ((v) & 0x4000)     // 0 : No CRC error in internal NVM 1: CRC error in internal NVM bits
#define RD_STATUS_NVM_BUSY(v)                ((v) & 0x2000)     // 0 : NVM write or load completed, Write to DAC registers allowed 1 : NVM write or load in progress, Write to DAC registers not allowed
#define RD_STATUS_DAC_UPDATE_BUSY(v)         ((v) & 0x1000)     // 0 : DAC outputs updated, Write to DAC registers allowed 1 : DAC outputs update in progress, Write to DAC registers not allowed
#define RD_STATUS_DEVICE_ID(v)               ((v) & 0x003C)
#define RD_STATUS_VERSION_ID(v)              ((v) & 0x0003)
#define RD_STATUS_DEVICE_VERSION_ID(v)       ((v) & 0x003F)
#define DAC_STATUS_DEVICE_VERSION_ID_53401   (0x000C)
#define DAC_STATUS_DEVICE_VERSION_ID_43401   (0x0014)

// REG_GENERAL_CONFIG D1
#define RD_GENERAL_CONFIG_FUNC_CONFIG(v)     (((v) & 0xC000) >> 14)
#define RD_GENERAL_CONFIG_DEVICE_LOCK(v)     ((v) & 0x2000)
#define RD_GENERAL_CONFIG_EN_PMBUS(v)        ((v) & 0x1000)
#define RD_GENERAL_CONFIG_CODE_STEP(v)       (((v) & 0x0E00) >> 8)
#define RD_GENERAL_CONFIG_SLEW_RATE(v)       (((v) & 0x01E0) >> 4)
#define RD_GENERAL_CONFIG_DAC_PDN(v)         (((v) & 0x0018) >> 3)
#define RD_GENERAL_CONFIG_REF_EN(v)          ((v) & 0x0004)
#define RD_GENERAL_CONFIG_DAC_SPAN(v)        ((v) & 0x0003)

// 00 : Generates a triangle wave between MARGIN_HIGH to MARGIN_LOW, with slope defined by SLEW_RATE.
// 01: Generates Saw-Tooth wave between MARGIN_HIGH to MARGIN_LOW, with rising slope defined by SLEW_RATE and immediate falling edge.
// 10: Generates Saw-Tooth wave between MARGIN_HIGH to MARGIN_LOW, with falling slope defined by SLEW_RATE and immediate rising edge.
// 11: Generates a square wave between MARGIN_HIGH to MARGIN_LOW, with pulse high and low period defined by SLEW_RATE.
#define WR_GENERAL_CONFIG_FUNC_CONFIG(v)     (((v) << 14) & 0xC000)
// 0 : Device not locked
// 1: Device locked, the device locks all the registers. This bit can be reset (unlock device) by writing 0101 to the DEVICE_UNLOCK_CODE.
#define WR_GENERAL_CONFIG_DEVICE_LOCK(v)     ((v) ? 0x2000 : 0)
// 0: PMBus mode disabled 
// 1: PMBus mode enabled
#define WR_GENERAL_CONFIG_EN_PMBUS(v)        ((v) ? 0x1000 : 0)
// Code step for programmable slew rate control. 
// 000: Code step size = 1 LSB (default)
// 001: Code step size = 2 LSB
// 010: Code step size = 3 LSB
// 011: Code step size = 4 LSB 
// 100: Code step size = 6 LSB 
// 101: Code step size = 8 LSB 
// 110: Code step size = 16 LSB 
// 111: Code step size = 32 LSB
#define WR_GENERAL_CONFIG_CODE_STEP(v)       (((v) << 8) & 0x0E00)
// Slew rate for programmable slew rate control. 
// 0000: 25.6 μs (per step)
// 0001: 25.6 μs × 1.25 (per step)
// 0010: 25.6 μs × 1.50 (per step)
// 0011: 25.6 μs × 1.75 (per step) 
// 0100: 204.8 μs (per step)
// 0101: 204.8 μs × 1.25 (per step) 
// 0110: 204.8 μs × 1.50 (per step) 
// 0111: 204.8 μs × 1.75 (per step) 
// 1000: 1.6384 ms (per step)
// 1001: 1.6384 ms × 1.25 (per step) 
// 1010: 1.6384 ms × 1.50 (per step) 
// 1011: 1.6384 ms × 1.75 (per step) 
// 1100: 12 μs (per step)
// 1101: 8 μs (per step) 
// 1110: 4 μs (per step) 
// 1111: No slew (default)
#define WR_GENERAL_CONFIG_SLEW_RATE(v)       (((v) << 4) & 0x01E0)
// 00: Power up
// 01: Power down to 10K
// 10: Power down to high impedance (default) 
// 11: Power down to 10K
#define DAC_PDN_UP                  (0x00)
#define DAC_PDN_10K                 (0x01)
#define DAC_PDN_HI_Z                (0x02)
#define DAC_PDN_10K_2               (0x03)
#define WR_GENERAL_CONFIG_DAC_PDN(v)         (((v) << 3) & 0x0018)
// 0: Internal reference disabled, VDD is DAC reference voltage, DAC output range from 0 to VDD.
// 1: Internal reference enabled, DAC reference = 1.21 V
#define WR_GENERAL_CONFIG_REF_EN(v)          ((v) ? 0x0004 : 0)
// Only applicable when internal reference is enabled. 
// 00: Reference to VOUT gain 1.5X
// 01: Reference to VOUT gain 2X
// 10: Reference to VOUT gain 3X
// 11: Reference to VOUT gain 4X
// 00: Reference to VOUT gain 1_5X
// 01: Reference to VOUT gain 2X
// 10: Reference to VOUT gain 3X
// 11: Reference to VOUT gain 4X
#define DAC_SPAN_VREF_GAIN_1_5X              (0x00)
#define DAC_SPAN_VREF_GAIN_2X                (0x01)
#define DAC_SPAN_VREF_GAIN_3X                (0x02)
#define DAC_SPAN_VREF_GAIN_4X                (0x03)
#define WR_GENERAL_CONFIG_DAC_SPAN(v)        ((v) & 0x0003)

// #define REG_MED_ALARM_CONFIG
#define RD_MED_ALARM_CONFIG_HP(v)         ((v) & 0x0400)
#define RD_MED_ALARM_CONFIG_MP(v)         ((v) & 0x0200)
#define RD_MED_ALARM_CONFIG_LP(v)         ((v) & 0x0100)
#define RD_MED_ALARM_CONFIG_DEAD_TIME(v)  (((v) & 0x0030) >> 8)
#define RD_MED_ALARM_CONFIG_PULSE_OFF_TIME(v)       (((v) & 0x000C) >> 2)
#define RD_MED_ALARM_CONFIG_PULSE_ON_TIME(v)        ((v) & 0x0003)

// 0: No medical alarm waveform generated 
// 1: High priority medical alarm waveform generated
#define WR_MED_ALARM_CONFIG_HP(v)         ((v) ? 0x0400 : 0)
// 0: No medical alarm waveform generated
// 1: Medium priority medical alarm waveform generated
#define WR_MED_ALARM_CONFIG_MP(v)         ((v) ? 0x0200 : 0)
// 0: No medical alarm waveform generated
// 1: Low priority medical alarm waveform generated
#define WR_MED_ALARM_CONFIG_LP(v)         ((v) ? 0x0100 : 0)
// High priority alarm     Medium priority alarm     Low priority alarm 
// 00: 2.55 sec            00: 2.60 sec              00: 16 sec
// 01: 2.96 sec            01: 3.06 sec              01: 16 sec
// 10: 3.38 sec            10: 3.52 sec              10: 16 sec
// 11: 3.80 sec            11: 4.00 sec              11: 16 sec
#define WR_MED_ALARM_CONFIG_DEAD_TIME(v)  (((v) << 8) & 0x0030)
// High priority alarm     Medium priority alarm     Low priority alarm  
// 00: 15 msec             00: 40 msec               00: 40 msec  
// 01: 36 msec             01: 60 msec               01: 60 msec  
// 10: 58 msec             10: 80 msec               10: 80 msec  
// 11: 80 msec             11: 100 msec              11: 100 msec  
#define WR_MED_ALARM_CONFIG_PULSE_OFF_TIME(v)       (((v) << 2) & 0x000C)
// High priority alarm     Medium priority alarm     Low priority alarm  
// 00: 80 msec             00: 130 msec              00: 130 msec 
// 01: 103 msec            01: 153 msec              01: 153 msec 
// 10: 126 msec            10: 176 msec              10: 176 msec 
// 11: 150 msec            11: 200 msec              11: 200 msec 
#define WR_MED_ALARM_CONFIG_PULSE_ON_TIME(v)        ((v) & 0x0003)

// REG_TRIGGER
// #define RD_TRIGGER_DEVICE_UNLOCK_CODE(v)            (((v) & 0xF000) >> 12) 
// #define RD_TRIGGER_DEVICE_CONFIG_RESET(v)           ((v) & 0x0200)
// #define RD_TRIGGER_START_FUNC_GEN(v)                ((v) & 0x0100)
#define RD_TRIGGER_PMBUS_MARGIN_HIGH(v)             ((v) & 0x0080)
#define RD_TRIGGER_PMBUS_MARGIN_LOW(v)              ((v) & 0x0040)
// #define RD_TRIGGER_NVM_RELOAD(v)                    ((v) & 0x0020)
// #define RD_TRIGGER_NVM_PROG(v)                      ((v) & 0x0010)
// #define RD_TRIGGER_SW_RESET(v)                      ((v) & 0x000F)

// Write 0101 to unlock the device to bypass DEVICE_LOCK bit.
#define WR_TRIGGER_DEVICE_UNLOCK_CODE(v)            (((v) << 12) & 0xF000)
// 0: Device configuration reset not initiated
// 1: Device configuration reset initiated. All registers loaded with factory reset values.
#define WR_TRIGGER_DEVICE_CONFIG_RESET(v)           ((v) ? 0x0200 : 0)
// 0: Continuous waveform generation mode disabled
// 1: Continuous waveform generation mode enabled, device generates continuous waveform based on FUNC_CONFIG, MARGIN_LOW, and SLEW_RATE.
#define WR_TRIGGER_START_FUNC_GEN(v)                ((v) ? 0x0100 : 0)
// 0: PMBus margin high command not initiated
// 1: PMBus margin high command initiated, DAC output margins high to MARGIN_HIGH. This bit automatically resets to 0 after the DAC code reaches MARGIN_HIGH value.
#define WR_TRIGGER_PMBUS_MARGIN_HIGH(v)             ((v) ? 0x0080 : 0)
// 0: PMBus margin low command not initiated
// 1: PMBus margin low command initiated, DAC output margins low to MARGIN_LOW. This bit automatically resets to 0 after the DAC code reaches MARGIN_LOW value.
#define WR_TRIGGER_PMBUS_MARGIN_LOW(v)              ((v) ? 0x0040 : 0)
//0: NVM reload not initiated
// 1: NVM reload initiated, applicable DAC registers loaded with corresponding NVM. 
// NVM_BUSY bit set to 1 which this operation is in progress. This is a self-resetting bit.
#define WR_TRIGGER_NVM_RELOAD(v)                    ((v) ? 0x0020 : 0)
// 0: NVM write not initiated
// 1: NVM write initiated, NVM corresponding to applicable DAC registers loaded with existing register settings. 
//    NVM_BUSY bit set to 1 which this operation is in progress. This is a self-resetting bit.
#define WR_TRIGGER_NVM_PROG(v)                      ((v) ? 0x0010 : 0)
// 1000: Software reset not initiated
// 1010: Software reset initiated, DAC registers loaded with corresponding NVMs, all other registers loaded with default settings.
#define WR_TRIGGER_SW_RESET(v)                      ((v) ? 0x000F : 0)

// REG_DATA
// #define RD_DATA_DAC(v)            (((v) & 0x0FFC) >> 2)
// #define RD_DATA_DAC_43401(v)      (((v) & 0x0FF0) >> 4)

// Writing to the DAC_DATA register forces the respective DAC channel to update the active register data to the DAC_DATA.
// Data are in straight binary format and use the following format: 
// DAC53401: { DATA[9:0] }
// DAC43401: { DATA[7:0], X, X }
// X = Don’t care bits
#define DATA_DAC_MAX              (0x03FF)
#define DATA_DAC_MAX_43401        (0x0FF)
#define WR_DATA_DAC(v)            (((v << 2) & 0x0FFC))
#define WR_DATA_DAC_43401(v)      (((v) << 4) & 0x0FF0)

// REG_MARGIN_HIGH
// #define RD_MARGIN_HIGH_DATA(v)        (((v) & 0x0FFC) >> 2)
// #define RD_MARGIN_HIGH_DATA_43401(v)  (((v) & 0x0FF0) >> 4)

// Margin high code for DAC output.
// Data are in straight binary format and use the following format: 
// DAC53401: { MARGIN_HIGH[[9:0] }
// DAC43401: { MARGIN_HIGH[[7:0], X, X }
// X = Don’t care bits
#define WR_MARGIN_HIGH_DATA(v)            (((v) << 2) & 0x0FFC))
#define WR_MARGIN_HIGH_DATA_43401(v)      (((v) << 4) & 0x0FF0)

// REG_MARGIN_LOW
// #define RD_MARGIN_LOW_DATA(v)        (((v) & 0x0FFC) >> 2)
// #define RD_MARGIN_LOW_DATA_43401(v)  (((v) & 0x0FF0) >> 4)

// Margin low code for DAC output.
// Data are in straight binary format and use the following format: 
// DAC53401: { MARGIN_LOW[[9:0] }
// DAC43401: { MARGIN_LOW[[7:0], X, X }
// X = Don’t care bits
#define WR_MARGIN_LOW_DATA(v)            (((v << 2) & 0x0FFC))
#define WR_MARGIN_LOW_DATA_43401(v)      (((v) << 4) & 0x0FF0)

// REG_PMBUS_OP
#define RD_PMBUS_OP_CMD(v)          (((v) & 0xFF00) >> 8)

//PMBus operation commands
// 00h: Turn off
// 80h: Turn on
// A4h: Margin high, DAC output margins high to MARGIN_HIGH
// 94h: Margin low, DAC output margins low to MARGIN_LOW
#define WR_PMBUS_OP_CMD(v)          (((v) << 8) & 0xFF00)

// REG_PMBUS_STATUS_BYTE
#define RD_REG_PMBUS_STATUS_BYTE_CML(v)     (((v) & 0x0200))

// 0: No communication Fault
// 1: PMBus communication fault for timeout, write with incorrect number of clocks, read before write command, and so more; 
// reset this bit by writing 1.
#define WR_REG_PMBUS_STATUS_BYTE_CML(v)     (((v) & 0x0200))

// REG_PMBUS_VERSION
// PMBus version = 0x2200
#define RD_PMBUS_VERSION(v)         (((v) & 0xFF00))

#define WR_DAC_VREF(v)           WR_GENERAL_CONFIG_DAC_SPAN(v)
#define RD_DAC_VREF(v)           RD_GENERAL_CONFIG_DAC_SPAN(v)

#define DAC_VREF_EN              (WR_GENERAL_CONFIG_REF_EN(1))
#define DAC_VREF_VDD             (WR_GENERAL_CONFIG_REF_EN(0))

#endif //ARDUINOPROJECTMODULE_DAC53401_CMD_H
