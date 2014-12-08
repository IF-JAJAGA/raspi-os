#include <stdint.h>

#include "hw.h"
#include "vmem.h"

unsigned int init_kern_translation_table (void) {
	unsigned int log_adress;
	unsigned int * ptr_adress_first_TT;	//endroit où on se trouve dans la table de page de niveau 1

	// /!\ A VERIFIER /!\
	for (ptr_adress_first_TT = MMUTABLEBASE; ptr_adress_first_TT < (MMUTABLEBASE + FIRST_LVL_TT_SIZE); ptr_adress_first_TT++) {
		ptr_adress_first_TT =
			0b01       << 0 | //bits pour Coarse page table base address
			0          << 2 | //bit pour SBZ
			1          << 3 | //bit pour NS
			0          << 4 | //bit pour SBZ (2)
			0b0000     << 5 | //bits pour Domain
			0          << 9 | //bit pour P (pas supporté par processeur
			log_adress << 10; //adresse pour retrouver page dans TP LLV 2

	/*
 	* On veut remplir la table des pages en fonction des adresse logiques reçues.
 	* Chaque table de page de niveau 1 renvoie à une table de page de niveau 2 contenant 256 entrées renvoyant à des pages de 4096 bits.
 	* 	On a donc une page de niveau 1 qui concerne : 2⁸ * 2¹² = 2²⁰ adresses (256*4096), ce qui correspond à 2²⁰ = 0x100000
 	* 	Comme on veut traduire que phy_adress = log_adress de 0x0 à 0x500000, cela concerne donc les 5 premières table de page de niveau 1
 	* 		Les tables de page de niveau 2 de ces tables de page de niveau 1 pointeront sur des adresses physiques égales aux adresses logiques
 	* 	On fait la même chose pour les log_adress comprises entre 0x20000000 et 0x20FFFFFF
 	* 	Pour les autres adresses, on remplie avec des adresses fausses (voir sujet)
 	*/
	for (ptr_adress_first_TT = MMUTABLEBASE; ptr_adress_first_TT < (MMUTABLEBASE + FIRST_LVL_TT_SIZE); ptr_adress_first_TT++) {
		unsigned int * ptr_adress_second_TT;
		unsigned int * ptr_adress_second_init = (*ptr_adress_first_TT)>>10;
		for (ptr_adress_second_TT = ptr_adress_second_init; ptr_adress_second_TT < (ptr_adress_second_init + SECOND_LVL_TT_SIZE); ptr_adress_second_TT++) {
			if(log_adress > 0x0 && log_adress < 0x500000) {
				*ptr_adress_second_TT = 
					normal_flags << 0  |
					log_adress   << 12 ;
			} else if (log_adress > 0x20000000 && log_adress < 0x20FFFFFF) {
				*ptr_adress_second_TT =
					device_flags << 0  |
					log_adress   << 12 ;
			} else {
				*ptr_adress_second_TT = 
					0b00 << 0; //adresse de défaut
			}
		}
	}
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
	register unsigned int pt_addr = MMUTABLEBASE;
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
