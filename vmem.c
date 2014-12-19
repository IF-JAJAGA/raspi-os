#include "vmem.h"
#include "constants.h"

uint32_t device_flags =
	1    << 0  | // Executable bit code
	1    << 1  | // Bit defaulting to 1
	0b01 << 2  | // 2 bits corresponding to C & B
	0b00 << 4  | // 2 bits corresponding to AP
	000  << 6  | // 3 bits correspondants à TEX
	0    << 9  | // Bit corresponding to APX
	0    << 10 | // Bit corresponding to S (share)
	0    << 11 ; // Bit corresponding to nG (not general)

uint32_t normal_flags =
	1     << 0  | //Xn
	1     << 1  | //default
	0b00  << 2  | //C & B
	0b11  << 4  | //AP
	0b001 << 6  | //TEX
	0     << 9  | //APX
	1     << 10 | //S
	0     << 11 ; //nG

unsigned int
init_kern_translation_table (void) {
	/**
	 * Filling the Translating Tables (TT) of level 1 and 2.
	 * Each level 1 cell redirects to a TT of level 2, itself containing 256 cells redirecting to pages of 4096 Bytes/octets
	 *    There is ONE level 1 TT which contains 2¹² cells, each concerning 2⁸ * 2¹² == 2²⁰ == 0x100000 physical addresses
	 *    As we want the virtual_addr == physical_addr for all virtual addresses from 0x0 to 0x500000, this concerns the 5 first cells of TT1
	 *      The level 2 TT will point to physical addresses equal to the virtual addresses given
	 *    We do the same for the virtual addresses between 0x20000000 and 0x20FFFFFF
	 *   For the other addresses, we fill the TT with WRONG addresses for now (see subject)
	 */
	for (unsigned int *tt_level_1 = (unsigned int *)TT1_BASE, i = 0;
	//		i < 5;
			i < TT1_SIZE_WRD;
			++i, ++tt_level_1) {

		*tt_level_1 = ((unsigned int *)TT2_1ST_BASE) + i*TT2_SIZE_WRD;
/*
		*tt_level_1 =
			0b01                              << 0 | // Bits for Coarse page table base address
			0                                 << 2 | // Bit for SBZ
			1                                 << 3 | // Bit for NS
			0                                 << 4 | // Bit for SBZ (2)
			0b0000                            << 5 | // Bits for Domain
			0                                 << 9 | // Bit for P (not supported by this CPU)
			(TT1_BASE + (i+1) * TT1_SIZE_WRD) << 10; // Address of the corresponding TT2
			// BUFFER OVERFLOW: Only 22 bits available, and we need 24
*/
		unsigned int *tt2_base = *tt_level_1 & 0x7FFFFF; // 23 bits
		for (unsigned int *tt_level_2 = tt2_base, j = 0;
				j < TT2_SIZE_WRD;
				++j, ++tt_level_2) {

			// i*PAGE_INDEX_RANGE*TT2_SIZE_WORD >> 12 == i << 8
			*tt_level_2 = (i << 8) | j;
/*
			if (i < 5) {
				*tt_level_2 = 
					normal_flags                            |
					(i*TT2_SIZE_OCT + j) << 10;
			} else if (i > 19 && i < 30) {
				*tt_level_2 =
					device_flags                            |
					(i*TT2_SIZE_WRD + j*PAGE_INDEX_RANGE) << 10;
			} else {
				*tt_level_2 = 0x0; // Rejecting address (translation fault)
			}
*/
		}
		unsigned int unused = 0; // For debug
	}
	unsigned int unused = 0; // For debug

	return 0;	
}

void start_mmu_C() {
	register unsigned int control;

	__asm("mcr p15, 0, %[zero], c1, c0, 0" : : [zero] "r"(0)); //Disable cache
	__asm("mcr p15, 0, r0, c7, c7, 0"); // Invalidate cache (data and instructions)
	__asm("mcr p15, 0, r0, c8, c7, 0"); //Invalidate TLB entries

	/* Enable ARMv6 MMU features (disable sub_page AP) */
	control = (1<<23) | (1 << 15) | (1 << 4) | 1;
	/* Invalidate the translation lookaside buffer (TLB) */
	__asm volatile("mcr p15, 0, %[data], c8, c7, 0" : : [data] "r" (0));
	/* Write control register */
	__asm volatile ("mcr p15, 0, %[control], c1, c0, 0" : : [control] "r" (control));
}

void configure_mmu_C() {
	register unsigned int pt_addr = TT1_BASE;
	//total++;

	/* Translation table 0 */
	__asm volatile ("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pt_addr));
	
	/* Translation table 1 */
	__asm volatile ("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pt_addr));

	/* Use translation table 0 for everything */
	__asm volatile ("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (0));

	/* Set Domain 0 ACL to "Manager", not enforcing memory permissions
 	 * Every mapped section/page is in domain 0
 	 */ 
	__asm volatile ("mcr p15, 0, %[r], c3, c0, 0" :: [r] "r" (0x3));
}

/*
unsigned int
translate(unsigned int va) {
	unsigned int pa; * The result *

	/* 1st and 2nd table addresses *
	unsigned int table_base;
	unsigned int second_level_table;

	/* Indexes *
	unsigned int first_level_index;
	unsigned int second_level_index;
	unsigned int page_index;

	/* Descriptors *
	unsigned int first_level_descriptor;
	unsigned int* first_level_descriptor_address;
	unsigned int second_level_descriptor;
	unsigned int* second_level_descriptor_address;

	__asm("mrc p15, 0, %[tb], c2, c0, 0" : [tb] "=r"(table_base));

	table_base = table_base & 0xFFFFC000;

	/* Indexes *
	first_level_index = (va >> 20);
	second_level_index = ((va << 12) >> 24);
	page_index = (va & 0x00000FFF);

	/* First level descriptor *
	first_level_descriptor_address = (unsigned int*) (table_base | (first_level_index << 2));
	first_level_descriptor = *(first_level_descriptor_address);

	/* Second level descriptor *
	second_level_table = first_level_descriptor & 0xFFFFFC00;
	second_level_descriptor_address = (unsigned int*) (second_level_table | (second_level_index << 2));
	second_level_descriptor = *((unsigned int*) second_level_descriptor_address);    

	/* Physical address *
	pa = (second_level_descriptor & 0xFFFFF000) | page_index;

	return pa;
}
*/

unsigned int
translate(unsigned int va) {
	unsigned int first_level_index = (va >> 20);
	unsigned int second_level_index = ((va << 12) >> 24);
	unsigned int page_index = (va & 0x00000FFF);

	unsigned int *tt1 = (unsigned int *) TT1_BASE;
	unsigned int *tt2 = (unsigned int *) tt1[first_level_index];

	unsigned int pa = 0xFFFFF000 & (tt2[second_level_index] << 12) | page_index;

	return pa;
}

