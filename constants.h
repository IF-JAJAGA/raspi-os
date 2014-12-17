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
extern const unsigned int PAGE_SIZE_OCT;     //   0x1000 = 4096 (bytes/octets)
extern const unsigned int TT1_SIZE_WRD;      //   0x1000 = Number of cells of the TT1 (index is 12 bits long)
extern const unsigned int TT2_SIZE_WRD;      //    0x100 =  Number of cells of the TT2 (index is 8 bits long)
extern const unsigned int NB_ACCESSIBLE_ADD; //   0x1000 = Number of accessible addresses with an offset (12 bits long)
extern const unsigned int SECTION_SIZE;      // 0x100000 = 1MB (Mega byte/octet)

// Constant address
extern const unsigned int TT1_BASE; // 0x48000 = Adress of the level 1 Translation Table

// Calculated constants (see comments)
extern const unsigned int TT1_SIZE_OCT;      //   0x4000 = TT1_SIZE_WRD * WORD_SIZE_OCT
extern const unsigned int TT2_SIZE_OCT;      //    0x400 = TT2_SIZE_WRD * WORD_SIZE_OCT
extern const unsigned int TOTAL_TT_SIZE_OCT; // 0x404000 = TT1_SIZE_OCT + TT1_SIZE_WRD * TT2_SIZE_OCT

// Calculated address
extern const unsigned int TT2_1ST_BASE; //0x4c000 = TT1_BASE + TT1_SIZE_OCT

#endif

