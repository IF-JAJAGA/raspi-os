#include "vmem.h"
#include "constants.h"

uint32_t device_flags =
	1    << 0  | //bit de code exécutable
	1    << 1  | //bit à 1 par défaut
	0b01 << 2  | //2 bits correspondants à C & B
	0b00 << 4  | //2 bits correspondants à AP
	000  << 6  | //3 bits correspondants à TEX
	0    << 9  | //bit correspondant à APX
	0    << 10 | //bit correspondant à S (share)
	0    << 11 ; //bit correspondant à nG (not general)

uint32_t normal_flags =
	1     << 0  | //Xn
	1     << 1  | //default
	0b00  << 2  | //C & B
	0b11  << 4  | //AP
	0b001 << 6  | //TEX
	0     << 9  | //APX
	1     << 10 | //S
	0     << 11 ; //nG


unsigned int init_kern_translation_table (void) {
	// Points to an element of the level 1 TT
	

	/*
 	 *On veut remplir la table des pages en fonction des adresse logiques reçues.
 	 *Chaque table de page de niveau 1 renvoie à une table de page de niveau 2 contenant 256 entrées renvoyant à des pages de 4096 bits.
 	 *  On a donc une page de niveau 1 qui concerne : 2⁸ * 2¹² = 2²⁰ adresses (256*4096), ce qui correspond à 2²⁰ = 0x100000
 	 *  Comme on veut traduire que phy_adress = log_adress de 0x0 à 0x500000, cela concerne donc les 5 premières table de page de niveau 1
 	 *    Les tables de page de niveau 2 de ces tables de page de niveau 1 pointeront sur des adresses physiques égales aux adresses logiques
 	 *  On fait la même chose pour les log_adress comprises entre 0x20000000 et 0x20FFFFFF
 	 *  Pour les autres adresses, on remplie avec des adresses fausses (voir sujet)
 	 */
	for (unsigned int *tt_level_1 = (unsigned int *)TT1_BASE, *max_1 = tt_level_1 + TT1_SIZE_WRD, i = 0;
			i < 5;// tt_level_1 < max_1;
			++i, ++tt_level_1) {

		*tt_level_1 =
			0b01                              << 0 | //bits pour Coarse page table base address
			0                                 << 2 | //bit pour SBZ
			1                                 << 3 | //bit pour NS
			0                                 << 4 | //bit pour SBZ (2)
			0b0000                            << 5 | //bits pour Domain
			0                                 << 9 | //bit pour P (pas supporté par processeur
			(TT1_BASE + (i+1) * TT1_SIZE_WRD) << 10; //adresse pour retrouver page dans TP LLV 2
			//BUFFER OVERFLOW : Seulement 22 bits disponibles et besoin de 24 bits

		unsigned int *tt2_base = (unsigned int *) (*tt_level_1 >> 10);
		for (unsigned int *tt_level_2 = tt2_base, *max_2 = tt_level_2 + TT2_SIZE_WRD, j = 0;
				tt_level_2 < max_2;
				++j, ++tt_level_2) {

			if (i < 5) {
				*tt_level_2 = 
					normal_flags                            |
					(i*TT2_SIZE_WRD + j*TT1_SIZE_WRD) << 12 ;
			} else if (i > 19 && i < 30) {
				*tt_level_2 =
					device_flags                            |
					(i*TT2_SIZE_WRD + j*TT1_SIZE_WRD) << 12 ;
			} else {
				*tt_level_2 = 0x0; //adresse de défaut
			}
		}
		unsigned int unused = 0; // pour le debug
	}
	unsigned int unused = 0; // pour le debug

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

unsigned int
translate(unsigned int va)
{
  unsigned int pa; /* The result */

  /* 1st and 2nd table addresses */
  unsigned int table_base;
  unsigned int second_level_table;

  /* Indexes */
  unsigned int first_level_index;
  unsigned int second_level_index;
  unsigned int page_index;
  
  /* Descriptors */
  unsigned int first_level_descriptor;
  unsigned int* first_level_descriptor_address;
  unsigned int second_level_descriptor;
  unsigned int* second_level_descriptor_address;

  table_base = TT1_BASE;
  
  /* Indexes*/
  first_level_index = (va >> 20);
  second_level_index = ((va << 12) >> 24);
  page_index = (va & 0x00000FFF);

  /* First level descriptor */
  first_level_descriptor_address = (unsigned int*) (table_base | (first_level_index << 2));
  first_level_descriptor = *(first_level_descriptor_address);

  /* Second level descriptor */
  second_level_table = (first_level_descriptor >> 10);
  second_level_descriptor_address = (unsigned int*) (second_level_table | (second_level_index << 2));
  second_level_descriptor = *((unsigned int*) second_level_descriptor_address);    

  /* Physical address */
  pa = (second_level_descriptor >> 12) | page_index;

  return pa;
}

