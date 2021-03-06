#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

// Global
// ======
extern const unsigned int WORD_SIZE_OCT;    // 4 = ARM word size (32 bits)
extern const unsigned int NUMBER_REGISTERS; // 14 = 13 all purpose registers (r0-r12) + lr

extern const unsigned int STACK_SIZE_WORDS; // 16384 = 4kB, stack size in words (divide by WORD_SIZE_OCT if necessary)

// Virtual Memory
// ==============

// Constants
extern const unsigned int PAGE_SIZE_OCT;        // 0x1000 = 4096 (bytes/octets)
extern const unsigned int TT1_SIZE_WRD;         // 0x1000 = Number of cells of the TT1 (index is 12 bits long)
extern const unsigned int TT2_SIZE_WRD;         //  0x100 = Number of cells of the TT2 (index is 8 bits long)
extern const unsigned int OFFSET_RANGE_OCT;     // 0x1000 = Number of accessible addresses with an offset (12 bits long)
extern const unsigned int KERNEL_SECT_MAX;      //    0x4 = Number of the last reserved section (TT1 entry) for the kernel (first is 0)
extern const unsigned int DEVICES_SECT_MIN;     //  0x200 = Number of the first reserved section (TT1 entry) for the devices
extern const unsigned int DEVICES_SECT_MAX;     //  0x20F = Number of the last reserved section (TT1 entry) for the devices
extern const unsigned int FRAME_TABLE_SIZE_OCT; // 0x8400 = Size of the frame table (each cell is one octet)

// Constant address
extern const unsigned int TT1_BASE; // 0x48000 = Adress of the level 1 Translation Table

// Calculated constants (see comments)
extern const unsigned int TT1_SIZE_OCT;      //   0x4000 = TT1_SIZE_WRD * WORD_SIZE_OCT
extern const unsigned int TT2_SIZE_OCT;      //    0x400 = TT2_SIZE_WRD * WORD_SIZE_OCT
extern const unsigned int TOTAL_TT_SIZE_OCT; // 0x404000 = TT1_SIZE_OCT + TT1_SIZE_WRD * TT2_SIZE_OCT
extern const unsigned int SECTION_SIZE_OCT;  // 0x100000 = TT2_SIZE_WRD * OFFSET_RANGE_OCT (indexable addresses for one TT1 entry)

// Calculated address
extern const unsigned int TT2_1ST_BASE;     //  0x4c000 = TT1_BASE + TT1_SIZE_OCT
extern const unsigned int FRAME_TABLE_BASE; // 0x44c000 = TT2_1ST_BASE + TT1_SIZE_WRD*TT2_SIZE_OCT

// System Calls
// ============

extern const unsigned int SYS_REBOOT;        // 1 = Number of SysCall reboot
extern const unsigned int SYS_WAIT;          // 2 = Number of SysCall wait

extern const int PM_RSTC;                    // 0x2010001c = For SysCall Reboot
extern const int PM_WDOG;                    // 0x20100024 = For SysCall Reboot
extern const int PM_PASSWORD;                // 0x5a000000 = For SysCall Reboot
extern const int PM_RSTC_WRCFG_FULL_RESET;   // 0x00000020 = For SysCall Reboot
#endif

